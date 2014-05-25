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

//#define DEBUG_EXTRACHECKS

//{ Global variables
int drp_count;
int drp_cur;
int loaded_unpacked=0;
const tbl_t table_version[NUM_VER_NAMES]=
{
    {"classguid",                  9},
    {"class",                      5},
    {"provider",                   8},
    {"catalogfile",                11},
    {"catalogfile.nt",             14},
    {"catalogfile.ntx86",          17},
    {"catalogfile.ntia64",         18},
    {"catalogfile.ntamd64",        19},
    {"driverver",                  9},
    {"driverpackagedisplayname",   24},
    {"driverpackagetype",          17}
};
//}

//{ Parse
void read_whitespace(parse_info_t *parse_info)
{
    char *p=parse_info->start;
    while(p<parse_info->strend)
    {
        switch(*p)
        {
            case 32:case '\t':
                p++;
                break;
            case '\\':
                p++;
                while(*p=='\r'&&*p=='\n'&&p<parse_info->strend)p++;
                break;
            default:
                parse_info->start=p;
                return;
        }
    }
    parse_info->start=p;
}

int read_item(parse_info_t *parse_info)
{
    char *strend=parse_info->strend;
    char *p=parse_info->start,*s1b=p,*s1e;

    parse_info->sb=0;
    parse_info->se=0;
    while(p<strend)
    {
        switch(*p)
        {
            case ' ':case '\t':case '\n':case '\r':case 0x1A:case '\\':
                p++;
                break;
            case ';':
                p++;
                while(p<strend&&*p!='\n'&&*p!='\r')p++;
                break;
            default:
                s1b=p;

                while(p<strend)
                {
                    switch(*p)
                    {
                        case '=':
                            parse_info->start=p+1;
                            s1e=p;
                            while(s1e[-1]==32||s1e[-1]=='\t')s1e--; // trim spaces
                            if(*s1b=='\"'){s1b++;s1e--;}
                            parse_info->sb=s1b;
                            parse_info->se=s1e;
                            str_sub(parse_info);
                            return 1;
                        case '\n':case '\r':
                            p++;
                            s1b=p;
#ifdef DEBUG_EXTRACHECKS
                            log("ERR1 '%.*s'\n",30,s1b);
#endif
                            break;
                        case ';':
                            while(*p!='\r'&&*p!='\n'&&p<strend)p++;
                            break;
                        default:
                            p++;
                    }
                }
        }
    }
    return 0;
}

int read_field(parse_info_t *parse_info)
{
    char *strend=parse_info->strend;
    char *p=parse_info->start,*s1b,*s1e;
    int flag=0;

    parse_info->sb=parse_info->se=p;
    if(p[-1]!='='&&p[-1]!=',')return 0;
    read_whitespace(parse_info);
    p=parse_info->start;
    s1b=p;
    if(*p=='\"')
    {
        s1b++;
        p++;
        while(p<strend)
        {
            switch(*p)
            {
                case '\r':case '\n':
                    p++;
#ifdef DEBUG_EXTRACHECKS
                    log("ERR2 '%.*s'\n",30,s1b-1);
#endif
                case '\"':
                    s1e=p;
                    p++;
                    parse_info->start=p+1;
                    parse_info->sb=s1b;
                    parse_info->se=s1e;
                    str_sub(parse_info);
                    return 1;
                default:
                    p++;
            }
        }
    }
    else
    {
        while(p<strend&&!flag)
        {
            switch(*p)
            {
                case ',':
                    s1e=p;
                    while(s1e[-1]==32||s1e[-1]=='\t')s1e--; // trim spaces
                    parse_info->start=p+1;
                    parse_info->sb=s1b;
                    parse_info->se=s1e;
                    str_sub(parse_info);
                    return 1;
                case '\n':case '\r':
                case ';':
                    s1e=p;
                    while((s1e[-1]==32||s1e[-1]=='\t')&&s1e>s1b)s1e--; // trim spaces
                    parse_info->start=p;
                    parse_info->sb=s1b;
                    parse_info->se=s1e;
                    str_sub(parse_info);
                    return s1e!=s1b;
                default:
                    p++;
            }
        }
    }
    return 0;
}

int read_number(char **en,char *v1e)
{
    char *v1b=*en;
    int n=atoi(*en);

    while(*v1b>='0'&&*v1b<='9'&&v1b<v1e)v1b++;
    if(v1b<v1e)v1b++;
    *en=v1b;
    return n;
}

int read_hex(parse_info_t *parse_info)
{
    int val=0;
    char *v1b,*v1e;

    v1b=parse_info->sb;
    v1e=parse_info->se;

    while((*v1b=='0'||*v1b=='x')&&v1b<v1e)v1b++;
    if(v1b<v1e)
    {
        val=toupper(*v1b)-(*v1b<='9'?'0':'A'-10);
    }
    v1b++;
    if(v1b<v1e)
    {
        val<<=4;
        val+=toupper(*v1b)-(*v1b<='9'?'0':'A'-10);
    }
    return val;
}

int read_date(parse_info_t *parse_info,version_t *t)
{
    int flag=0;
    char *v1b,*v1e;

    v1b=parse_info->sb;
    v1e=parse_info->se;

    while(!(*v1b>='0'&&*v1b<='9')&&v1b<v1e)v1b++;
    t->m=read_number(&v1b,v1e);
    t->d=read_number(&v1b,v1e);
    t->y=read_number(&v1b,v1e);
    if(t->y<100)t->y+=1900;

    if(t->y<1990)flag=1;
    if(t->y>2013)flag=2;
    switch(t->m)
    {
        case 1:case 3:case 5:case 7:case 8:case 10:case 12:
            if(t->d<1||t->d>31)flag=3;
            break;
        case 4:case 6:case 9:case 11:
            if(t->d<1||t->d>30)flag=4;
            break;
        case 2:
            if(t->d<1||t->d>((((t->y%4==0)&&(t->y%100))||(t->y%400==0))?29:28))flag=5;
            break;
        default:
            flag=6;
    }
    return flag;
}

void read_version(parse_info_t *parse_info,version_t *t)
{
    char *v1b,*v1e;

    v1b=parse_info->sb;
    v1e=parse_info->se;

    t->v1=read_number(&v1b,v1e);
    t->v2=read_number(&v1b,v1e);
    t->v3=read_number(&v1b,v1e);
    t->v4=read_number(&v1b,v1e);
}

void parse_init(parse_info_t *parse_info,driverpack_t *drp)
{
    parse_info->pack=drp;
    heap_init(&parse_info->strings,ID_PARSE,(void **)&parse_info->text,1024*64,1);
}

void parse_free(parse_info_t *parse_info)
{
    if(parse_info->strings.used)log("Parse_used %d\n",parse_info->strings.used);
    heap_free(&parse_info->strings);
}

void parse_set(parse_info_t *parse_info,char *inf_base,sect_data_t *lnk)
{
    parse_info->start=inf_base+lnk->ofs;
    parse_info->strend=inf_base+lnk->len;
}

void parse_getstr(parse_info_t *parse_info,char **vb,char **ve)
{
    *vb=parse_info->sb;
    *ve=parse_info->se;
}

void str_sub(parse_info_t *parse_info)
{
    static char static_buf[4096];
    int vers_len;
    char *v1b,*v1e;
    char *res;
    int isfound;

    v1b=parse_info->sb;
    v1e=parse_info->se;

    parse_info->se=v1e;

    if(*v1b=='%'/*&&v1e[-1]=='%'*/)
    {
        v1b++;
        vers_len=v1e-v1b-1;
        if(v1e[-1]!='%')vers_len++;
        if(vers_len<0)vers_len=0;

        strtolower(v1b,vers_len);
        res=(char *)hash_find(&parse_info->pack->string_list,v1b,vers_len,&isfound);
        if(isfound)
        {
            parse_info->sb=res;
            parse_info->se=res+strlen(res);
            return;
        }else
        {
            //if(memcmp(v1b,"system",5))
            //log("ERROR: string '%.*s' not found\n",vers_len+2,v1b-1);
            //return;
        }
    }
    char *p,*p_s=static_buf;
    v1b=parse_info->sb;
    int flag=0;
    while(v1b<v1e)
    {
        while(*v1b!='%'&&v1b<v1e)*p_s++=*v1b++;
        if(*v1b=='%')
        {
            //log("Deep replace %.*s\n",v1e-v1b,v1b);
            p=v1b+1;
            while(*p!='%'&&p<v1e)p++;
            if(*p=='%')
            {
                strtolower(v1b+1,p-v1b-1);
                res=(char *)hash_find(&parse_info->pack->string_list,v1b+1,p-v1b-1,&isfound);
                if(isfound)
                {
                    strcpy(p_s,res);
                    p_s+=strlen(res);
                    v1b=p+1;
                    flag=1;
                }
            }
            if(v1b<v1e)*p_s++=*v1b++;
        }
    }
    if(!flag)return;

    *p_s++=0;*p_s=0;
    p_s=static_buf;

    p_s=parse_info->text+heap_alloc(&parse_info->strings,strlen(p_s)+1);
    strcpy(p_s,static_buf);

    parse_info->sb=p_s;
    parse_info->se=p_s+strlen(p_s);
}
//}

