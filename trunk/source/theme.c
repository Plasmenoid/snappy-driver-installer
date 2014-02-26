/*
This file is part of Snappy Driver Installer.

Snappy Driver Installer is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Snappy Driver Installer is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Snappy Driver Installer.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "main.h"

//{ Global vars
img_t box[BOX_NUM];
img_t icon[ICON_NUM];

monitor_t mon_lang,mon_theme;

WCHAR *themedata=0;
WCHAR *langdata=0;
WCHAR langlist[64][250];
WCHAR themelist[64][250];
//}

//{ Image
void box_init(img_t *img,int i)
{
    WCHAR *filename;
    int j;

    if(img->big&&!img->iscopy)free(img->big);
    memset(img,0,sizeof(img_t));

    img->index=boxindex[i];
    filename=(WCHAR *)D(img->index+4);
    if(!*filename)return;

    for(j=0;j<BOX_NUM;j++)
        if(box[j].index&&j!=i)
            if(!wcscmp(filename,(WCHAR *)D(box[j].index+4)))
    {
        printf("Match %d,'%ws'\n",j,D(box[j].index+4));
        img->big=box[j].big;
        img->bitmap=box[j].bitmap;
        img->dc=box[j].dc;
        img->sx=box[j].sx;
        img->sy=box[j].sy;
        img->iscopy=1;
        return;
    }

    if(wcsstr(filename,L"RES_"))
        image_loadRes(img,_wtoi(filename+4));
    else
        image_loadFile(img,filename);
}

void box_free(img_t *img)
{
    if(img->big&&!img->iscopy)free(img->big);
}

void icon_init(img_t *img,int i)
{
    WCHAR *filename;

    if(img->big&&!img->iscopy)free(img->big);
    memset(img,0,sizeof(img_t));

    filename=(WCHAR *)D(i);
    if(wcsstr(filename,L"RES_"))
        image_loadRes(img,_wtoi(filename+4));
    else
        image_loadFile(img,filename);
}

void icon_free(img_t *img)
{
    if(img->big&&!img->iscopy)free(img->big);
}
//}

//{ Vault
void vault_startmonitors()
{
    WCHAR buf[BUFLEN];

    wsprintf(buf,L"%s\\langs",data_dir);
    mon_lang=monitor_start(buf,FILE_NOTIFY_CHANGE_LAST_WRITE|FILE_NOTIFY_CHANGE_FILE_NAME,1,lang_callback);
    wsprintf(buf,L"%s\\themes",data_dir);
    mon_theme=monitor_start(buf,FILE_NOTIFY_CHANGE_LAST_WRITE|FILE_NOTIFY_CHANGE_FILE_NAME,1,theme_callback);
}

void vault_stopmonitors()
{
    if(mon_lang)monitor_stop(mon_lang);
    if(mon_theme)monitor_stop(mon_theme);
}

void vault_free()
{
    int i;

    for(i=0;i<THEME_NM;i++)
        if(theme[i].init==2)free((void *)theme[i].val);

    for(i=0;i<STR_NM;i++)
        if(language[i].init==2)free((void *)language[i].val);

    for(i=0;i<BOX_NUM;i++)box_free(&box[i]);
    for(i=0;i<ICON_NUM;i++)icon_free(&icon[i]);

    if(themedata)free(themedata);
    if(langdata)free(langdata);
}

void *vault_loadfile(const WCHAR *filename,int *sz)
{
    FILE *f;
    void *data;
    void *data1;
    WCHAR *p;

    f=_wfopen(filename,L"rb");
    if(!f)
    {
        log_err("ERROR in loadfile(): failed _wfopen(%ws)\n",filename);
        return 0;
    }

    fseek(f,0,SEEK_END);
    *sz=ftell(f)+4096;
    fseek(f,0,SEEK_SET);
    data=malloc(*sz*2+2);

    fread(data,2,1,f);
    if(!memcmp(data,"\xEF\xBB",2))// UTF-8
    {
        int szo;
        *sz-=3;
        fread(data,1,1,f);
        int q=fread(data,1,*sz,f);
        szo=MultiByteToWideChar(CP_UTF8,0,data,q,0,0);
        data1=malloc(szo*2+2);
        *sz=MultiByteToWideChar(CP_UTF8,0,data,q,data1,szo);
        free(data);
        fclose(f);
        return data1;
    }else
    if(!memcmp(data,"\xFF\xFE",2))// UTF-16 LE
    {
        fread(data,*sz,1,f);
        *sz>>=1;(*sz)--;
        fclose(f);
        return data;
    }else
    if(!memcmp(data,"\xFE\xFF",2))// UTF-16 BE
    {
        data1=malloc(*sz+2);
        fread(data,*sz,1,f);
        _swab(data,data1,*sz);
        free(data);
        *sz>>=1;(*sz)--;
        fclose(f);
        return data1;
    }else                             // ANSI
    {
        fclose(f);
        f=_wfopen(filename,L"rt");
        if(!f)
        {
            log_err("ERROR in loadfile(): failed _wfopen(%ws)\n",filename);
            free(data);
            return 0;
        }
        p=data;(*sz)--;
        while(!feof(f))
        {
            fgetws(p,*sz,f);
            p+=wcslen(p);
        }
        fclose(f);
        return data;
    }
}

int vault_findvar(entry_t *a,int num,WCHAR *str)
{
    int i;
    WCHAR *p;
    WCHAR c;

    while(*str&&(*str==L' '||*str==L'\t'))str++;
    p=str;
    while(*p&&(*p!=L' '&&*p!=L'\t'))p++;
    c=*p;
    *p=0;

    for(i=0;i<num;i++)if(!wcscmp(str,a[i].name))
    {
        *p=c;
        return i;
    }

    *p=c;
    return -1;
}

int vault_readvalue(const WCHAR *str)
{
    WCHAR *p;

    p=wcsstr(str,L"0x");
    return p?wcstol(str,0,16):_wtoi(str);
}

int vault_findstr(WCHAR *str)
{
    WCHAR *b,*e;

    b=wcschr(str,L'\"');
    if(!b)return 0;
    b++;
    e=wcschr(b,L'\"');
    if(!e)return 0;
    *e=0;
    return (int)b;
}

void vault_parse(WCHAR *data,entry_t *entry,int num,WCHAR **origdata)
{
    WCHAR *lhs,*rhs,*le;
    WCHAR *tmp;

    le=lhs=data;

    while(le)
    {
        // Split lines
        le=wcschr(lhs,L'\n');
        if(le)*le=0;

        // Comments
        if(wcsstr(lhs,L"//"))*wcsstr(lhs,L"//")=0;

        // Split LHS and RHS
        rhs=wcschr(lhs,L'=');
        if(!rhs)
        {
            lhs=le+1;
            continue;
        }
        *rhs=0;rhs++;

        // Parse LHS
        int r;
        r=vault_findvar(entry,num,lhs);
        if(r<0)
        {
            //printf("Error: unknown var '%ws'\n",lhs);
        }else
        {
            int v,r1,r2;
            r1=vault_findstr(rhs);
            r2=vault_findvar(entry,num,rhs);

            if(r1)               // String
            {
                int sz=wcslen((WCHAR *)r1)+1;

                if(entry[r].init==2)free((void *)entry[r].val);
                v=r1;
                tmp=(WCHAR *)malloc(sz*2);
                memcpy(tmp,(void *)v,sz*2);
                while(wcsstr(tmp,L"\\n"))
                {
                    WCHAR *yy=wcsstr(tmp,L"\\n");
                    wcscpy(yy,yy+1);
                    *yy=L'\n';
                }
                while(wcsstr(tmp,L"\\0"))
                {
                    WCHAR *yy=wcsstr(tmp,L"\\0");
                    wcscpy(yy,yy+1);
                    *yy=1;
                }
                {
                    int l=wcslen(tmp);
                    int i;
                    for(i=0;i<l;i++)if(tmp[i]==1)tmp[i]=0;
                }
                v=(int)tmp;
            }
            else if(r2>=0)      // Var
                v=entry[r2].val;
            else                // Number
                v=vault_readvalue(rhs);

            //if(r2<0)printf("-RHS:'%ws'\n",L"",rhs);
            //if(r2>=0)printf("+RHS:'%ws'\n",L"",rhs);
            //log("%d,%d,%X,{%ws|%ws}\n",r2,v,v,lhs,rhs);
            entry[r].val=v;
            entry[r].init=r1?2:1;
        }
        lhs=le+1;
    }
    if(*origdata)free(*origdata);
    *origdata=data;
}

void vault_loadfromfile(WCHAR *filename,entry_t *entry,int num,WCHAR **origdata)
{
    WCHAR *data;
    int sz;

    if(!filename[0])return;
    //printf("{%ws\n",filename);
    data=vault_loadfile(filename,&sz);
    if(!data)
    {
        log_err("ERROR in vault_loadfromfile(): failed to load '%ws'\n",filename);
        return;
    }
    data[sz]=0;
    vault_parse(data,entry,num,origdata);
}

void vault_loadfromres(int id,entry_t *entry,int num,WCHAR **origdata)
{
    WCHAR *data;
    char *data1;
    int sz,i;

    get_resource(id,(void **)&data1,&sz);
    data=malloc(sz*2+2);
    int j=0;
    for(i=0;i<sz;i++,j++)
    {
        if(data1[i]==L'\r')data[i]=L' ';else
        data[i]=data1[j];
    }
    data[sz]=0;
    vault_parse(data,entry,num,origdata);
    for(i=0;i<num;i++)
        if(entry[i].init<1)log_err("ERROR in vault_loadfromres: not initialized '%ws'\n",entry[i].name);
}
//}

//{ Lang/theme
void lang_load(WCHAR *filename)
{
    vault_loadfromres(IDR_LANG,language,STR_NM,&langdata);
    vault_loadfromfile(filename,language,STR_NM,&langdata);
}

void theme_load(WCHAR *filename)
{
    vault_loadfromres(IDR_THEME,theme,THEME_NM,&themedata);
    vault_loadfromfile(filename,theme,THEME_NM,&themedata);
}

void lang_set(int i)
{
    //printf("%d,'%ws'\n",i,langlist[i]);
    lang_load(langlist[i]);
}

void theme_set(int i)
{
    //printf("%d,'%ws'\n",i,themelist[i]);
    theme_load(themelist[i]);
    for(i=0;i<BOX_NUM;i++)box_init(&box[i],i);
    icon_init(&icon[0],ITEM_EXPAND_UP);
    icon_init(&icon[1],ITEM_EXPAND_UP_H);
    icon_init(&icon[2],ITEM_EXPAND_DOWN);
    icon_init(&icon[3],ITEM_EXPAND_DOWN_H);
    icon_init(&icon[4],BUTTON_BITMAP_UNCHECKED);
    icon_init(&icon[5],BUTTON_BITMAP_UNCHECKED_H);
    icon_init(&icon[6],BUTTON_BITMAP_CHECKED);
    icon_init(&icon[7],BUTTON_BITMAP_CHECKED_H);
}

void lang_enum(HWND hwnd,WCHAR *path,int locale)
{
    WCHAR buf[4096];
    WCHAR langauto[4096];
    WCHAR langauto2[4096];
    HANDLE hFind;
    WIN32_FIND_DATA FindFileData;
    int i=0;

    //SendMessage(hwnd,CB_ADDSTRING,0,(int)L"Default (English)");langs=1;
    langauto[0]=0;
    wcscpy(langauto2,L"Auto");

    wsprintf(buf,L"%s\\%s\\*.txt",data_dir,path);
    hFind=FindFirstFile(buf,&FindFileData);
    do
    if(!(FindFileData.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY))
    {
        wsprintf(buf,L"%s\\%s\\%s",data_dir,path,FindFileData.cFileName);
        vault_loadfromfile(buf,language,STR_NM,&langdata);
        if(language[STR_LANG_CODE].val==(locale&0xFF))
        {
            wsprintf(langauto2,L"Auto (%s)",STR(STR_LANG_NAME));
            wcscpy(langauto,buf);
        }
        SendMessage(hwnd,CB_ADDSTRING,0,(int)STR(STR_LANG_NAME));
        wcscpy(langlist[i],buf);
        i++;
    }
    while(FindNextFile(hFind,&FindFileData)!=0);
    FindClose(hFind);

    if(i)
    {
        SendMessage(hwnd,CB_ADDSTRING,0,(int)langauto2);
        wcscpy(langlist[i],langauto);
        i++;
    }
    if(!i)
    {
        SendMessage(hwnd,CB_ADDSTRING,0,(int)L"English");
        langlist[i][0]=0;
    }
}

void theme_enum(HWND hwnd,WCHAR *path)
{
    WCHAR buf[4096];
    HANDLE hFind;
    WIN32_FIND_DATA FindFileData;
    int i=0;

    //SendMessage(hwnd,CB_ADDSTRING,0,(int)L"Classic(default)");langs=1;
    wsprintf(buf,L"%s\\%s\\*.txt",data_dir,path);
    hFind=FindFirstFile(buf,&FindFileData);
    do
    if(!(FindFileData.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY))
    {
        wsprintf(buf,L"%s\\%s\\%s",data_dir,path,FindFileData.cFileName);
        vault_loadfromfile(buf,theme,THEME_NM,&themedata);
        SendMessage(hwnd,CB_ADDSTRING,0,(int)D(THEME_NAME));
        wcscpy(themelist[i],buf);
        i++;
    }
    while(FindNextFile(hFind,&FindFileData)!=0);
    FindClose(hFind);

    if(!i)
    {
        SendMessage(hwnd,CB_ADDSTRING,0,(int)L"(default)");
        themelist[i][0]=0;
    }
}

void CALLBACK lang_callback(LPTSTR szFile,DWORD action,LPARAM lParam)
{
    UNREFERENCED_PARAMETER(action);
    UNREFERENCED_PARAMETER(lParam);

    printf("Change %ws\n",szFile);
    PostMessage(hMain,WM_UPDATELANG,0,0);
}

void CALLBACK theme_callback(LPTSTR szFile,DWORD action,LPARAM lParam)
{
    UNREFERENCED_PARAMETER(action);
    UNREFERENCED_PARAMETER(lParam);

    printf("Change %ws\n",szFile);
    PostMessage(hMain,WM_UPDATETHEME,0,0);
}
//}

//{ Monitors
monitor_t monitor_start(LPCTSTR szDirectory, DWORD notifyFilter, int subdirs, FileChangeCallback callback)
{
	monitor_t pMonitor = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(*pMonitor));

	pMonitor->hDir=CreateFile(szDirectory,FILE_LIST_DIRECTORY,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
	                            NULL,OPEN_EXISTING,FILE_FLAG_BACKUP_SEMANTICS|FILE_FLAG_OVERLAPPED,NULL);

	if(pMonitor->hDir!=INVALID_HANDLE_VALUE)
	{
		pMonitor->ol.hEvent    = CreateEvent(NULL,TRUE,FALSE,NULL);
		pMonitor->notifyFilter = notifyFilter;
		pMonitor->callback     = callback;
		pMonitor->subdirs      = subdirs;

		if (monitor_refresh(pMonitor))
		{
			return pMonitor;
		}
		else
		{
			CloseHandle(pMonitor->ol.hEvent);
			CloseHandle(pMonitor->hDir);
		}
	}
	HeapFree(GetProcessHeap(),0,pMonitor);
	return NULL;
}

BOOL monitor_refresh(monitor_t pMonitor)
{
	return ReadDirectoryChangesW(pMonitor->hDir,pMonitor->buffer,sizeof(pMonitor->buffer),pMonitor->subdirs,
	                      pMonitor->notifyFilter,NULL,&pMonitor->ol,monitor_callback);
}

void CALLBACK monitor_callback(DWORD dwErrorCode,DWORD dwNumberOfBytesTransfered,LPOVERLAPPED lpOverlapped)
{
    UNREFERENCED_PARAMETER(dwNumberOfBytesTransfered);

	TCHAR szFile[MAX_PATH];
	PFILE_NOTIFY_INFORMATION pNotify;
	monitor_t pMonitor=(monitor_t)lpOverlapped;
	size_t offset=0;

	if(dwErrorCode==ERROR_SUCCESS)
	{
		do
		{
			pNotify=(PFILE_NOTIFY_INFORMATION)&pMonitor->buffer[offset];
			offset+=pNotify->NextEntryOffset;

#			if defined(UNICODE)
			{
			    lstrcpynW(szFile,pNotify->FileName,min(MAX_PATH,pNotify->FileNameLength/sizeof(WCHAR)+1));
			}
#			else
			{
			    int count = WideCharToMultiByte(CP_ACP,0,pNotify->FileName,
			                                    pNotify->FileNameLength/sizeof(WCHAR),
			                                    szFile,MAX_PATH-1,NULL,NULL);
			    szFile[count]=L'\0';
			}
#			endif

			pMonitor->callback(szFile,pNotify->Action,pMonitor->lParam);

		}while(pNotify->NextEntryOffset!=0);
	}
	if(!pMonitor->fStop)monitor_refresh(pMonitor);
}

void monitor_stop(monitor_t pMonitor)
{
	if(pMonitor)
	{
		pMonitor->fStop=TRUE;
		CancelIo(pMonitor->hDir);
		if(!HasOverlappedIoCompleted(&pMonitor->ol))SleepEx(5,TRUE);
		CloseHandle(pMonitor->ol.hEvent);
		CloseHandle(pMonitor->hDir);
		HeapFree(GetProcessHeap(),0,pMonitor);
	}
}
//}