//{ Misc
int unicode2ansi(char *s,char *out,int size)
{
    int ret,flag;
    size/=2;
    if(!out)log_err("Error out:\n");
    if(!s)log_err("Error in:\n");
    if(size<0)log_err("Error size:\n");
    ret=WideCharToMultiByte(CP_ACP,0,(WCHAR *)(s+(s[0]==-1?2:0)),size-(s[0]==-1?1:0),(CHAR *)out,size,0,&flag);
    if(!ret)log_err("Error:%d\n",GetLastError());
    out[size]=0;
    return ret;
}

int encode(char *dest,int dest_sz,char *src,int src_sz)
{
    Lzma86_Encode((Byte *)dest,(SizeT *)&dest_sz,(const Byte *)src,src_sz,0,1<<23,SZ_FILTER_AUTO);
    return dest_sz;
}

int decode(char *dest,int dest_sz,char *src,int src_sz)
{
    Lzma86_Decode((Byte *)dest,(SizeT *)&dest_sz,(const Byte *)src,(SizeT *)&src_sz);
    return dest_sz;
}

WCHAR *finddrp(WCHAR *s)
{
    return collection_finddrp(manager_g->matcher->col,s);
}

//}

//{ Collection
void collection_init(collection_t *col,WCHAR *driverpacks_dir,WCHAR *index_bin_dir,WCHAR *index_linear_dir,int flags_l)
{
    heap_init(&col->driverpack_handle,ID_DRIVERPACK,(void **)&col->driverpack_list,0,sizeof(driverpack_t));
    col->flags=flags_l;

    col->driverpack_dir=driverpacks_dir;
    col->index_bin_dir=index_bin_dir;
    col->index_linear_dir=index_linear_dir;
}

void collection_free(collection_t *col)
{
    int i;

    for(i=0;i<col->driverpack_handle.items;i++)
        driverpack_free(&col->driverpack_list[i]);

    heap_free(&col->driverpack_handle);
}

void collection_save(collection_t *col)
{
    HANDLE hFind = INVALID_HANDLE_VALUE;
    WIN32_FIND_DATA FindFileData;
    WCHAR buf1[BUFLEN];
    WCHAR buf2[BUFLEN];
    WCHAR buf3[BUFLEN];
    int i;

    time_indexsave=GetTickCount();

#ifndef CONSOLE_MODE
    // Save indexes
    if(*drpext_dir==0)
    {
        int count=0,cur=1;

        for(i=0;i<col->driverpack_handle.items;i++)
            if(col->driverpack_list[i].type==DRIVERPACK_TYPE_PENDING_SAVE)count++;

        log_con("Saving indexes...");
        for(i=0;i<col->driverpack_handle.items;i++)
        {
            if((flags&FLAG_KEEPUNPACKINDEX)==0&&!i)
            {
                cur++;
                continue;
            }
            if(col->driverpack_list[i].type==DRIVERPACK_TYPE_PENDING_SAVE)
            {
                if(flags&COLLECTION_USE_LZMA)
                {
                    WCHAR bufw2[BUFLEN];

                    wsprintf(bufw2,L"%ws\\%ws",
                        col->driverpack_list[i].text+col->driverpack_list[i].drppath,
                        col->driverpack_list[i].text+col->driverpack_list[i].drpfilename);

                    log_con("Saving indexes for '%ws'\n",bufw2);
                    manager_g->items_list[SLOT_INDEXING].isactive=2;
                    manager_g->items_list[SLOT_INDEXING].val1=cur-1;
                    manager_g->items_list[SLOT_INDEXING].val2=count-1;
                    wcscpy(manager_g->items_list[SLOT_INDEXING].txt1,bufw2);
                    manager_g->items_list[SLOT_INDEXING].percent=(cur)*1000/count;
                    manager_setpos(manager_g);
                    redrawfield();
                    cur++;
                }

                driverpack_saveindex(&col->driverpack_list[i]);
            }
        }
        manager_g->items_list[SLOT_INDEXING].isactive=0;
        manager_setpos(manager_g);
        log_con("DONE\n");
    }

    // Delete unused indexes
    if(*drpext_dir)return;
    if(!canWrite(col->index_bin_dir))
    {
        log_err("ERROR in collection_save(): Write-protected,'%ws'\n",col->index_bin_dir);
        return;
    }
    wsprintf(buf1,L"%ws\\*.*",col->index_bin_dir);
    hFind=FindFirstFile(buf1,&FindFileData);
    while(FindNextFile(hFind,&FindFileData)!=0)
    {
        if(!(FindFileData.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY))
        {
            wsprintf(buf3,L"%s\\%s",col->index_bin_dir,FindFileData.cFileName);
            for(i=flags&FLAG_KEEPUNPACKINDEX?0:1;i<col->driverpack_handle.items;i++)
            {
                driverpack_getindexfilename(&col->driverpack_list[i],col->index_bin_dir,L"bin",buf2);
                if(!wcscmp(buf2,buf3))break;
            }
            if(i==col->driverpack_handle.items&&!StrStrIW(buf3,L"\\_"))
            {
                printf("Deleting %ws\n",buf3);
                _wremove(buf3);
            }
        }
    }
#endif
    time_indexsave=GetTickCount()-time_indexsave;
}

int collection_scanfolder_count(collection_t *col,const WCHAR *path)
{
    HANDLE hFind = INVALID_HANDLE_VALUE;
    WIN32_FIND_DATA FindFileData;
    WCHAR buf[BUFLEN];
    driverpack_t drp;
    int cnt=0;

    wsprintf(buf,L"%ws\\*.*",path);
    hFind=FindFirstFile(buf,&FindFileData);

    while(FindNextFile(hFind,&FindFileData)!=0)
    {
        if(FindFileData.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)
        {
            if(lstrcmp(FindFileData.cFileName,L"..")==0)continue;
            wsprintf(buf,L"%ws\\%ws",path,FindFileData.cFileName);
            cnt+=collection_scanfolder_count(col,buf);
        } else
        {
            int len=lstrlen(FindFileData.cFileName);
            if(_wcsicmp(FindFileData.cFileName+len-3,L".7z")==0)
            {
                driverpack_init(&drp,path,FindFileData.cFileName,col);
                if(flags&COLLECTION_FORCE_REINDEXING||!driverpack_checkindex(&drp))cnt++;
                driverpack_free(&drp);
            }
        }
    }
    FindClose(hFind);
    return cnt;
}

void collection_updatedindexes(collection_t *col)
{
    HANDLE hFind = INVALID_HANDLE_VALUE;
    WIN32_FIND_DATA FindFileData;
    WCHAR buf[BUFLEN];
    WCHAR filename[BUFLEN];
    driverpack_t *drp;

    wsprintf(buf,L"%ws\\_*.*",col->index_bin_dir);
    hFind=FindFirstFile(buf,&FindFileData);

    while(FindNextFile(hFind,&FindFileData)!=0)
    {
        wsprintf(filename,L"%ws",FindFileData.cFileName);
        wcscpy(filename+wcslen(FindFileData.cFileName)-3,L"7z");

        int index=heap_allocitem_i(&col->driverpack_handle);

        drp=&col->driverpack_list[index];
        driverpack_init(drp,col->driverpack_dir,filename,col);
        driverpack_loadindex(drp);
    }
    FindClose(hFind);
}

void collection_load(collection_t *col)
{
    driverpack_t *unpacked_drp;

    time_indexes=GetTickCount();
    registerall();
    heap_reset(&col->driverpack_handle,0);
    unpacked_drp=(driverpack_t *)heap_allocitem_ptr(&col->driverpack_handle);
    driverpack_init(unpacked_drp,col->driverpack_dir,L"unpacked.7z",col);

//{thread
    int i;
    HANDLE thr;
    col->inflist=malloc(LSTCNT*sizeof(inflist_t));
    for(i=0;i<LSTCNT;i++)
    {
        col->inflist[i].dataready=CreateEvent(0,0,0,0);
        col->inflist[i].slotvacant=CreateEvent(0,0,1,0);
    }
    col->pos_in=col->pos_out=0;
    thr=(HANDLE)_beginthreadex(0,0,&thread_indexinf,col,0,0);
//}thread

    if(flags&FLAG_KEEPUNPACKINDEX)loaded_unpacked=driverpack_loadindex(unpacked_drp);
    drp_count=collection_scanfolder_count(col,col->driverpack_dir);
    drp_cur=0;
    collection_scanfolder(col,col->driverpack_dir);
    collection_updatedindexes(col);
    manager_g->items_list[SLOT_INDEXING].isactive=0;
    if(col->driverpack_handle.items<=1&&(flags&FLAG_DPINSTMODE)==0)
        itembar_settext(manager_g,SLOT_NODRIVERS,L"",0);
    driverpack_genhashes(&col->driverpack_list[0]);
    time_indexes=GetTickCount()-time_indexes;
    flags&=~COLLECTION_FORCE_REINDEXING;

//{thread
    driverpack_indexinf_async(0,col,L"",L"",0,0);
    WaitForSingleObject(thr,INFINITE);
    CloseHandle_log(thr,L"driverpack_genindex",L"thr");
    for(i=0;i<LSTCNT;i++)
    {
        CloseHandle_log(col->inflist[i].dataready,L"driverpack_genindex",L"dataready");
        CloseHandle_log(col->inflist[i].slotvacant,L"driverpack_genindex",L"slotvacant");
    }
    free(col->inflist);
//}thread
}

void collection_print(collection_t *col)
{
    int i;

    time_indexprint=GetTickCount();

    for(i=0;i<col->driverpack_handle.items;i++)
        driverpack_print(&col->driverpack_list[i]);

    time_indexprint=GetTickCount()-time_indexprint;
}

void collection_printstates(collection_t *col)
{
    int i,sum=0;
    driverpack_t *drp;
    WCHAR *s;

    if((log_verbose&LOG_VERBOSE_DRP)==0)return;
    log("Driverpacks\n");
    for(i=0;i<col->driverpack_handle.items;i++)
    {
        drp=&col->driverpack_list[i];
        s=(WCHAR *)drp->text;
        log("  %6d  %ws\\%ws\n",drp->HWID_list_handle.items,s+drp->drppath,s+drp->drpfilename);
        sum+=drp->HWID_list_handle.items;
    }
    log("  Sum: %d\n\n",sum);
}

WCHAR *collection_finddrp(collection_t *col,WCHAR *fnd)
{
    int i;
    driverpack_t *drp;
    WCHAR *s;

    for(i=0;i<col->driverpack_handle.items;i++)
    {
        drp=&col->driverpack_list[i];
        s=(WCHAR *)(drp->text+drp->drpfilename);
        if(StrStrIW(s,fnd))return s;
    }
    return 0;
}

void collection_scanfolder(collection_t *col,const WCHAR *path)
{
    HANDLE hFind = INVALID_HANDLE_VALUE;
    WIN32_FIND_DATA FindFileData;
    WCHAR buf[1024];
    driverpack_t *drp;

    wsprintf(buf,L"%s\\*.*",path);
    hFind=FindFirstFile(buf,&FindFileData);

    while(FindNextFile(hFind,&FindFileData)!=0)
    {
        if(FindFileData.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)
        {
            if(lstrcmp(FindFileData.cFileName,L"..")==0)continue;
            wsprintf(buf,L"%s\\%s",path,FindFileData.cFileName);
            collection_scanfolder(col,buf);
        } else
        {
            int len=lstrlen(FindFileData.cFileName);
            if(_wcsicmp(FindFileData.cFileName+len-3,L".7z")==0)
            {
                //drp=(driverpack_t *)heap_allocitem_ptr(&col->driverpack_handle);
                int index=heap_allocitem_i(&col->driverpack_handle);
                drp=&col->driverpack_list[index];
                driverpack_init(drp,path,FindFileData.cFileName,col);
                if(flags&COLLECTION_FORCE_REINDEXING||!driverpack_loadindex(drp))
                {
                    WCHAR bufw1[4096];
                    WCHAR bufw2[4096];
                    if(!drp_count)drp_count=1;
                    wsprintf(bufw1,L"Indexing %d/%d",drp_cur,drp_count);
                    wsprintf(bufw2,L"%s\\%s",path,FindFileData.cFileName);
                    manager_g->items_list[SLOT_INDEXING].isactive=1;
                    manager_g->items_list[SLOT_INDEXING].val1=drp_cur;
                    manager_g->items_list[SLOT_INDEXING].val2=drp_count;
                    itembar_settext(manager_g,SLOT_INDEXING,bufw2,(drp_cur)*1000/drp_count);
                    manager_setpos(manager_g);
                    drp=&col->driverpack_list[index];
                    driverpack_genindex(drp);
                    drp_cur++;
                }
            }else
            if(_wcsicmp(FindFileData.cFileName+len-4,L".inf")==0&&loaded_unpacked==0)
            {
                FILE *f;
                char *buft;
                wsprintf(buf,L"%s\\%s",path,FindFileData.cFileName);
                f=_wfopen(buf,L"rb");
                fseek(f,0,SEEK_END);
                len=ftell(f);
                fseek(f,0,SEEK_SET);
                buft=(char *)malloc(len);
                fread(buft,len,1,f);
                fclose(f);
                wsprintf(buf,L"%s\\",path+wcslen(col->driverpack_dir)+1);
                driverpack_indexinf(&col->driverpack_list[0],buf,FindFileData.cFileName,buft,len);
                free(buft);
            }else
            if(_wcsicmp(FindFileData.cFileName+len-4,L".cat")==0&&loaded_unpacked==0)
            {
                FILE *f;
                char *buft;
                wsprintf(buf,L"%s\\%s",path,FindFileData.cFileName);
                f=_wfopen(buf,L"rb");
                fseek(f,0,SEEK_END);
                len=ftell(f);
                fseek(f,0,SEEK_SET);
                buft=(char *)malloc(len);
                fread(buft,len,1,f);
                fclose(f);
                wsprintf(buf,L"%s\\",path+wcslen(col->driverpack_dir)+1);
//                driverpack_indexinf(&col->driverpack_list[0],buf,FindFileData.cFileName,buft,len);
                driverpack_parsecat(&col->driverpack_list[0],buf,FindFileData.cFileName,buft,len);
                free(buft);
            }
        }
    }
    FindClose(hFind);
}
//}

//{ Driverpack
void driverpack_init(driverpack_t *drp,WCHAR const *driverpack_path,WCHAR const *driverpack_filename,collection_t *col)
{
    char buf[BUFLEN];

    drp->col=col;

    heap_init(&drp->inffile_handle,ID_DRIVERPACK_inffile,(void **)&drp->inffile,0,sizeof(data_inffile_t));
    heap_init(&drp->manufacturer_handle,ID_DRIVERPACK_manufacturer,(void **)&drp->manufacturer_list,0,sizeof(data_manufacturer_t));
    heap_init(&drp->desc_list_handle,ID_DRIVERPACK_desc_list,(void **)&drp->desc_list,0,sizeof(data_desc_t));
    heap_init(&drp->HWID_list_handle,ID_DRIVERPACK_HWID_list,(void **)&drp->HWID_list,0,sizeof(data_HWID_t));
    heap_init(&drp->text_handle,ID_DRIVERPACK_text,(void **)&drp->text,0,1);

    hash_init(&drp->string_list,ID_STRINGS,1024,HASH_FLAG_KEYS_ARE_POINTERS|HASH_FLAG_STR_TO_LOWER);
    hash_init(&drp->section_list,ID_SECTIONS,16,HASH_FLAG_KEYS_ARE_POINTERS|HASH_FLAG_STR_TO_LOWER);
    hash_init(&drp->cat_list,ID_CAT_LIST,512*8,HASH_FLAG_STR_TO_LOWER);

    sprintf(buf,"%ws",driverpack_path);
    drp->drppath=heap_memcpy(&drp->text_handle,driverpack_path,wcslen(driverpack_path)*2+2);

    sprintf(buf,"%ws",driverpack_filename);
    drp->drpfilename=heap_memcpy(&drp->text_handle,driverpack_filename,wcslen(driverpack_filename)*2+2);
    drp->indexes.size=0;
}

void driverpack_free(driverpack_t *drp)
{
    heap_free(&drp->inffile_handle);
    heap_free(&drp->manufacturer_handle);
    heap_free(&drp->desc_list_handle);
    heap_free(&drp->HWID_list_handle);
    heap_free(&drp->text_handle);

    hash_free(&drp->string_list);
    hash_free(&drp->section_list);
    hash_free(&drp->cat_list);

    if(drp->indexes.size)hash_free(&drp->indexes);
}

void driverpack_saveindex(driverpack_t *drp)
{
    WCHAR filename[BUFLEN];
    FILE *f;
    int sz;
    int version=VER_INDEX;
    char *mem,*p,*mem_pack;

    driverpack_getindexfilename(drp,drp->col->index_bin_dir,L"bin",filename);
    if(!canWrite(filename))
    {
        log_err("ERROR in driverpack_saveindex(): Write-protected,'%ws'\n",filename);
        return;
    }
    f=_wfopen(filename,L"wb");

    sz=
        drp->inffile_handle.used+
        drp->manufacturer_handle.used+
        drp->desc_list_handle.used+
        drp->HWID_list_handle.used+
        drp->text_handle.used+
        drp->indexes.items_handle.used+sizeof(int)+
        6*sizeof(int)*2;

    p=mem=(char *)malloc(sz);

    fwrite("SDW",3,1,f);
    fwrite(&version,sizeof(int),1,f);

    p=heap_save(&drp->inffile_handle,p);
    p=heap_save(&drp->manufacturer_handle,p);
    p=heap_save(&drp->desc_list_handle,p);
    p=heap_save(&drp->HWID_list_handle,p);
    p=heap_save(&drp->text_handle,p);
    p=hash_save(&drp->indexes,p);

    if(drp->col->flags&COLLECTION_USE_LZMA)
    {
        mem_pack=(char *)malloc(sz);
        sz=encode(mem_pack,sz,mem,sz);
        fwrite(mem_pack,sz,1,f);
        free(mem_pack);
    }
    else fwrite(mem,sz,1,f);

    free(mem);
    fclose(f);
}

int driverpack_checkindex(driverpack_t *drp)
{
    WCHAR filename[BUFLEN];
    CHAR buf[3];
    FILE *f;
    int sz;
    int version;

    driverpack_getindexfilename(drp,drp->col->index_bin_dir,L"bin",filename);
    f=_wfopen(filename,L"rb");
    if(!f)return 0;

    fseek(f,0,SEEK_END);
    sz=ftell(f);
    fseek(f,0,SEEK_SET);

    fread(buf,3,1,f);
    fread(&version,sizeof(int),1,f);
    sz-=3+sizeof(int);

    if(memcmp(buf,"SDW",3)||version!=VER_INDEX)return 0;
    if(*drpext_dir)return 0;

    fclose(f);
    return 1;
}

int driverpack_loadindex(driverpack_t *drp)
{
    WCHAR filename[BUFLEN];
    CHAR buf[3];
    FILE *f;
    int sz;
    int version;
    char *mem,*p,*mem_unpack=0;

    driverpack_getindexfilename(drp,drp->col->index_bin_dir,L"bin",filename);
    f=_wfopen(filename,L"rb");
    drp->type=DRIVERPACK_TYPE_EMPTY;
    if(!f)return 0;

    fseek(f,0,SEEK_END);
    sz=ftell(f);
    fseek(f,0,SEEK_SET);

    fread(buf,3,1,f);
    fread(&version,sizeof(int),1,f);
    sz-=3+sizeof(int);

    if(memcmp(buf,"SDW",3)||version!=VER_INDEX)return 0;
    if(*drpext_dir)return 0;

    p=mem=(char *)malloc(sz);
    fread(mem,sz,1,f);

    if(drp->col->flags&COLLECTION_USE_LZMA)
    {
        UInt64 sz_unpack;

        Lzma86_GetUnpackSize((Byte *)p,sz,&sz_unpack);
        mem_unpack=(char *)malloc(sz_unpack);
        decode(mem_unpack,sz_unpack,mem,sz);
        p=mem_unpack;
    }

    p=heap_load(&drp->inffile_handle,p);
    p=heap_load(&drp->manufacturer_handle,p);
    p=heap_load(&drp->desc_list_handle,p);
    p=heap_load(&drp->HWID_list_handle,p);
    p=heap_load(&drp->text_handle,p);
    p=hash_load(&drp->indexes,p);

    free(mem);
    if(mem_unpack)free(mem_unpack);
    fclose(f);

    drp->type=StrStrIW(filename,L"\\_")?DRIVERPACK_TYPE_UPDATE:DRIVERPACK_TYPE_INDEXED;
    return 1;
}

void driverpack_getindexfilename(driverpack_t *drp,WCHAR *dir,const WCHAR *ext,WCHAR *indfile)
{
    WCHAR *p;
    WCHAR buf[BUFLEN];
    int len=wcslen((WCHAR *)(drp->text+drp->drpfilename));

    wsprintf(buf,L"%s",drp->text+drp->drpfilename);

    if(*(drp->text+drp->drppath))
        wsprintf(buf+(len-3)*1,L"%s.%s",(WCHAR *)(drp->text+drp->drppath)+lstrlen(drp->col->driverpack_dir),ext);
    else
        wsprintf(buf+(len-3)*1,L".%s",ext);

    p=buf;
    while(*p){if(*p==L'\\'||*p==L' ')*p=L'_';p++;}
    wsprintf(indfile,L"%s\\%s",dir,buf);
}

void driverpack_print(driverpack_t *drp)
{
    int inffile_index,manuf_index,pos,desc_index,HWID_index;
    int n=drp->inffile_handle.items;
    version_t *t;
    data_inffile_t *d_i;
    hwidmatch_t hwidmatch;
    char buf[4096];
    WCHAR filename[BUFLEN];
    FILE *f;
    int cnts[NUM_DECS],plain;
    int HWID_index_last=0;
    int manuf_index_last=0;
    int i;

    hwidmatch.drp=drp;
    driverpack_getindexfilename(drp,drp->col->index_linear_dir,L"txt",filename);
    f=_wfopen(filename,L"wt");

    fprintf(f,"%ws\\%ws (%d inf files)\n",drp->text+drp->drppath,drp->text+drp->drpfilename,n);
    for(inffile_index=0;inffile_index<n;inffile_index++)
    {
        d_i=&drp->inffile[inffile_index];
        fprintf(f,"  %s%s (%d bytes)\n",drp->text+d_i->infpath,drp->text+d_i->inffilename,d_i->infsize);
    for(i=0;i<n;i++)if(i!=inffile_index&&d_i->infcrc==drp->inffile[i].infcrc)
    fprintf(f,"**%s%s\n",drp->text+drp->inffile[i].infpath,drp->text+drp->inffile[i].inffilename);
        t=&d_i->version;
        fprintf(f,"    date\t\t\t%d/%d/%d\n",t->d,t->m,t->y);
        fprintf(f,"    version\t\t\t%d.%d.%d.%d\n",t->v1,t->v2,t->v3,t->v4);
        for(i=0;i<NUM_VER_NAMES;i++)
            if(d_i->fields[i])
            {
                fprintf(f,"    %-28s%s\n",table_version[i].s,drp->text+d_i->fields[i]);
                if(d_i->cats[i])fprintf(f,"      %s\n",drp->text+d_i->cats[i]);

            }

        memset(cnts,-1,sizeof(cnts));plain=0;
        for(manuf_index=manuf_index_last;manuf_index<drp->manufacturer_handle.items;manuf_index++)
            if(drp->manufacturer_list[manuf_index].inffile_index==inffile_index)
        {
            manuf_index_last=manuf_index;
            //hwidmatch.HWID_index=HWID_index_last;
            if(drp->manufacturer_list[manuf_index].manufacturer)
                fprintf(f,"      {%s}\n",drp->text+drp->manufacturer_list[manuf_index].manufacturer);
            for(pos=0;pos<drp->manufacturer_list[manuf_index].sections_n;pos++)
            {
                getdrp_drvsectionAtPos(drp,buf,pos,manuf_index);
                i=calc_secttype(buf);
                if(i>=0&&cnts[i]<0)cnts[i]=0;
                if(i<0&&pos>0)fprintf(f,"!!![%s]\n",buf);
                fprintf(f,"        [%s]\n",buf);
//                strcpy(buf+1000,buf);
                for(desc_index=0;desc_index<drp->desc_list_handle.items;desc_index++)
                    if(drp->desc_list[desc_index].manufacturer_index==manuf_index&&
                       drp->desc_list[desc_index].sect_pos==pos)
                {
                    for(HWID_index=HWID_index_last;HWID_index<drp->HWID_list_handle.items;HWID_index++)
                        if(drp->HWID_list[HWID_index].desc_index==desc_index)
                    {
                        if(HWID_index_last+1!=HWID_index&&HWID_index)fprintf(f,"Skip:%d,%d\n",HWID_index_last,HWID_index);
                        HWID_index_last=HWID_index;
                        hwidmatch.HWID_index=HWID_index_last;

                //if(drp->text+drp->manufacturer_list[manuf_index].manufacturer!=get_manufacturer(&hwidmatch))
                //fprintf(f,"*%s\n",get_manufacturer(&hwidmatch));
                //get_section(&hwidmatch,buf+500);
                //if(strcmp(buf+1000,buf+500))
                //fprintf(f,"*%s,%s\n",buf+1000,buf+500);
                        if(i>=0)cnts[i]++;
                        if(pos==0&&i<0)plain++;

                        if(getdrp_drvinfpos(&hwidmatch))
                            sprintf(buf,"%-2d",getdrp_drvinfpos(&hwidmatch));
                        else
                            sprintf(buf,"  ");

                        fprintf(f,"       %s %-50s%-20s\t%s\n",buf,
                            getdrp_drvHWID(&hwidmatch),
                            getdrp_drvinstall(&hwidmatch),
                            getdrp_drvdesc(&hwidmatch));
                        fprintf(f,"          feature:%-42hX%-20s\n\n",
                            getdrp_drvfeature(&hwidmatch)&0xFF,
                            getdrp_drvinstallPicked(&hwidmatch));
                    }
                    else if(HWID_index!=HWID_index_last)break;
                }
            }
        }
        else if(manuf_index!=manuf_index_last)break;

        fprintf(f,"  Decors:\n");
        fprintf(f,"    %-15s%d\n","plain",plain);
        for(i=0;i<NUM_DECS;i++)
        {
            if(cnts[i]>=0)fprintf(f,"    %-15s%d\n",nts[i],cnts[i]);
        }
        fprintf(f,"\n");
    }
    fprintf(f,"  HWIDS:%d/%d\n",HWID_index_last+1,drp->HWID_list_handle.items);

    //hash_stats(&drp->indexes);
/*    for(i=0;i<drp->indexes.items_handle.items;i++)
    {
        fprintf(f,"%d,%d,%d\n",i,drp->indexes.items[i].key,drp->indexes.items[i].next);
    }*/
    fclose(f);
}

void driverpack_genhashes(driverpack_t *drp)
{
    char filename[BUFLEN];
    int i,j,r;


    hash_init(&drp->indexes,ID_INDEXES,drp->HWID_list_handle.items/2,HASH_FLAG_STRS_ARE_INTS);
    //heap_expand(&t->indexes.strs_handle,64*1024);
    //log("Items: %d\n",pack->HWID_list_handle.items);
    for(i=0;i<drp->inffile_handle.items;i++)
    {
        sprintf(filename,"%s%s",drp->text+drp->inffile[i].infpath,drp->text+drp->inffile[i].inffilename);
        strtolower(filename,strlen(filename));
        //log_con("%s\n",filename);
        for(j=CatalogFile;j<=CatalogFile_ntamd64;j++)
        {
            if(drp->inffile[i].fields[j])
            {
                sprintf(filename,"%s%s",drp->text+drp->inffile[i].infpath,drp->text+drp->inffile[i].fields[j]);
                strtolower(filename,strlen(filename));
                //log_con("%d: (%s)\n",j,filename);
                r=hash_find_str(&drp->cat_list,filename);
                if(r)
                {
                    //log_con("^^%d: %s\n",r,drp->text+r);
                    drp->inffile[i].cats[j]=r;
                }
                //else log_con("Not found\n");

            }
        }
    }
    //hash_stats(&drp->cat_list);

    for(i=0;i<drp->HWID_list_handle.items;i++)
    {
        int val=0;
        char *vv=drp->text+drp->HWID_list[i].HWID;

        val=hash_getcode(vv,strlen(vv));
        hash_add(&drp->indexes,(char *)&val,sizeof(int),i,HASH_MODE_ADD);
    }
}

unsigned int __stdcall thread_indexinf(void *arg)
{
    collection_t *col=(collection_t *)arg;
    inflist_t *t;

    while(1)
    {
        t=&col->inflist[col->pos_out];
        if(++col->pos_out>=LSTCNT)col->pos_out=0;

        WaitForSingleObject(t->dataready,INFINITE);
        if(!t->drp)break;
        if(!*t->inffile)
        {
            driverpack_genhashes(t->drp);
            free(t->adr);
            SetEvent(t->slotvacant);
            continue;
        }

        driverpack_indexinf_ansi(t->drp,t->pathinf,t->inffile,t->adr,t->len);
        free(t->adr);
        SetEvent(t->slotvacant);
    }
    return 0;
}

void driverpack_indexinf_async(driverpack_t *drp,collection_t *colv,WCHAR const *pathinf,WCHAR const *inffile,char *adr,int len)
{
    collection_t *col=colv;
    inflist_t *t=&col->inflist[col->pos_in];
    if(++col->pos_in>=LSTCNT)col->pos_in=0;

    WaitForSingleObject(t->slotvacant,INFINITE);
    if(len>4&&((adr[0]==-1&&adr[3]==0)||adr[0]==0))
    {
        t->adr=(char *)malloc(len+2);
        if(!t->adr)
        {
            log_err("ERROR in driverpack_indexinf: malloc(%d)\n",len+2);
            return;
        }
        len=unicode2ansi(adr,t->adr,len);
    }
    else
    {
        t->adr=(char *)malloc(len);
        memmove(t->adr,adr,len);
    }

    t->drp=drp;
    wcscpy(t->pathinf,pathinf);
    wcscpy(t->inffile,inffile);
    t->len=len;
    SetEvent(t->dataready);
}

void driverpack_parsecat(driverpack_t *drp,WCHAR const *pathinf,WCHAR const *inffile,char *adr,int len)
{
    CHAR filename[BUFLEN];
    CHAR bufa[BUFLEN];
    unsigned bufal=0;
    char *p=adr;

    *bufa=0;
    sprintf(filename,"%ws%ws",pathinf,inffile);
    while(p+11<adr+len)
    {
        if(*p=='O'&&!memcmp(p,L"OSAttr",11))
        {
            //sprintf(bufb,"%ws",p+19);
            //if(*bufa&&strcmp(bufa,bufb))log_con("^^");
            if(!*bufa||bufal<wcslen((WCHAR *)(p+19)))
            {
                sprintf(bufa,"%ws",p+19);
                bufal=strlen(bufa);
                //log_con("Found '%s'\n",bufa);
                //break;
            }
        }
        p++;
    }
    if(*bufa)
    {
        strtolower(filename,strlen(filename));
        hash_add(&drp->cat_list,filename,strlen(filename),heap_memcpyz_dup(&drp->text_handle,bufa,strlen(bufa)),HASH_MODE_INTACT);
        //log_con("(%s)\n##%s\n",filename,bufa);
    }
}

#ifdef MERGE_FINDER
int checkfolders(WCHAR *folder1,WCHAR *folder2,hashtable_t *filename2path,hashtable_t *path2filename,int sub)
{
    filedata_t *file1,*file2;
    char bufa[BUFLEN];
    char bufa1[BUFLEN];
    int size=0,sizedif=0,sizeuniq=0;
    int isfound;
    int ismergeable=1;

    //log_con("\n%ws\n%ws\n",folder1,folder2);

    sprintf(bufa,"%ws",folder1);
    file1=(filedata_t *)hash_find(path2filename,bufa,strlen(bufa),&isfound);
    while(file1)
    {

        sprintf(bufa1,"%ws",file1->str);
        file2=(filedata_t *)hash_find(filename2path,bufa1,strlen(bufa1),&isfound);
        //log_con("  [%ws]\n",file1->str);

        sizeuniq+=file1->size;
        while(file2)
        {
            if(!wcscmp(folder2,file2->str))
            {
                sizeuniq-=file1->size;
                if(sub&&file1->crc!=file2->crc)
                    log_con("  %c%ws\t%d\n",file1->crc==file2->crc?'+':'-',file1->str,file1->size);

                if(file1->crc==file2->crc)
                    size+=file1->size;
                else
                {
                    sizedif+=file1->size;
                    ismergeable=0;
                }
            }
            file2=(filedata_t *)hash_findnext(filename2path);
        }

        file1=(filedata_t *)hash_findnext(path2filename);
    }
    if(ismergeable&&sub==0)
    {
        WCHAR folder1d[BUFLEN],folder2d[BUFLEN];
        WCHAR *folder1a=folder1d,*folder2a=folder2d;
        wcscpy(folder1a,folder1);
        wcscpy(folder2a,folder2);
        while(wcschr(folder1a,L'/'))folder1a=wcschr(folder1a,L'/')+1;
        while(wcschr(folder2a,L'/'))folder2a=wcschr(folder2a,L'/')+1;
        folder1a[-1]=0;folder2a[-1]=0;


        log_con("\nrem %ws\nrem %ws\n",folder1,folder2);
        log_con("rem %s (%d,%d,%d)\n",ismergeable?"++++":"----",size/1024,sizedif/1024,sizeuniq/1024);
        int val=checkfolders(folder1d,folder2d,filename2path,path2filename,1);
        log_con("rem %d\n",val);

        folder1a=folder1d;folder2a=folder2d;
        while(wcschr(folder1a,L'/'))*wcschr(folder1a,L'/')=L'\\';
        while(wcschr(folder2a,L'/'))*wcschr(folder2a,L'/')=L'\\';
        log_con("xcopy /S /I /Y /H %ws %ws\n",folder1d,folder1d);
        log_con("xcopy /S /I /Y /H %ws %ws\n",folder2d,folder2d);
        log_con("rd /S /Q %ws\nrd /S /Q %ws\n",folder1d,folder2d);
    }
    if(ismergeable&&!size)return 1;
    return ismergeable?size:0;
}

void hash_clearfiles(hashtable_t *t)
{
    hashitem_t *cur;
    int i=0;

    while(i<t->size)
    {
        cur=&t->items[i++];
        while(1)
        {
            if(t->flags&HASH_FLAG_KEYS_ARE_POINTERS&&cur->key)
            {
                //if(t->id==ID_STRINGS)printf("'%s'\n",cur->key);
                filedata_t *file1;
                file1=(filedata_t *)cur->key;
                //log_con("%x\n",file1);
                //log_con("%ws\n",file1->str);
                free(file1->str);
            }

            if(cur->next<=0)break;
            cur=&t->items[cur->next];
        }
    }
}
#endif

int driverpack_genindex(driverpack_t *drp)
{
    CFileInStream archiveStream;
    CLookToRead lookStream;
    CSzArEx db;
    SRes res;
    ISzAlloc allocImp;
    ISzAlloc allocTempImp;
    UInt16 *temp=NULL;
    size_t tempSize=0;
    unsigned i;

#ifdef MERGE_FINDER
    hashtable_t filename2path;
    hashtable_t path2filename;
    hashtable_t foldercmps;
    filedata_t *filedata;
#endif

    WCHAR name[BUFLEN];
    WCHAR pathinf[BUFLEN];
    WCHAR *inffile;

    log_con("Indexing %ws\\%ws\n",drp->text+drp->drppath,drp->text+drp->drpfilename);
    wsprintf(name,L"%ws\\%ws",drp->text+drp->drppath,drp->text+drp->drpfilename);
    //log("Scanning '%s'\n",name);
    allocImp.Alloc=SzAlloc;
    allocImp.Free=SzFree;
    allocTempImp.Alloc=SzAllocTemp;
    allocTempImp.Free=SzFreeTemp;

    if(InFile_OpenW(&archiveStream.file,name))return 1;

    FileInStream_CreateVTable(&archiveStream);
    LookToRead_CreateVTable(&lookStream,False);
    lookStream.realStream=&archiveStream.s;
    LookToRead_Init(&lookStream);
    CrcGenerateTable();
    SzArEx_Init(&db);

#ifdef MERGE_FINDER
    hash_init(&filename2path,ID_FILES,1024,HASH_FLAG_KEYS_ARE_POINTERS);
    hash_init(&path2filename,ID_FILES,1024,HASH_FLAG_KEYS_ARE_POINTERS);
    hash_init(&foldercmps,ID_FILES,1024,0);
#endif

    res=SzArEx_Open(&db,&lookStream.s,&allocImp,&allocTempImp);
    if(res==SZ_OK)
    {
      /*
      if you need cache, use these 3 variables.
      if you use external function, you can make these variable as static.
      */
        UInt32 blockIndex=0xFFFFFFFF; /* it can have any value before first call (if outBuffer = 0) */
        Byte *outBuffer=0; /* it must be 0 before first call for each new archive. */
        size_t outBufferSize=0;  /* it can have any value before first call (if outBuffer = 0) */

        for(i=0;i<db.db.NumFiles;i++)
        {
            size_t offset=0;
            size_t outSizeProcessed=0;
            const CSzFileItem *f=db.db.Files+i;
            size_t len;
            if (f->IsDir)continue;

            len=SzArEx_GetFileNameUtf16(&db,i,NULL);
            if(len>tempSize)
            {
                SzFree(NULL,temp);
                tempSize=len;
                temp=(UInt16 *)SzAlloc(NULL,tempSize *sizeof(temp[0]));
                if(temp==0)
                {
                    res=SZ_ERROR_MEM;
                    log_err("ERROR mem(%d)\n",tempSize *sizeof(temp[0]));
                    break;
                }
            }
            SzArEx_GetFileNameUtf16(&db,i,temp);

#ifdef MERGE_FINDER
            {
                char bufa[BUFLEN];
                WCHAR *filename=(WCHAR *)temp;
                while(wcschr(filename,L'/'))filename=wcschr(filename,L'/')+1;
                filename[-1]=0;
                //log_con("%8d,%ws\n",f->Size,temp);

                filedata=malloc(sizeof(filedata_t));
                filedata->crc=f->Crc;
                filedata->size=f->Size;
                filedata->str=malloc(wcslen(temp)*2+2);
                wcscpy(filedata->str,temp);
                sprintf(bufa,"%ws",filename);
                hash_add(&filename2path,bufa,strlen(bufa),(int)filedata,HASH_MODE_ADD);
                //log_con("%8d,%08X,[%s],%ws\n",filedata->size,filedata->crc,bufa,filedata->str);

                filedata=malloc(sizeof(filedata_t));
                filedata->crc=f->Crc;
                filedata->size=f->Size;
                filedata->str=malloc(wcslen(filename)*2+2);
                wcscpy(filedata->str,filename);
                sprintf(bufa,"%ws",temp);
                hash_add(&path2filename,bufa,strlen(bufa),(int)filedata,HASH_MODE_ADD);
                //log_con("%8d,%08X,%ws,[%s]\n",filedata->size,filedata->crc,filedata->str,bufa);
            }
#endif

            if(_wcsicmp((WCHAR *)temp+wcslen((WCHAR *)temp)-4,L".inf")==0)
            {
                WCHAR *ii=(WCHAR *)temp;
                while(*ii)
                {
                    if(*ii=='/')*ii='\\';
                    ii++;
                }
                res = SzArEx_Extract(&db,&lookStream.s,i,
                    &blockIndex,&outBuffer,&outBufferSize,
                    &offset,&outSizeProcessed,
                    &allocImp,&allocTempImp);
                if(res!=SZ_OK)continue;


                inffile=(WCHAR *)temp;
                while(*inffile++);inffile--;
                while(inffile!=(WCHAR *)temp&&*inffile!='\\')inffile--;
                if(*inffile=='\\'){*inffile++=0;}
                wsprintf(pathinf,L"%ws\\",temp);
//                log("%10ld, %10ld, Openning '%ws%ws'\n",offset,outSizeProcessed,pathinf,inffile);

//                driverpack_indexinf(drp,pathinf,inffile,(char *)(outBuffer+offset),f->Size);
                driverpack_indexinf_async(drp,drp->col,pathinf,inffile,(char *)(outBuffer+offset),outSizeProcessed);
            }
            if(_wcsicmp((WCHAR *)temp+wcslen((WCHAR *)temp)-4,L".cat")==0)
            {
                WCHAR *ii=(WCHAR *)temp;
                while(*ii)
                {
                    if(*ii=='/')*ii='\\';
                    ii++;
                }
                res = SzArEx_Extract(&db,&lookStream.s,i,
                    &blockIndex,&outBuffer,&outBufferSize,
                    &offset,&outSizeProcessed,
                    &allocImp,&allocTempImp);
                if(res!=SZ_OK)continue;

                inffile=(WCHAR *)temp;
                while(*inffile++);inffile--;
                while(inffile!=(WCHAR *)temp&&*inffile!='\\')inffile--;
                if(*inffile=='\\'){*inffile++=0;}
                wsprintf(pathinf,L"%ws\\",temp);
                driverpack_parsecat(drp,pathinf,inffile,(char *)(outBuffer+offset),outSizeProcessed);
            }
        }
#ifdef MERGE_FINDER
        for(i=0;i<db.db.NumFiles;i++)
        {
            char bufa[BUFLEN];
            char bufa1[BUFLEN];
            int isfound;
            SzArEx_GetFileNameUtf16(&db,i,temp);
            const CSzFileItem *f=db.db.Files+i;

            WCHAR *filename=(WCHAR *)temp;
            while(wcschr(filename,L'/'))filename=wcschr(filename,L'/')+1;
            filename[-1]=0;

            //log_con("%ws,%ws\n",temp,filename);
            sprintf(bufa,"%ws",filename);
            filedata=(filedata_t *)hash_find(&filename2path,bufa,strlen(bufa),&isfound);
            while(filedata)
            {
                sprintf(bufa1,"%ws - %ws",temp,filedata->str);
                hash_find(&foldercmps,bufa1,strlen(bufa1),&isfound);

                if(f->Crc==filedata->crc&&wcscmp(temp,filedata->str)&&!isfound)
                {
                    checkfolders(temp,filedata->str,&filename2path,&path2filename,0);
                    //log_con("  %ws\n",filedata->str);
                }

                sprintf(bufa1,"%ws - %ws",temp,filedata->str);
                hash_add(&foldercmps,bufa1,strlen(bufa1),1,HASH_MODE_INTACT);

                sprintf(bufa1,"%ws - %ws",filedata->str,temp);
                hash_add(&foldercmps,bufa1,strlen(bufa1),1,HASH_MODE_INTACT);

                filedata=(filedata_t *)hash_findnext(&filename2path);
            }
        }
#endif

        IAlloc_Free(&allocImp,outBuffer);
    }
    SzArEx_Free(&db,&allocImp);
    SzFree(NULL,temp);
    File_Close(&archiveStream.file);

#ifdef MERGE_FINDER
    hash_clearfiles(&filename2path);
    hash_clearfiles(&path2filename);
    hash_free(&filename2path);
    hash_free(&path2filename);
    hash_free(&foldercmps);
#endif

    driverpack_indexinf_async(drp,drp->col,L"",L"",0,0);
    return 1;
}

void driverpack_indexinf(driverpack_t *drp,WCHAR const *drpdir,WCHAR const *inffile,char *bb,int inf_len)
{
    if(inf_len>4&&((bb[0]==-1&&bb[3]==0)||bb[0]==0))
    {
        char *buf_out;
        int size=inf_len;

        buf_out=(char *)malloc(size+2);
        if(!buf_out)
        {
            log_err("ERROR in driverpack_indexinf: malloc(%d)\n",size+2);
            return;
        }
        size=unicode2ansi(bb,buf_out,size);
        driverpack_indexinf_ansi(drp,drpdir,inffile,buf_out,size);
        free(buf_out);
    }
    else
    {
        driverpack_indexinf_ansi(drp,drpdir,inffile,bb,inf_len);
    }
}
// http://msdn.microsoft.com/en-us/library/ff547485(v=VS.85).aspx
void driverpack_indexinf_ansi(driverpack_t *drp,WCHAR const *drpdir,WCHAR const *inffile,char *inf_base,int inf_len)
{
    version_t *cur_ver;
    sect_data_t strlink;

    int cur_inffile_index;
    data_inffile_t *cur_inffile;
    int cur_manuf_index;
    data_manufacturer_t *cur_manuf;
    int cur_desc_index;
    data_desc_t * cur_desc;
    int cur_HWID_index;
    data_HWID_t *cur_HWID;

    char date_s[256];
    char build_s[256];
    char secttry[256];
    char line[2048];
    int  strs[16];

    parse_info_t parse_info,parse_info2;
    parse_info_t parse_info3;
    sect_data_t *lnk,*lnk2;

    hashtable_t *string_list=&drp->string_list;
    hashtable_t *section_list=&drp->section_list;

    parse_init(&parse_info,drp);
    parse_init(&parse_info2,drp);
    parse_init(&parse_info3,drp);

    char *p=inf_base,*strend=inf_base+inf_len;
    char *p2,*sectnmend;
    char *lnk_s=0;

    cur_inffile_index=heap_allocitem_i(&drp->inffile_handle);
    cur_inffile=&drp->inffile[cur_inffile_index];
    sprintf(line,"%ws",drpdir);
    cur_inffile->infpath=heap_strcpy(&drp->text_handle,line);
    sprintf(line,"%ws",inffile);
    cur_inffile->inffilename=heap_strcpy(&drp->text_handle,line);
    cur_inffile->infsize=inf_len;
    cur_inffile->infcrc=0;
    //log("%ws%ws\n",drpdir,inffile);

    // Populate sections
    //char *b;
    while(p<strend)
    {
        switch(*p)
        {
            case ' ':case '\t':case '\n':case '\r':
                p++;
                break;

            case ';':
                p++;
                while(p<strend&&*p!='\n'&&*p!='\r')p++;
                break;

            case '[':
                if(lnk_s)
                    ((sect_data_t *)lnk_s)->len=p-inf_base;
#ifdef DEBUG_EXTRACHECKS
/*					char *strings_base=inf_base+(*it).second.ofs;
					int strings_len=(*it).second.len-(*it).second.ofs;
					if(*(strings_base-1)!=']')
						log("B'%.*s'\n",1,strings_base-1);
					if(*(strings_base+strings_len)!='[')
						log("E'%.*s'\n",1,strings_base+strings_len);*/
#endif
                p++;
                p2=p;

                while(*p2!=']'&&p2<strend)
                {
#ifdef DEBUG_EXTRACHECKS
                    if(*p2=='\\')log("Err \\\n");else
                    if(*p2=='"')log("Err \"\n");else
                    if(*p2=='%')log("Err %\n");else
                    if(*p2==';')log("Err ;\n");
#endif
                    cur_inffile->infcrc+=*p2++;
                }
                sectnmend=p2;
                p2++;

                strlink.ofs=p2-inf_base;
                strlink.len=inf_len;
                {
                    lnk_s=memcpy_alloc((char *)(&strlink),sizeof(sect_data_t));
                    hash_add(section_list,p,sectnmend-p,(int)lnk_s,HASH_MODE_ADD);
                }
                p=p2;
                break;

            default:
                //b=p;
                //while(*p!='\n'&&*p!='\r'&&*p!='['&&p<strend)p++;
                //if(*p=='['&&p<strend)log("ERROR in %ws%ws:\t\t\t'%.*s'(%d/%d)\n",drpdir,inffile,p-b+20,b,p,strend);
                while(p<strend&&*p!='\n'&&*p!='\r'/*&&*p!='['*/)cur_inffile->infcrc+=*p++;
        }
    }

    // Find [strings]
    lnk=(sect_data_t *)hash_find_str(section_list,"strings");
    if(!lnk)log_index("ERROR: missing [strings] in %ws%ws\n",drpdir,inffile);
    while(lnk)
    {
        char *s1b,*s1e,*s2b,*s2e;

        parse_set(&parse_info,inf_base,lnk);
        while(read_item(&parse_info))
        {
            parse_getstr(&parse_info,&s1b,&s1e);
            read_field(&parse_info);
            parse_getstr(&parse_info,&s2b,&s2e);
            hash_add(string_list,s1b,s1e-s1b,(int)memcpy_alloc(s2b,s2e-s2b),HASH_MODE_INTACT);
        }
        lnk=(sect_data_t *)hash_findnext(section_list);
        if(lnk)log_index("NOTE: multiple [strings] in %ws%ws\n",drpdir,inffile);
    }

    // Find [version]
    date_s[0]=0;
    build_s[0]=0;
    cur_ver=&cur_inffile->version;
    cur_ver->v1=-1;
    cur_ver->y=-1;

    lnk=(sect_data_t *)hash_find_str(section_list,"version");
    if(!lnk)log_index("ERROR: missing [version] in %ws%ws\n",drpdir,inffile);
    while(lnk)
    {
        char *s1b,*s1e;

        parse_set(&parse_info,inf_base,lnk);
        while(read_item(&parse_info))
        {
            parse_getstr(&parse_info,&s1b,&s1e);
            strtolower(s1b,s1e-s1b);

            int i,sz=s1e-s1b;
            for(i=0;i<NUM_VER_NAMES;i++)
            if(table_version[i].sz==sz&&!memcmp(s1b,table_version[i].s,sz))
            {
                if(i==DriverVer)
                {
                        // date
                        read_field(&parse_info);
                        i=read_date(&parse_info,cur_ver);
                        if(i)log_index("ERROR: invalid date(%d.%d.%d)[%d] in %ws%ws\n",
                                 cur_ver->d,cur_ver->m,cur_ver->y,i,drpdir,inffile);

                        sprintf(date_s,"%02d/%02d/%04d",cur_ver->m,cur_ver->d,cur_ver->y);

                        // version
                        if(read_field(&parse_info))
                        {
                            read_version(&parse_info,cur_ver);
                        }
                        sprintf(build_s,"%d.%d.%d.%d",cur_ver->v1,cur_ver->v2,cur_ver->v3,cur_ver->v4);

                }else
                {
                    read_field(&parse_info);
                    parse_getstr(&parse_info,&s1b,&s1e);
                    cur_inffile->fields[i]=heap_memcpyz(&drp->text_handle,s1b,s1e-s1b);
                }
                break;
            }
            if(i==NUM_VER_NAMES)
            {
                s1e=parse_info.se;
                //log("QQ '%.*s'\n",s1e-s1b,s1b);
            }
            while(read_field(&parse_info));
        }
        lnk=(sect_data_t *)hash_findnext(section_list);
        if(lnk)log_index("NOTE:  multiple [version] in %ws%ws\n",drpdir,inffile);
        if(!lnk)
        {
            if(cur_ver->y==-1) log_index("ERROR: missing date in %ws%ws\n",drpdir,inffile);
            if(cur_ver->v1==-1)log_index("ERROR: missing build number in %ws%ws\n",drpdir,inffile);
        }
    }

    // Find [manufacturer] section
    lnk=(sect_data_t *)hash_find_str(section_list,"manufacturer");
    if(!lnk)log_index("ERROR: missing [manufacturer] in %ws%ws\n",drpdir,inffile);
    while(lnk)
    {
        parse_set(&parse_info,inf_base,lnk);
        while(read_item(&parse_info))
        {
            char *s1b,*s1e;
            parse_getstr(&parse_info,&s1b,&s1e);

            cur_manuf_index=heap_allocitem_i(&drp->manufacturer_handle);
            cur_manuf=&drp->manufacturer_list[cur_manuf_index];
            cur_manuf->inffile_index=cur_inffile_index;
            cur_manuf->manufacturer=heap_memcpyz(&drp->text_handle,s1b,s1e-s1b);
            cur_manuf->sections_n=0;

            if(read_field(&parse_info))
            {
                parse_getstr(&parse_info,&s1b,&s1e);
                strtolower(s1b,s1e-s1b);
                strs[cur_manuf->sections_n++]=heap_memcpyz(&drp->text_handle,s1b,s1e-s1b);
                while(1)
                {
                    find_t savedfind0;
                    if(cur_manuf->sections_n>1)
                        sprintf(secttry,"%s.%s",
                                drp->text+strs[0],
                                drp->text+strs[cur_manuf->sections_n-1]);
                    else
                        sprintf(secttry,"%s",drp->text+strs[0]);

                    memcpy(&savedfind0,&section_list->finddata,sizeof(find_t));
                    lnk2=(sect_data_t *)hash_find_str(section_list,secttry);
                    if(!lnk2&&cur_manuf->sections_n>1)log_index("ERROR: missing [%s] in %ws%ws\n",secttry,drpdir,inffile);
                    while(lnk2)
                    {
                        parse_set(&parse_info2,inf_base,lnk2);
                        while(read_item(&parse_info2))
                        {
                            parse_getstr(&parse_info2,&s1b,&s1e);

                            cur_desc_index=heap_allocitem_i(&drp->desc_list_handle);
                            cur_desc=&drp->desc_list[cur_desc_index];
                            cur_desc->manufacturer_index=cur_manuf_index;
                            cur_desc->sect_pos=drp->manufacturer_list[cur_manuf_index].sections_n-1;
                            cur_desc->desc=heap_memcpyz_dup(&drp->text_handle,s1b,s1e-s1b);

                            //{ featurescore
                            cur_desc->feature=0xFF;
                            char installsection[4096];
                            sect_data_t *lnk3;
                            //parse_info3.pack=drp;
                            find_t savedfind;

                            read_field(&parse_info2);
                            parse_getstr(&parse_info2,&s1b,&s1e);
                            cur_desc->install=heap_memcpyz_dup(&drp->text_handle,s1b,s1e-s1b);

                            memcpy(&savedfind,&section_list->finddata,sizeof(find_t));

                            sprintf(installsection,"%.*s.nt",s1e-s1b,s1b);
                            strtolower(installsection,strlen(installsection));
                            lnk3=(sect_data_t *)hash_find_str(section_list,installsection);

                            if(!lnk3)
                            {
                                sprintf(installsection,"%.*s",s1e-s1b,s1b);
                                strtolower(installsection,strlen(installsection));
                                lnk3=(sect_data_t *)hash_find_str(section_list,installsection);
                            }
                            if(!lnk3)
                            {
                                if(cur_manuf->sections_n>1)
                                    sprintf(installsection,"%.*s.%s",s1e-s1b,s1b,drp->text+strs[cur_manuf->sections_n-1]);
                                else
                                    sprintf(installsection,"%.*s",s1e-s1b,s1b);

                                strtolower(installsection,strlen(installsection));
                                while(strlen(installsection)>=(unsigned)(s1e-s1b))
                                {
                                    lnk3=(sect_data_t *)hash_find_str(section_list,installsection);
                                    if(lnk3)break;
                                    //log("Tried '%s'\n",installsection);
                                    installsection[strlen(installsection)-1]=0;
                                }
                            }
                            char iii[4096];
                            *iii=0;
                            //int cnt=0;
                            sect_data_t *tlnk;
                            if(!lnk3)
                            {
                                int i;
                                for(i=0;i<NUM_DECS;i++)
                                {
                                    sprintf(installsection,"%.*s.%s",s1e-s1b,s1b,nts[i]);
                                    strtolower(installsection,strlen(installsection));
                                    tlnk=(sect_data_t *)hash_find_str(section_list,installsection);
                                    if(tlnk&&!lnk3)lnk3=tlnk;
                                    if(tlnk)
                                    {
                                        strcat(iii,installsection);
                                        strcat(iii,",");
                                    }
                                    //if(lnk3){log("Found '%s'\n",installsection);cnt++;}
                                }
                            }
                            //if(cnt>1)log("@num: %d\n",cnt);
                            //if(cnt>1&&!lnk3)log("ERROR in %ws%ws:\t\t\tMissing [%s]\n",drpdir,inffile,iii);
                            if(lnk3)
                            {
                                if(*iii)sprintf(installsection,"$%s",iii);
                                cur_desc->install_picked=heap_memcpyz_dup(&drp->text_handle,installsection,strlen(installsection));
                            }
                            else
                            {
                                cur_desc->install_picked=heap_memcpyz_dup(&drp->text_handle,"{missing}",9);
                            }

                            while(lnk3)
                            {
                                parse_info3.start=inf_base+lnk3->ofs;
                                parse_info3.strend=inf_base+lnk3->len;
                                if(!strcmp(secttry,installsection))
                                {
                                    log_index("ERROR: [%s] refers to itself in %ws%ws\n",installsection,drpdir,inffile);
                                    break;
                                }

                                while(read_item(&parse_info3))
                                {
                                    s1b=parse_info3.sb;
                                    s1e=parse_info3.se;
                                    strtolower(s1b,s1e-s1b);
                                    int sz=s1e-s1b;
                                    if(sz==12&&!memcmp(s1b,"featurescore",sz))
                                    {
                                        read_field(&parse_info3);
                                        cur_desc->feature=read_hex(&parse_info3);
                                    }
                                    while(read_field(&parse_info3));
                                }
                                lnk3=(sect_data_t *)hash_findnext(section_list);
                            }
                            memcpy(&section_list->finddata,&savedfind,sizeof(find_t));
                            //} feature

                            int hwid_pos=0;
                            while(read_field(&parse_info2))
                            {
                                parse_getstr(&parse_info2,&s1b,&s1e);
                                if(s1b>=s1e)continue;
                                strtoupper(s1b,s1e-s1b);

                                cur_HWID_index=heap_allocitem_i(&drp->HWID_list_handle);
                                cur_HWID=&drp->HWID_list[cur_HWID_index];
                                cur_HWID->desc_index=cur_desc_index;
                                cur_HWID->HWID=heap_memcpyz_dup(&drp->text_handle,s1b,s1e-s1b);
                                cur_HWID->inf_pos=hwid_pos++;

                                /*sprintf(line,"%s%s",drp->text+cur_HWID->HWID,drp->text+cur_desc->desc);
                                if(drp->fi&&!hash_find_str(dup_list,line))
                                {
                                    hash_add(dup_list,line,strlen(line),1,HASH_MODE_INTACT);
                                    trap_mode=0;
                                }*/
                            }
                        }
                        lnk2=(sect_data_t *)hash_findnext(section_list);
                        if(lnk2)log_index("NOTE:  multiple [%s] in %ws%ws\n",secttry,drpdir,inffile);
                    }
                    memcpy(&section_list->finddata,&savedfind0,sizeof(find_t));

                    if(!read_field(&parse_info))break;
                    parse_getstr(&parse_info,&s1b,&s1e);
                    if(s1b>s1e)break;
                    strtolower(s1b,s1e-s1b);
                    strs[cur_manuf->sections_n++]=heap_memcpyz(&drp->text_handle,s1b,s1e-s1b);
                }
            }
            cur_manuf->sections=heap_memcpyz(&drp->text_handle,strs,sizeof(int)*cur_manuf->sections_n);

        }
        lnk=(sect_data_t *)hash_findnext(section_list);
        if(lnk)log_index("NOTE:  multiple [manufacturer] in %ws%ws\n",drpdir,inffile);
    }
    //hash_stats(string_list);
    //hash_stats(section_list);
    //hash_stats(dup_list);

    parse_free(&parse_info);
    parse_free(&parse_info2);
    parse_free(&parse_info3);

    hash_clear(string_list,1);
    hash_clear(section_list,1);
}
//}
