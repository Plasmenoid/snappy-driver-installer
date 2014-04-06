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

//{ Global variables
panelitem_t panelitems[PANELITEMS_NUM]=
{
    {TYPE_GROUP,0,3,0},
    {TYPE_TEXT,STR_SHOW_SYSINFO,0,0},
    {TYPE_TEXT,0,0,0},
    {TYPE_TEXT,0,0,0},

    {TYPE_GROUP,0,3,0},
    {TYPE_BUTTON,STR_INSTALL,               ID_INSTALL,0},
    {TYPE_BUTTON,STR_SELECT_ALL,            ID_SELECT_ALL,0},
    {TYPE_BUTTON,STR_SELECT_NONE,           ID_SELECT_NONE,0},

    {TYPE_GROUP,0,5,0},
    {TYPE_TEXT,STR_LANG,0,0},
    {TYPE_TEXT,0,0,0},
    {TYPE_TEXT,STR_THEME,0,0},
    {TYPE_TEXT,0,0,0},
    {TYPE_CHECKBOX,STR_EXPERT,         ID_EXPERT_MODE,0},

    {TYPE_GROUP_BREAK,0,4,0},
    {TYPE_BUTTON,STR_OPENLOGS,              ID_OPENLOGS,0},
    {TYPE_BUTTON,STR_SNAPSHOT,              ID_SNAPSHOT,0},
    {TYPE_BUTTON,STR_EXTRACT,               ID_EXTRACT,0},
    {TYPE_BUTTON,STR_DRVDIR,                ID_DRVDIR,0},

    {TYPE_GROUP,0,7,0},
    {TYPE_TEXT,STR_SHOW_FOUND,0,0},
    {TYPE_CHECKBOX, STR_SHOW_MISSING,       ID_SHOW_MISSING,0},
    {TYPE_CHECKBOX, STR_SHOW_NEWER,         ID_SHOW_NEWER,0},
    {TYPE_CHECKBOX, STR_SHOW_CURRENT,       ID_SHOW_CURRENT,0},
    {TYPE_CHECKBOX, STR_SHOW_OLD,           ID_SHOW_OLD,0},
    {TYPE_CHECKBOX, STR_SHOW_BETTER,        ID_SHOW_BETTER,0},
    {TYPE_CHECKBOX, STR_SHOW_WORSE_RANK,    ID_SHOW_WORSE_RANK,0},

    {TYPE_GROUP,0,4,0},
    {TYPE_TEXT,STR_SHOW_NOTFOUND,0,0},
    {TYPE_CHECKBOX, STR_SHOW_NF_MISSING,    ID_SHOW_NF_MISSING,0},
    {TYPE_CHECKBOX, STR_SHOW_NF_UNKNOWN,    ID_SHOW_NF_UNKNOWN,0},
    {TYPE_CHECKBOX, STR_SHOW_NF_STANDARD,   ID_SHOW_NF_STANDARD,0},

    {TYPE_GROUP,0,3,0},
    {TYPE_CHECKBOX, STR_SHOW_ONE,           ID_SHOW_ONE,0},
    {TYPE_CHECKBOX, STR_SHOW_DUP,           ID_SHOW_DUP,0},
    {TYPE_CHECKBOX, STR_SHOW_INVALID,       ID_SHOW_INVALID,0},

    {0,0,0,0}
};

// Manager
manager_t manager_v[2];
manager_t *manager_g=&manager_v[0];
int manager_active=0;
int bundle_display=1;
int bundle_shadow=0;
int volatile installmode=MODE_NONE;
CRITICAL_SECTION sync;

// Window
HINSTANCE ghInst;
MSG msg;
HFONT hFont=0;
canvas_t canvasMain;
canvas_t canvasField;
canvas_t canvasPopup;
const WCHAR classMain[]= L"classMain";
const WCHAR classField[]=L"classField";
const WCHAR classPopup[]=L"classPopup";
HWND hMain=0;
HWND hField=0;
HWND hPopup=0;
HWND hLang=0;
HWND hTheme=0;

// Window helpers
int panel_lasti;
int field_lasti,field_lastz;
int mainx_c,mainy_c;
int mainx_w,mainy_w;
int mousex=-1,mousey=-1,mousedown=0,mouseclick=0;
int cntd=0;
int hideconsole=SW_HIDE;

int ctrl_down=0;
int space_down=0;
int floating_type=0;
int floating_itembar=-1;
int floating_x=1,floating_y=1;
int horiz_sh=0;

int exitflag=0;
int ret_global;
HANDLE event;

// Settings
WCHAR drp_dir   [BUFLEN]=L"drivers";
WCHAR drpext_dir[BUFLEN]=L"";
WCHAR index_dir [BUFLEN]=L"indexes\\SDI";
WCHAR output_dir[BUFLEN]=L"indexes\\SDI\\txt";
WCHAR data_dir  [BUFLEN]=L"tools\\SDI";
WCHAR logO_dir  [BUFLEN]=L"logs";
WCHAR log_dir   [BUFLEN];

WCHAR state_file[BUFLEN]=L"untitled.snp";
WCHAR finish    [BUFLEN]=L"";
WCHAR finish_rb [BUFLEN]=L"";

int flags=0;
int statemode=0;
int expertmode=0;
int license=0;
WCHAR curlang [BUFLEN]=L"";
WCHAR curtheme[BUFLEN]=L"(default)";
int filters=
    (1<<ID_SHOW_MISSING)+
    (1<<ID_SHOW_NEWER)+
    (1<<ID_SHOW_BETTER)+
    (1<<ID_SHOW_NF_MISSING)+
    (1<<ID_SHOW_ONE);
int virtual_os_version=0;
int virtual_arch_type=0;

#define NUM_OS 7
const WCHAR *windows_name[NUM_OS]=
{
    L"Windows 2000",
    L"Windows XP",
    L"Windows Vista",
    L"Windows 7",
    L"Windows 8",
    L"Windows 8.1",
    L"Unknown OS"
};
int windows_ver[NUM_OS]={50,51,60,61,62,63,0};
//}

//{ Main
void settings_parse(const WCHAR *str,int ind)
{
    WCHAR buf[BUFLEN];
    WCHAR **argv,*pr;
    int argc;
    int i;

    argv=CommandLineToArgvW(str,&argc);
    for(i=ind;i<argc;i++)
    {
        pr=argv[i];
        if (pr[0] == '/') pr[0]='-';
        if( wcsstr(pr,L"-drp_dir:"))     wcscpy(drp_dir,pr+9);else
        if( wcsstr(pr,L"-index_dir:"))   wcscpy(index_dir,pr+11);else
        if( wcsstr(pr,L"-output_dir:"))  wcscpy(output_dir,pr+12);else
        if( wcsstr(pr,L"-data_dir:"))    wcscpy(data_dir,pr+10);else
        if( wcsstr(pr,L"-log_dir:"))     {wcscpy(logO_dir,pr+9);
        ExpandEnvironmentStrings(logO_dir,log_dir,BUFLEN);
        }else
        if( wcsstr(pr,L"-finish_cmd:"))  wcscpy(finish,pr+12);else
        if( wcsstr(pr,L"-finishrb_cmd:"))wcscpy(finish_rb,pr+14);else
        if( wcsstr(pr,L"-lang:"))        wcscpy(curlang,pr+6);else
        if( wcsstr(pr,L"-theme:"))       wcscpy(curtheme,pr+7);else
        if(!wcscmp(pr,L"-expertmode"))   expertmode=1;else
        if( wcsstr(pr,L"-filters:"))     filters=_wtoi(pr+9);else
        if(!wcscmp(pr,L"-license"))      license=1;else
        if(!wcscmp(pr,L"-norestorepnt")) flags|=FLAG_NORESTOREPOINT;else
        if(!wcscmp(pr,L"-nofeaturescore"))flags|=FLAG_NOFEATURESCORE;else
        if(!wcscmp(pr,L"-showdrpnames")) flags|=FLAG_SHOWDRPNAMES;else
        if(!wcscmp(pr,L"-preservecfg"))  flags|=FLAG_PRESERVECFG;else
        if(!wcscmp(pr,L"-7z"))
        {
            WCHAR cmd[BUFLEN];
            wsprintf(cmd,L"7za %s",wcsstr(GetCommandLineW(),L"-7z")+4);
            log_con("Executing '%ws'\n",cmd);
            registerall();
            statemode=STATEMODE_EXIT;
            ret_global=Extract7z(cmd);
            log_con("Ret: %d\n",ret_global);
            return;
        }
        else
        if(!wcscmp(pr,L"-install")&&argc-i==3)
        {
            log_con("Install '%ws' '%s'\n",argv[i+1],argv[i+2]);
            GetEnvironmentVariable(L"TEMP",buf,BUFLEN);
            wsprintf(extractdir,L"%s\\SDI",buf);
            installmode=MODE_INSTALLING;
            driver_install(argv[i+1],argv[i+2],&ret_global,&needreboot);
            log_con("Ret: %X,%d\n",ret_global,needreboot);
            if(needreboot)ret_global|=0x80000000;
            wsprintf(buf,L" /c rd /s /q \"%s\"",extractdir);
            RunSilent(L"cmd",buf,SW_HIDE,1);
            statemode=STATEMODE_EXIT;
            return;
        }
        else
        if(!wcscmp(pr,L"-reindex"))      flags|=COLLECTION_FORCE_REINDEXING;else
        if(!wcscmp(pr,L"-index_hr"))     flags|=COLLECTION_PRINT_INDEX;else
        if(!wcscmp(pr,L"-lzma"))         flags|=COLLECTION_USE_LZMA;else
        if(!wcscmp(pr,L"-nogui"))        flags|=FLAG_NOGUI|FLAG_AUTOCLOSE;else
        if(!wcscmp(pr,L"-noslowsysinfo"))flags|=FLAG_NOSLOWSYSINFO;else
        if(!wcscmp(pr,L"-autoinstall"))  flags|=FLAG_AUTOINSTALL;else
        if(!wcscmp(pr,L"-autoclose"))    flags|=FLAG_AUTOCLOSE;else
        if(!wcscmp(pr,L"-nologfile"))    flags|=FLAG_NOLOGFILE;else
        if(!wcscmp(pr,L"-nosnapshot"))   flags|=FLAG_NOSNAPSHOT;else
        if(!wcscmp(pr,L"-nostamp"))      flags|=FLAG_NOSTAMP;else
        if( wcsstr(pr,L"-extractdir:"))  {flags|=FLAG_EXTRACTONLY;wcscpy(extractdir,pr+12);}else
        if(!wcscmp(pr,L"-keepunpackedindex"))flags|=FLAG_KEEPUNPACKINDEX;else
        if(!wcscmp(pr,L"-keeptempfiles"))flags|=FLAG_KEEPTEMPFILES;else
        if(!wcscmp(pr,L"-disableinstall"))flags|=FLAG_DISABLEINSTALL;else
        if(!wcscmp(pr,L"-failsafe"))     flags|=FLAG_FAILSAFE;else
        if( wcsstr(pr,L"-verbose:"))     log_verbose=_wtoi(pr+9);else
        if( wcsstr(pr,L"-ls:"))          {wcscpy(state_file,pr+4);statemode=STATEMODE_LOAD;}else
        if(!wcscmp(pr,L"-a:32"))         virtual_arch_type=32;else
        if(!wcscmp(pr,L"-a:64"))         virtual_arch_type=64;else
        if( wcsstr(pr,L"-v:"))           virtual_os_version=_wtoi(pr+3);else
        if(_wcsnicmp(pr,SAVE_INSTALLED_ID_DEF,wcslen(SAVE_INSTALLED_ID_DEF))==0)
            Parse_save_installed_id_swith(pr);else
        if(wcsstr(pr,L"-?"))CLIParam.ShowHelp=TRUE;else
        if(_wcsnicmp(pr,HWIDINSTALLED_DEF,wcslen(HWIDINSTALLED_DEF))==0)
            Parse_HWID_installed_swith(pr); else
        if(_wcsnicmp(pr,GFG_DEF,wcslen(GFG_DEF))==0)  continue;
        else
            log_err("Unknown argument '%ws'\n",pr);
       if(statemode==STATEMODE_EXIT)break;
    }
    LocalFree(argv);
    if(statemode==STATEMODE_EXIT)return;
    // Expert mode
    panelitems[13].checked=expertmode;

    // Left panel
    for(i=0;i<PANELITEMS_NUM;i++)
        if(panelitems[i].action_id>=ID_SHOW_MISSING&&panelitems[i].action_id<=ID_SHOW_INVALID)
            panelitems[i].checked=filters&(1<<panelitems[i].action_id)?1:0;
}

void settings_save()
{
    FILE *f;

    if(flags&FLAG_PRESERVECFG)return;
    if(!canWrite(L"settings.cfg"))
    {
        log_err("ERROR in settings_save(): Write-protected,'settings.cfg'\n");
        return;
    }
    f=_wfopen(L"settings.cfg",L"wt");
    if(!f)return;
    fwprintf(f,L"\"-drp_dir:%s\" \"-index_dir:%s\" \"-output_dir:%s\" "
              "\"-data_dir:%s\" \"-log_dir:%s\" "
              "\"-finish_cmd:%s\" \"-finishrb_cmd:%s\" "
              "-filters:%d \"-lang:%s\" \"-theme:%s\" ",
            drp_dir,index_dir,output_dir,
            data_dir,logO_dir,
            finish,finish_rb,
            filters,curlang,curtheme);

    if(license)fwprintf(f,L"-license ");
    if(expertmode)fwprintf(f,L"-expertmode ");
    if(flags&FLAG_NORESTOREPOINT)fwprintf(f,L"-norestorepnt ");
    if(flags&FLAG_NOFEATURESCORE)fwprintf(f,L"-nofeaturescore ");
    if(flags&FLAG_SHOWDRPNAMES)fwprintf(f,L"-showdrpnames ");
    fclose(f);
}

int  settings_load(WCHAR *filename)
{
    FILE *f;
    WCHAR buf[BUFLEN];

    f=_wfopen(filename,L"rt");
    if(!f)return 0;
    fgetws(buf,BUFLEN,f);
    settings_parse(buf,0);
    fclose(f);
    return 1;
}

void SignalHandler(int signum)
{
    switch (signum)
    {
    case SIGSEGV:
        log_err("!!! Crashed !!!\n");
        log_stop();
        break;
    default:
        break;
    }
}

void CALLBACK drp_callback(LPTSTR szFile,DWORD action,LPARAM lParam)
{
    UNREFERENCED_PARAMETER(action);
    UNREFERENCED_PARAMETER(lParam);

    if(StrStrIW(szFile,L".7z")||StrStrIW(szFile,L".inf")){}
    //SetEvent(event);
}

int WINAPI WinMain(HINSTANCE hInst,HINSTANCE hinst,LPSTR pStr,int nCmd)
{
    UNREFERENCED_PARAMETER(hinst);
    UNREFERENCED_PARAMETER(pStr);

    bundle_t bundle[2];
    monitor_t mon_drp;
    HANDLE thr;
    HMODULE backtrace;
    DWORD dwProcessId;

    GetWindowThreadProcessId(GetConsoleWindow(),&dwProcessId);
    if(GetCurrentProcessId()!=dwProcessId)hideconsole=SW_SHOWNOACTIVATE;
    ShowWindow(GetConsoleWindow(),hideconsole);

    time_startup=time_total=GetTickCount();
    backtrace=LoadLibraryA("backtrace.dll");
    ghInst=hInst;
    init_CLIParam();
    if (isCfgSwithExist(GetCommandLineW(),CLIParam.SaveInstalledFileName))
    {
      WCHAR buff[BUFLEN];
      LoadCFGFile(CLIParam.SaveInstalledFileName, buff);
      settings_parse(buff,0);
      //CLIParam.SaveInstalledFileName[0] = '\0';

    }
    else
    if(!settings_load(L"settings.cfg"))
        settings_load(L"tools\\SDI\\settings.cfg");

    settings_parse(GetCommandLineW(),1);

#ifdef CONSOLE_MODE
    flags|=FLAG_NOGUI;
    license=1;
    wcscpy(drp_dir,log_dir);
    wcscpy(index_dir,log_dir);
    wcscpy(output_dir,log_dir);
#endif
    ExpandEnvironmentStrings(logO_dir,log_dir,BUFLEN);
    if(statemode==STATEMODE_EXIT)
    {
        if(backtrace)FreeLibrary(backtrace);
        ShowWindow(GetConsoleWindow(),SW_SHOW);
        return ret_global;
    }
    log_start(log_dir);
    RUN_CLI(CLIParam);

    if(statemode==STATEMODE_EXIT)
    {
        if(backtrace)FreeLibrary(backtrace);
        ShowWindow(GetConsoleWindow(),SW_SHOW);
        return ret_global;
    }
    //signal(SIGSEGV,SignalHandler);
#ifndef CONSOLE_MODE
    ShowWindow(GetConsoleWindow(),expertmode?SW_SHOWNOACTIVATE:hideconsole);
#endif

    if(log_verbose&LOG_VERBOSE_ARGS)
    {
        log_con("Settings\n");
        log_con("  drp_dir='%ws'\n",drp_dir);
        log_con("  index_dir='%ws'\n",index_dir);
        log_con("  output_dir='%ws'\n",output_dir);
        log_con("  data_dir='%ws'\n",data_dir);
        log_con("  log_dir='%ws'\n",log_dir);
        log_con("  extractdir='%ws'\n",extractdir);
        log_con("  lang=%ws\n",curlang);
        log_con("  theme=%ws\n",curtheme);
        log_con("  expertmode=%d\n",expertmode);
        log_con("  filters=%d\n",filters);
        log_con("  autoinstall=%d\n",flags&FLAG_AUTOINSTALL?1:0);
        log_con("  autoclose=%d\n",flags&FLAG_AUTOCLOSE?1:0);
        log_con("  failsafe=%d\n",flags&FLAG_FAILSAFE?1:0);
        log_con("  norestorepnt=%d\n",flags&FLAG_NORESTOREPOINT?1:0);
        log_con("  disableinstall=%d\n",flags&FLAG_DISABLEINSTALL?1:0);
        log("\n");
        if(*state_file&&statemode)log_con("Virtual system system config '%ws'\n",state_file);
        if(virtual_arch_type)log_con("Virtual Windows version: %d-bit\n",virtual_arch_type);
        if(virtual_os_version)log_con("Virtual Windows version: %d.%d\n",virtual_os_version/10,virtual_os_version%10);
        log("\n");
    }
    mkdir_r(drp_dir);
    mkdir_r(index_dir);
    mkdir_r(output_dir);
    vault_init();
    bundle_init(&bundle[0]);
    bundle_init(&bundle[1]);
    manager_init(&manager_v[0],&bundle[bundle_display].matcher);
    manager_init(&manager_v[1],&bundle[bundle_display].matcher);

    bundle_prep(&bundle[bundle_display]);

    event=CreateEvent(0,0,0,0);
    thr=(HANDLE)_beginthreadex(0,0,&thread_loadall,&bundle[0],0,0);
    mon_drp=monitor_start(drp_dir,FILE_NOTIFY_CHANGE_LAST_WRITE|FILE_NOTIFY_CHANGE_FILE_NAME,1,drp_callback);
    virusmonitor_start();
    viruscheck(L"",0,0);
    if(!(flags&FLAG_NOGUI)||flags&FLAG_AUTOINSTALL)gui(nCmd);
    exitflag=1;
    SetEvent(event);
    if(mon_drp)monitor_stop(mon_drp);
    WaitForSingleObject(thr,INFINITE);
    CloseHandle_log(thr,L"WinMain",L"thr");
    CloseHandle_log(event,L"WinMain",L"event");

    bundle_free(&bundle[0]);
    bundle_free(&bundle[1]);
    vault_free();
    manager_free(&manager_v[0]);
    manager_free(&manager_v[1]);
#ifndef CONSOLE_MODE
    settings_save();
#endif

    virusmonitor_stop();
    time_total=GetTickCount()-time_total;
    log_times();
    log_stop();
    //signal(SIGSEGV,SIG_DFL);
    ShowWindow(GetConsoleWindow(),SW_SHOWNOACTIVATE);

#ifdef CONSOLE_MODE
    MessageBox(0,L"В папке logs отчет создан!",L"Сообщение",0);
#endif

    if(backtrace)FreeLibrary(backtrace);
    return msg.wParam;
}
//}

//{ Threads
unsigned int __stdcall thread_scandevices(void *arg)
{
    bundle_t *bundle=(bundle_t *)arg;
    state_t *state=&bundle->state;

    if(statemode==0)
        state_scandevices(state);else
    if(statemode==STATEMODE_LOAD)
        state_load(state,state_file);
    return 0;
}

unsigned int __stdcall thread_loadindexes(void *arg)
{
    bundle_t *bundle=(bundle_t *)arg;
    collection_t *collection=&bundle->collection;

    if(manager_g->items_list[SLOT_EMPTY].curpos==1)*drpext_dir=0;
    collection->driverpack_dir=*drpext_dir?drpext_dir:drp_dir;
    //printf("'%ws'\n",collection->driverpack_dir);
    collection_load(collection);
    return 0;
}

unsigned int __stdcall thread_loadall(void *arg)
{
    bundle_t *bundle=(bundle_t *)arg;

    InitializeCriticalSection(&sync);
    do
    {
        int cancel_update=0;
        //long long t=GetTickCount();

        log_con("*** START *** %d,%d\n",bundle_display,bundle_shadow);
        bundle_prep(&bundle[bundle_shadow]);
        bundle_load(&bundle[bundle_shadow]);

        if(WaitForSingleObject(event,0)==WAIT_OBJECT_0)cancel_update=1;

        if(!cancel_update)
        {
            if((flags&FLAG_NOGUI||hMain==0)&&(flags&FLAG_AUTOINSTALL)==0)
            {
                manager_g->matcher=&bundle[bundle_shadow].matcher;
                manager_populate(manager_g);
                manager_filter(manager_g,filters);
                /*if(flags&FLAG_AUTOINSTALL)
                {
                    manager_selectall(manager_g);
                    manager_install(INSTALLDRIVERS);
                }*/
            }
            else
                SendMessage(hMain,WM_BUNDLEREADY,(int)&bundle[bundle_shadow],(int)&bundle[bundle_display]);
        }

        bundle_lowprioirity(&bundle[bundle_shadow]);

        if(cancel_update)
            log_con("*** CANCEL ***\n\n");
        else
        {
            log_con("*** FINISH ***\n\n");
            bundle_display^=1;
            bundle_shadow^=1;
        }
        //printf("%d\n",)
        bundle_free(&bundle[bundle_shadow]);
        bundle_init(&bundle[bundle_shadow]);
        if(cancel_update)SetEvent(event);
        WaitForSingleObject(event,INFINITE);
        //printf("%ld\n",GetTickCount()-t);
    }while(!exitflag);
    DeleteCriticalSection(&sync);
    return 0;
}
//}

//{ Bundle
void bundle_init(bundle_t *bundle)
{
    state_init(&bundle->state);
    collection_init(&bundle->collection,drp_dir,index_dir,output_dir,flags);
    matcher_init(&bundle->matcher,&bundle->state,&bundle->collection);
}

void bundle_prep(bundle_t *bundle)
{
    state_getsysinfo_fast(&bundle->state);
}

void bundle_free(bundle_t *bundle)
{
    collection_free(&bundle->collection);
    matcher_free(&bundle->matcher);
    state_free(&bundle->state);
}

void bundle_load(bundle_t *bundle)
{
    HANDLE thandle[2];

    thandle[0]=(HANDLE)_beginthreadex(0,0,&thread_scandevices,bundle,0,0);
    thandle[1]=(HANDLE)_beginthreadex(0,0,&thread_loadindexes,bundle,0,0);
    WaitForMultipleObjects(2,thandle,1,INFINITE);
    CloseHandle_log(thandle[0],L"bundle_load",L"0");
    CloseHandle_log(thandle[1],L"bundle_load",L"1");

    if(!(flags&FLAG_NOSLOWSYSINFO)&&statemode!=STATEMODE_LOAD)
    {
        state_getsysinfo_slow(&bundle->state);
    }
    matcher_populate(&bundle->matcher);
    matcher_sort(&bundle->matcher);
}

void bundle_lowprioirity(bundle_t *bundle)
{
    time_startup=GetTickCount()-time_startup;

    log_con("lowprioirity.[");
    redrawmainwnd();
    if(!(flags&FLAG_NOSLOWSYSINFO)&&statemode!=STATEMODE_LOAD)
    {
        //state_getsysinfo_slow(&bundle->state);
        redrawmainwnd();
        log_con("6");
    }

    collection_printstates(&bundle->collection);
    state_print(&bundle->state);
    matcher_print(&bundle->matcher);
    manager_print(manager_g);
    log_con("7");

    collection_save(&bundle->collection);
    {
        WCHAR filename[BUFLEN];
        gen_timestamp();
        wsprintf(filename,L"%s\\%sstate.snp",log_dir,timestamp);
        state_save(&bundle->state,filename);
    }
    log_con("8");

    if(flags&COLLECTION_PRINT_INDEX)
    {
        collection_print(&bundle->collection);
        flags&=~COLLECTION_PRINT_INDEX;
    }
    log_con("9]\n");
}
//}

//{ Windows
HWND CreateWindowM(const WCHAR *type,const WCHAR *name,HWND hwnd,int id)
{
    return CreateWindow(type,name,WS_CHILD|WS_VISIBLE,
                        0,0,0,0,hwnd,(HMENU)(id),ghInst,NULL);
}

HWND CreateWindowMF(const WCHAR *type,const WCHAR *name,HWND hwnd,int id,int f)
{
    return CreateWindow(type,name,WS_CHILD|WS_VISIBLE|f,
                        0,0,0,0,hwnd,(HMENU)(id),ghInst,NULL);
}

void setfont()
{
    if(hFont&&!DeleteObject(hFont))
        log_err("ERROR in manager_setfont(): failed DeleteObject\n");

    hFont=CreateFont(-D(FONT_SIZE),0,0,0,FW_DONTCARE,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,
                CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,VARIABLE_PITCH,(WCHAR *)D(FONT_NAME));
    if(!hFont)
        log_err("ERROR in manager_setfont(): failed CreateFont\n");
}

void redrawfield()
{
    if(!hField)
    {
        log_err("ERROR in redrawfield(): hField is 0\n");
        return;
    }
    InvalidateRect(hField,NULL,TRUE);
    UpdateWindow(hField);
}

void redrawmainwnd()
{
    if(!hMain)
    {
        log_err("ERROR in redrawfield(): hField is 0\n");
        return;
    }
    //log_con("Redraw main %d\n",cntd++);
    InvalidateRect(hMain,NULL,TRUE);
    UpdateWindow(hMain);
}

void lang_refresh()
{
    if(!hMain||!hField)
    {
        log_err("ERROR in theme_refresh(): hMain is %d, hField is %d\n",hMain,hField);
        return;
    }
    redrawmainwnd();

    POINT p;
    GetCursorPos(&p);
    SetCursorPos(p.x+1,p.y);
    SetCursorPos(p.x,p.y);
}

void theme_refresh()
{
    RECT rect;

    if(!hMain)
    {
        log_err("ERROR in theme_refresh(): hMain is 0\n");
        return;
    }
    GetWindowRect(hMain,&rect);
    MoveWindow(hMain,rect.left,rect.top,D(MAINWND_WX),D(MAINWND_WY)+1,1);
    MoveWindow(hMain,rect.left,rect.top,D(MAINWND_WX),D(MAINWND_WY),1);
}

void setscrollrange(int y)
{
    SCROLLINFO si;
    RECT rect;

    if(!hField)
    {
        log_err("ERROR in setscrollrange(): hField is 0\n");
        return;
    }
    GetClientRect(hField,&rect);

    si.cbSize=sizeof(si);
    si.fMask =SIF_RANGE|SIF_PAGE;

    si.nMin  =0;
    si.nMax  =y;
    si.nPage =rect.bottom;
    SetScrollInfo(hField,SB_VERT,&si,TRUE);
}

int getscrollpos()
{
    SCROLLINFO si;

    if(!hField)
    {
        log_err("ERROR in getscrollpos(): hField is 0\n");
        return 0;
    }
    si.cbSize=sizeof(si);
    si.fMask=SIF_POS;
    si.nPos=0;
    GetScrollInfo(hField,SB_VERT,&si);
    return si.nPos;
}

void setscrollpos(int pos)
{
    SCROLLINFO si;

    if(!hField)
    {
        log_err("ERROR in setscrollpos(): hField is 0\n");
        return;
    }
    si.cbSize=sizeof(si);
    si.fMask=SIF_POS;
    si.nPos=pos;
    SetScrollInfo(hField,SB_VERT,&si,TRUE);
}
//}

//{ Helpers
void get_resource(int id,void **data,int *size)
{
    HRSRC myResource=FindResource(NULL,MAKEINTRESOURCE(id),(WCHAR *)RESFILE);
    if(!myResource)
    {
        log_err("ERROR in get_resource(): failed FindResource(%d)\n",id);
        *size=0;
        *data=0;
        return;
    }
    *size=SizeofResource(NULL,myResource);
    *data=LoadResource(NULL,myResource);
}

const WCHAR *get_winverstr(manager_t *manager1)
{
    int i;
    int ver=manager1->matcher->state->platform.dwMinorVersion;
    ver+=10*manager1->matcher->state->platform.dwMajorVersion;

    for(i=0;i<NUM_OS;i++)if(windows_ver[i]==ver)return windows_name[i];
    return windows_name[NUM_OS-1];
}

void mkdir_r(const WCHAR *path)
{
    WCHAR buf[BUFLEN];
    WCHAR *p;

    if(!canWrite(path))
    {
        log_err("ERROR in mkdir_r(): Write-protected,'%ws'\n",path);
        return;
    }
    wcscpy(buf,path);
    p=buf;
    while((p=wcschr(p,L'\\')))
    {
        *p=0;
        if(_wmkdir(buf)<0&&errno!=EEXIST)
            log_err("ERROR in mkdir_r(): failed _wmkdir(%ws)\n",buf);
        *p=L'\\';
        p++;

    }
    if(_wmkdir(buf)<0&&errno!=EEXIST)
        log_err("ERROR in mkdir_r(): failed _wmkdir(%ws)\n",buf);
}
//}

//{ Panel
int panel_hitscan(int hx,int hy)
{
    int r,i;

    hx-=D(PANEL_OFSX)+D(PNLITEM_OFSX);
    hy-=D(PANEL_OFSY)+D(PNLITEM_OFSY);
    if(hx<0)return -1;
    if(hx>D(PANEL_WX))return -1;
    if(hy<0)return -1;
    if(hy>=D(PANEL_WY)*(PANELITEMS_NUM-2))return -1;

    r=hy/D(PANEL_WY)+1;
    if(!expertmode)
        for(i=0;i<=r;i++)if(panelitems[i].type==TYPE_GROUP_BREAK)return -1;
    return r;
}

void panel_draw(HDC hdc)
{
    WCHAR buf[BUFLEN];
    POINT p;
    int cur_i;
    int i;
    int x=D(PANEL_OFSX),y=D(PANEL_OFSY);
    int ofsx=D(PNLITEM_OFSX),ofsy=D(PNLITEM_OFSY);

    GetCursorPos(&p);
    ScreenToClient(hMain,&p);
    cur_i=panel_hitscan(p.x,p.y);

    for(i=0;panelitems[i].type;i++)
    {
        if(i==2)
        {
            wsprintf(buf,L"%s (%d-bit)",get_winverstr(manager_g),manager_g->matcher->state->architecture?64:32);
            TextOut(hdc,x+ofsx,y+ofsy,buf,wcslen(buf));
        }
        if(i==3)
        {
            wsprintf(buf,L"%s",manager_g->matcher->state->text+manager_g->matcher->state->product);
            TextOut(hdc,x+ofsx,y+ofsy,buf,wcslen(buf));
        }
        if(panelitems[i].type==TYPE_GROUP_BREAK&&!expertmode)break;
        switch(panelitems[i].type)
        {
            case TYPE_CHECKBOX:
                drawcheckbox(hdc,x+ofsx-1,y+ofsy,D(PNLITEM_WY)-1,D(PNLITEM_WY)-1,panelitems[i].checked,i==cur_i);
                SetTextColor(hdc,D(i==cur_i?CHKBOX_TEXT_COLOR_H:CHKBOX_TEXT_COLOR));
                TextOut(hdc,x+D(CHKBOX_TEXT_OFSX)+ofsx,y+ofsy,STR(panelitems[i].str_id),wcslen(STR(panelitems[i].str_id)));
                y+=D(PANEL_WY);
                break;

            case TYPE_BUTTON:
                box_draw(hdc,x+ofsx,y+ofsy,x+D(PANEL_WX)-ofsx,y+ofsy+D(PNLITEM_WY),i==cur_i?BOX_BUTTON_H:BOX_BUTTON);
                SetTextColor(hdc,D(CHKBOX_TEXT_COLOR));
                if(i==5)
                {
                    int j,cnt=0;
                    itembar_t *itembar;

                    itembar=&manager_g->items_list[RES_SLOTS];
                    for(j=RES_SLOTS;j<manager_g->items_handle.items;j++,itembar++)
                    if(itembar->checked)cnt++;

                    wsprintf(buf,L"%s (%d)",STR(panelitems[i].str_id),cnt);
                    TextOut(hdc,x+ofsx+5,y+ofsy+2,buf,wcslen(buf));
                }
                else
                    TextOut(hdc,x+ofsx+5,y+ofsy+2,STR(panelitems[i].str_id),wcslen(STR(panelitems[i].str_id)));

                y+=D(PANEL_WY);
                break;

            case TYPE_TEXT:
                SetTextColor(hdc,D(i==cur_i&&i>11?CHKBOX_TEXT_COLOR_H:CHKBOX_TEXT_COLOR));
                TextOut(hdc,x+ofsx,y+ofsy,STR(panelitems[i].str_id),wcslen(STR(panelitems[i].str_id)));
                y+=D(PANEL_WY);
                break;

            case TYPE_GROUP_BREAK:
            case TYPE_GROUP:
                if(i)y+=D(PANEL_WY);
                box_draw(hdc,x,y,x+D(PANEL_WX),y+D(PANEL_WY)*panelitems[i].action_id+ofsy*2,BOX_PANEL);
                break;

            default:
                break;
        }

    }
}
//}

//{ GUI
void gui(int nCmd)
{
    int done=0;
    WNDCLASSEX wcx;
    memset(&wcx,0,sizeof(WNDCLASSEX));
    wcx.cbSize=         sizeof(WNDCLASSEX);
    wcx.lpfnWndProc=    WndProc;
    wcx.hInstance=      ghInst;
    wcx.hIcon=          LoadIcon(ghInst,MAKEINTRESOURCE(IDI_ICON1));
    wcx.hCursor=        (HCURSOR)LoadCursor(0,IDC_ARROW);
    wcx.lpszClassName=  classMain;
    wcx.hbrBackground=  (HBRUSH)(COLOR_WINDOW+1);
    if(!RegisterClassEx(&wcx))
    {
        log_err("ERROR in gui(): failed to register '%ws' class\n",wcx.lpszClassName);
        return;
    }

    wcx.lpfnWndProc=PopupProcedure;
    wcx.lpszClassName=classPopup;
    wcx.hIcon=0;
    if(!RegisterClassEx(&wcx))
    {
        log_err("ERROR in gui(): failed to register '%ws' class\n",wcx.lpszClassName);
        UnregisterClass_log(classMain,ghInst,L"gui",L"classMain");
        return;
    }

    wcx.lpfnWndProc=WindowGraphProcedure;
    wcx.lpszClassName=classField;
    if(!RegisterClassEx(&wcx))
    {
        log_err("ERROR in gui(): failed to register '%ws' class\n",wcx.lpszClassName);
        UnregisterClass_log(classMain,ghInst,L"gui",L"classMain");
        UnregisterClass_log(classPopup,ghInst,L"gui",L"classPopup");
        return;
    }

    if(!license)
        DialogBox(ghInst,MAKEINTRESOURCE(IDD_DIALOG1),0,LicenseProcedure);

    if(license)
    {
//        hMain=CreateWindowEx(/*WS_EX_LAYERED|WS_EX_COMPOSITED(*/0,
        hMain=CreateWindowEx(0,
                            classMain,
                            APPTITLE,
                            WS_OVERLAPPEDWINDOW|WS_CLIPCHILDREN,
                            CW_USEDEFAULT,CW_USEDEFAULT,D(MAINWND_WX),D(MAINWND_WY),
                            0,0,ghInst,0);
        if(!hMain)
        {
            log_err("ERROR in gui(): failed to create '%ws' window\n",classMain);
            return;
        }

        ShowWindow(hMain,flags&FLAG_NOGUI?SW_HIDE:nCmd);

        while(!done)
        {
            while(WAIT_IO_COMPLETION==MsgWaitForMultipleObjectsEx(0,0,INFINITE,QS_ALLINPUT,MWMO_ALERTABLE));

            while(PeekMessage(&msg,0,0,0,PM_REMOVE))
            {
                if(msg.message==WM_QUIT)
                {
                    done=TRUE;
                    break;
                }else
                if(msg.message==WM_KEYDOWN&&!(msg.lParam&(1<<30)))
                {
                    if(msg.wParam==VK_CONTROL||msg.wParam==VK_SPACE)
                    {
                        POINT p;
                        GetCursorPos(&p);
                        SetCursorPos(p.x+1,p.y);
                        SetCursorPos(p.x,p.y);
                    }
                    if(msg.wParam==VK_CONTROL)ctrl_down=1;
                    if(msg.wParam==VK_SPACE)  space_down=1;
                }else
                if(msg.message==WM_KEYUP)
                {
                    if(msg.wParam==VK_CONTROL||msg.wParam==VK_SPACE)
                    {
                        drawpopup(-1,FLOATING_NONE,0,0,hField);
                    }
                    if(msg.wParam==VK_CONTROL)ctrl_down=0;
                    if(msg.wParam==VK_SPACE)  space_down=0;
                }

                if(!(msg.message==WM_SYSKEYDOWN&&msg.wParam==VK_MENU))
                {
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                }
            }
        }
    }

    UnregisterClass_log(classMain,ghInst,L"gui",L"classMain");
    UnregisterClass_log(classPopup,ghInst,L"gui",L"classPopup");
    UnregisterClass_log(classField,ghInst,L"gui",L"classField");
}

LRESULT CALLBACK WndProcCommon(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
    UNREFERENCED_PARAMETER(wParam);

    RECT rect;
    short x,y;

    x=LOWORD(lParam);
    y=HIWORD(lParam);
    switch(uMsg)
    {
        case WM_MOUSELEAVE:
            ShowWindow(hPopup,SW_HIDE);
            InvalidateRect(hwnd,0,0);//common
            break;

        case WM_ACTIVATE:
            InvalidateRect(hwnd,0,0);//common
            break;

        case WM_MOUSEMOVE:
            if(mousedown==1||mousedown==2)
            {
                GetWindowRect(hMain,&rect);
                if(mousedown==2||abs(mousex-x)>2||abs(mousey-y)>2)
                {
                    mousedown=2;
                    MoveWindow(hMain,rect.left+x-mousex,rect.top+y-mousey,mainx_w,mainy_w,1);
                }
            }
            return 1;

        case WM_LBUTTONDOWN:
            SetFocus(hMain);
            if(!IsZoomed(hMain))
            {
                mousex=x;
                mousey=y;
                mousedown=1;
                SetCapture(hwnd);
            }
            else
                mousedown=3;

            break;

        case WM_CANCELMODE:
        case WM_LBUTTONUP:
        case WM_MBUTTONUP:
        case WM_RBUTTONUP:
            mousex=-1;
            mousey=-1;
            SetCursor(LoadCursor(0,IDC_ARROW));
            ReleaseCapture();
            if(mousedown==2)
            {
                mousedown=0;
                mouseclick=0;
                break;
            }
            mouseclick=mousedown&&uMsg==WM_LBUTTONUP?1:0;
            mousedown=0;
            return 1;

        default:
            return 1;
    }
    return 0;
}

void snapshot()
{
    OPENFILENAME ofn;
    memset(&ofn,0,sizeof(OPENFILENAME));
    ofn.lStructSize=sizeof(OPENFILENAME);
    ofn.hwndOwner  =hMain;
    ofn.lpstrFilter=STR(STR_OPENSNAPSHOT);
    ofn.nMaxFile   =BUFLEN;
    ofn.lpstrDefExt=L"snp";
    ofn.lpstrFile  =state_file;
    ofn.Flags      =OFN_FILEMUSTEXIST|OFN_HIDEREADONLY|OFN_PATHMUSTEXIST|OFN_NOCHANGEDIR;

    if(GetOpenFileName(&ofn))
    {
        statemode=STATEMODE_LOAD;
        PostMessage(hMain,WM_DEVICECHANGE,7,2);
    }
}

void extractto()
{
    BROWSEINFO lpbi;
    WCHAR dir[BUFLEN];
    WCHAR buf[BUFLEN];
    LPITEMIDLIST list;
    WCHAR **argv;
    int argc;

    memset(&lpbi,0,sizeof(BROWSEINFO));
    lpbi.hwndOwner=hMain;
    lpbi.pszDisplayName=dir;
    lpbi.lpszTitle=STR(STR_EXTRACTFOLDER);
    lpbi.ulFlags=BIF_NEWDIALOGSTYLE|BIF_EDITBOX;


    list=SHBrowseForFolder(&lpbi);
    if(list)
    {
        SHGetPathFromIDList(list,dir);

        argv=CommandLineToArgvW(GetCommandLineW(),&argc);
        //printf("'%ws',%d\n",argv[0],argc);
        wsprintf(buf,L"%s\\drv.exe",dir);
        if(!CopyFile(argv[0],buf,0))
            log_err("ERROR in extractto(): failed CopyFile(%ws,%ws)\n",argv[0],buf);
        LocalFree(argv);

        wcscat(dir,L"\\drivers");
        wcscpy(extractdir,dir);
        manager_install(OPENFOLDER);
    }
}

void drvdir()
{
    BROWSEINFO lpbi;
    LPITEMIDLIST list;

    memset(&lpbi,0,sizeof(BROWSEINFO));
    lpbi.hwndOwner=hMain;
    lpbi.pszDisplayName=drpext_dir;
    lpbi.lpszTitle=STR(STR_EXTRACTFOLDER);
    lpbi.ulFlags=BIF_NEWDIALOGSTYLE|BIF_EDITBOX;

    list=SHBrowseForFolder(&lpbi);
    if(list)
    {
        SHGetPathFromIDList(list,drpext_dir);
        int len=wcslen(drpext_dir);
        drpext_dir[len]=0;
//        printf("'%ws',%d\n",drpext_dir,len);
        PostMessage(hMain,WM_DEVICECHANGE,7,2);
    }
}

WCHAR *getHWIDby(int id,int num)
{
    devicematch_t *devicematch_f=manager_g->items_list[id].devicematch;
    WCHAR *p;
    char *t=manager_g->matcher->state->text;
    int i=0;

    if(devicematch_f->device->HardwareID)
    {
        p=(WCHAR *)(t+devicematch_f->device->HardwareID);
        while(*p)
        {
            if(i==num)return p;
            p+=lstrlen(p)+1;
            i++;
        }
    }
    if(devicematch_f->device->CompatibleIDs)
    {
        p=(WCHAR *)(t+devicematch_f->device->CompatibleIDs);
        while(*p)
        {
            if(i==num)return p;
            p+=lstrlen(p)+1;
            i++;
        }
    }
    return L"";
}

void escapeAmpUrl(WCHAR *buf,WCHAR *source)
{
    WCHAR *p1=buf,*p2=source;

    while(*p2)
    {
        *p1=*p2;
        if(*p1==L'&')
        {
            *p1++=L'%';
            *p1++=L'2';
            *p1=L'6';
        }
        if(*p1==L'\\')
        {
            *p1++=L'%';
            *p1++=L'5';
            *p1=L'C';
        }
        p1++;p2++;
    }
    *p1=0;
}

void checktimer(WCHAR *str,long long t,int uMsg)
{
    if(GetTickCount()-t>40&&log_verbose&LOG_VERBOSE_LAGCOUNTER)
        log_con("GUI lag in %ws[%X]: %ld\n",str,uMsg,GetTickCount()-t);
}

LRESULT CALLBACK WndProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
    RECT rect;
    short x,y;

    int i,j,f;
    int wp;
    long long timer=GetTickCount();

    x=LOWORD(lParam);
    y=HIWORD(lParam);

    if(WndProcCommon(hwnd,uMsg,wParam,lParam))
    switch(uMsg)
    {
        case WM_CREATE:
            // Canvas
            canvas_init(&canvasMain);

            // Field
            hField=CreateWindowM(classField,NULL,hwnd,0);

            // Popup
            hPopup=CreateWindowEx(/*WS_EX_COMPOSITED|*/WS_EX_LAYERED|WS_EX_NOACTIVATE|WS_EX_TOPMOST|WS_EX_TRANSPARENT,
                classPopup,L"",WS_POPUP,
                0,0,0,0,hwnd,(HMENU)0,ghInst,0);

            // Lang
            hLang=CreateWindowMF(WC_COMBOBOX,L"",hwnd,ID_LANG,CBS_DROPDOWNLIST|CBS_HASSTRINGS|WS_OVERLAPPED);
            SendMessage(hwnd,WM_UPDATELANG,0,0);

            // Theme
            hTheme=CreateWindowMF(WC_COMBOBOX,L"",hwnd,ID_THEME,CBS_DROPDOWNLIST|CBS_HASSTRINGS|WS_OVERLAPPED);
            SendMessage(hwnd,WM_UPDATETHEME,0,0);

            // Misc
            vault_startmonitors();
            DragAcceptFiles(hwnd,1);

            manager_populate(manager_g);
            manager_filter(manager_g,filters);
            manager_setpos(manager_g);

            GetWindowRect(GetDesktopWindow(),&rect);
            rect.left=(rect.right-D(MAINWND_WX))/2;
            rect.top=(rect.bottom-D(MAINWND_WY))/2;
            MoveWindow(hwnd,rect.left,rect.top,D(MAINWND_WX),D(MAINWND_WY),1);
            break;

        case WM_DESTROY:
            if(!DeleteObject(hFont))
                log_err("ERROR in manager_free(): failed DeleteObject\n");
            vault_stopmonitors();
            canvas_free(&canvasMain);
            PostQuitMessage(0);
            break;

        case WM_UPDATELANG:
            SendMessage(hLang,CB_RESETCONTENT,0,0);
            lang_enum(hLang,L"langs",manager_g->matcher->state->locale);
            f=SendMessage(hLang,CB_FINDSTRINGEXACT,-1,(int)curlang);
            if(f==CB_ERR)f=SendMessage(hLang,CB_GETCOUNT,0,0)-1;
            lang_set(f);
            SendMessage(hLang,CB_SETCURSEL,f,0);
            lang_refresh();
            break;

        case WM_UPDATETHEME:
            SendMessage(hTheme,CB_RESETCONTENT,0,0);
            theme_enum(hTheme,L"themes");
            f=SendMessage(hTheme,CB_FINDSTRINGEXACT,-1,(int)curtheme);
            if(f==CB_ERR)
            {
                theme_set(f);
                j=SendMessage(hTheme,CB_GETCOUNT,0,0);
                for(i=0;i<j;i++)
                    if(StrStrI(themelist[i],(WCHAR *)D(THEME_NAME)))f=i;
            }else
                theme_set(f);

            setfont();
            SendMessage(hTheme,WM_SETFONT,(int)hFont,MAKELPARAM(FALSE,0));
            SendMessage(hLang,WM_SETFONT,(int)hFont,MAKELPARAM(FALSE,0));
            SendMessage(hTheme,CB_SETCURSEL,f,0);
            redrawmainwnd();
            theme_refresh();
            break;

        case WM_BUNDLEREADY:
            {
                bundle_t *bb=(bundle_t *)wParam;
                manager_t *manager_prev=manager_g;

                log_con("{Sync");
                EnterCriticalSection(&sync);
                log_con("...\n");
                manager_active++;
                manager_active&=1;
                manager_g=&manager_v[manager_active];

                manager_g->matcher=&bb->matcher;
                memcpy(manager_g->items_list,manager_prev->items_list,sizeof(itembar_t)*RES_SLOTS);
                manager_populate(manager_g);
                manager_filter(manager_g,filters);
                manager_g->items_list[SLOT_SNAPSHOT].isactive=statemode==STATEMODE_LOAD?1:0;
                manager_g->items_list[SLOT_DPRDIR].isactive=*drpext_dir?1:0;
                manager_restorepos(manager_g,manager_prev);
                viruscheck(L"",0,0);
                manager_setpos(manager_g);
                log_con("}Sync\n");
                LeaveCriticalSection(&sync);
                //log_con("Mode in WM_BUNDLEREADY: %d\n",installmode);
                if(flags&FLAG_AUTOINSTALL)
                {
                    int cnt=0;
                    if(installmode==MODE_SCANNING)
                    {
                        manager_selectall(manager_g);
                        itembar_t *itembar=&manager_g->items_list[RES_SLOTS];
                        for(i=RES_SLOTS;i<manager_g->items_handle.items;i++,itembar++)
                            if(itembar->checked)
                        {
                            cnt++;
                        }

                        if(!cnt)flags&=~FLAG_AUTOINSTALL;
                        log_con("Autoinstall rescan: %d found\n",cnt);
                    }

                    if(installmode==MODE_NONE||(installmode==MODE_SCANNING&&cnt))
                    {
                        manager_selectall(manager_g);
                        if((flags&FLAG_EXTRACTONLY)==0)
                        wsprintf(extractdir,L"%s\\SDI",manager_g->matcher->state->text+manager_g->matcher->state->temp);
                        manager_install(INSTALLDRIVERS);
                    }
                    else
                    {
                        WCHAR buf[BUFLEN];
                        installmode=MODE_NONE;
                        wsprintf(buf,L" /c %s",needreboot?finish_rb:finish);
                        if(*(needreboot?finish_rb:finish))
                            RunSilent(L"cmd",buf,SW_HIDE,0);
                        if(flags&FLAG_AUTOCLOSE)PostMessage(hMain,WM_CLOSE,0,0);
                    }
                }
                else
                    if(installmode==MODE_SCANNING)installmode=MODE_NONE;
            }
            break;

        case WM_DROPFILES:
		{
            WCHAR lpszFile[MAX_PATH]={0};
            UINT uFile=0;
            HDROP hDrop=(HDROP)wParam;

            uFile=DragQueryFile(hDrop,0xFFFFFFFF,NULL,0);
            if(uFile!=1)
            {
                //MessageBox(0,L"Dropping multiple files is not supported.",NULL,MB_ICONERROR);
                DragFinish(hDrop);
                break;
            }

            lpszFile[0] = '\0';
            if(DragQueryFile(hDrop,0,lpszFile,MAX_PATH))
            {
                uFile=GetFileAttributes(lpszFile);
                if(uFile!=INVALID_FILE_ATTRIBUTES&&uFile&FILE_ATTRIBUTE_DIRECTORY)
                {
                    wcscpy(drpext_dir,lpszFile);
                    PostMessage(hMain,WM_DEVICECHANGE,7,2);
                }
                else if(StrStrI(lpszFile,L".snp"))
                {
                    wcscpy(state_file,lpszFile);
                    statemode=STATEMODE_LOAD;
                    PostMessage(hMain,WM_DEVICECHANGE,7,2);
                }
                //else
                //    MessageBox(NULL,lpszFile,NULL,MB_ICONINFORMATION);
            }
            DragFinish(hDrop);
            break;
		}

        case WM_KEYUP:
            if(wParam==VK_F5)
                PostMessage(hwnd,WM_DEVICECHANGE,7,2);
            if(wParam==VK_F6&&ctrl_down)
            {
                manager_testitembars(manager_g);
                manager_setpos(manager_g);
                redrawfield();
            }
            if(wParam==VK_F7)
            {
                wndclicker(2);
                MessageBox(0,L"Windows data recorded into the log.",L"Message",0);
            }
            if(wParam==VK_F8)
            {
                flags^=FLAG_SHOWDRPNAMES;
                if(ctrl_down)
                {
                    flags|=FLAG_SHOWDRPNAMES;
                    manager_sort(manager_g);
                }
                redrawfield();
            }
            break;

        case WM_SYSKEYDOWN:
            if(wParam==VK_MENU)break;
            return DefWindowProc(hwnd,uMsg,wParam,lParam);

        case WM_DEVICECHANGE:
            if(installmode==MODE_INSTALLING)break;
            log_con("WM_DEVICECHANGE(%x,%x)\n",wParam,lParam);
            if(lParam<2)
                instflag|=RESTOREPOS;
            else
                instflag&=~RESTOREPOS;

            SetEvent(event);
            break;

        case WM_SIZE:
            SetLayeredWindowAttributes(hMain,0,D(MAINWND_TRANSPARENCY),LWA_ALPHA);
            SetLayeredWindowAttributes(hPopup,0,D(POPUP_TRANSPARENCY),LWA_ALPHA);

            MoveWindow(hField,D(DRVLIST_OFSX),D(DRVLIST_OFSY),x-D(DRVLIST_OFSX)-D(DRVLIST_OFSY),y-D(DRVLIST_OFSY)*2,TRUE);
            MoveWindow(hLang,D(PANEL_OFSX)+D(PNLITEM_OFSX),D(PANEL_OFSY)+9*D(PANEL_WY)-2,D(PANEL_WX)-D(PNLITEM_OFSX)*2,190,0);
            MoveWindow(hTheme,D(PANEL_OFSX)+D(PNLITEM_OFSX),D(PANEL_OFSY)+11*D(PANEL_WY)-2,D(PANEL_WX)-D(PNLITEM_OFSX)*2,190,0);
            manager_setpos(manager_g);
            redrawmainwnd();

            GetWindowRect(hwnd,&rect);
            mainx_w=rect.right-rect.left;
            mainy_w=rect.bottom-rect.top;
            break;

        case WM_TIMER:
            if(manager_animate(manager_g))
                redrawfield();
            else
                KillTimer(hwnd,1);
            break;

        case WM_PAINT:
            GetClientRect(hwnd,&rect);
            canvas_begin(&canvasMain,hwnd,rect.right,rect.bottom);

            box_draw(canvasMain.hdcMem,0,0,rect.right+1,rect.bottom+1,BOX_MAINWND);
            SelectObject(canvasMain.hdcMem,hFont);
            drawrevision(canvasMain.hdcMem,rect.bottom);
            panel_draw(canvasMain.hdcMem);

            canvas_end(&canvasMain);
            redrawfield();
            break;

        case WM_ERASEBKGND:
            return 1;

        case WM_MOUSEMOVE:
            GetClientRect(hwnd,&rect);
            i=panel_hitscan(x,y);

            if(i<0&&y>rect.bottom-20&&x<D(PANEL_WX))
                drawpopup(-1,FLOATING_ABOUT,x,y,hwnd);
            else
                drawpopup(panelitems[i].str_id+1,i>0&&i<4?FLOATING_SYSINFO:FLOATING_TOOLTIP,x,y,hwnd);

            if(panel_lasti!=i)redrawmainwnd();
            panel_lasti=i;
            break;

        case WM_LBUTTONUP:
            if(!mouseclick)break;
            i=panel_hitscan(x,y);
            if(i<0)break;
            if(i<4)
                RunSilent(L"devmgmt.msc",0,SW_SHOW,0);
            else
            if(panelitems[i].type==TYPE_CHECKBOX||TYPE_BUTTON)
            {
                panelitems[i].checked^=1;
                if(panelitems[i].action_id==ID_EXPERT_MODE)
                {
                    expertmode=panelitems[i].checked;
                    ShowWindow(GetConsoleWindow(),expertmode&&ctrl_down?SW_SHOWNOACTIVATE:hideconsole);
                }
                else
                    PostMessage(hwnd,WM_COMMAND,panelitems[i].action_id+(BN_CLICKED<<16),0);

                InvalidateRect(hwnd,NULL,TRUE);
            }
            break;

        case WM_RBUTTONDOWN:
            i=panel_hitscan(x,y);
            if(i>=0&&i<4)contextmenu2(x,y);
            break;

        case WM_MOUSEWHEEL:
            i=GET_WHEEL_DELTA_WPARAM(wParam);
            if(space_down)
            {
                horiz_sh-=i/5;
                if(horiz_sh>0)horiz_sh=0;
                InvalidateRect(hPopup,0,0);
            }
            else
                SendMessage(hField,WM_VSCROLL,MAKELONG(i>0?SB_LINEUP:SB_LINEDOWN,0),0);
            break;

        case WM_COMMAND:
            wp=LOWORD(wParam);
//            printf("com:%d,%d,%d\n",wp,HIWORD(wParam),BN_CLICKED);

            switch(wp)
            {
                case ID_SCHEDULE:
                    manager_toggle(manager_g,floating_itembar);
                    redrawfield();
                    break;

                case ID_SHOWALT:
                    if(floating_itembar==SLOT_RESTORE_POINT)
                    {
                        RunSilent(L"cmd",L"/c %windir%\\Sysnative\\rstrui.exe",SW_HIDE,0);
                    }
                    else
                    {
                        manager_expand(manager_g,floating_itembar);
                        manager_setpos(manager_g);
                    }
                    break;

                case ID_OPENINF:
                case ID_LOCATEINF:
                    {
                        WCHAR buf[BUFLEN];

                        devicematch_t *devicematch_f=manager_g->items_list[floating_itembar].devicematch;
                        driver_t *cur_driver=&manager_g->matcher->state->drivers_list[devicematch_f->device->driver_index];
                        wsprintf(buf,L"%s%s%s",
                                (wp==ID_LOCATEINF)?L"/select,":L"",
                               manager_g->matcher->state->text+manager_g->matcher->state->windir,
                               manager_g->matcher->state->text+cur_driver->InfPath);

                        if(wp==ID_OPENINF)
                            RunSilent(buf,L"",SW_SHOW,0);
                        else
                            RunSilent(L"explorer.exe",buf,SW_SHOW,0);
                    }
                    break;

                default:
                    break;
            }
            if(wp>=ID_HWID_CLIP&&wp<=ID_HWID_WEB+100)
            {
                int id=wp%100;
                //printf("%ws\n",getHWIDby(floating_itembar,id));

                if(wp>=ID_HWID_WEB)
                {
                    WCHAR buf[BUFLEN];
                    WCHAR buf2[BUFLEN];
                    wsprintf(buf,L"https://www.google.com/#q=%s",getHWIDby(floating_itembar,id));
                    wsprintf(buf,L"http://catalog.update.microsoft.com/v7/site/search.aspx?q=%s",getHWIDby(floating_itembar,id));
                    escapeAmpUrl(buf2,buf);
                    RunSilent(L"iexplore.exe",buf2,SW_SHOW,0);

                }
                else
                {
                    int len=wcslen(getHWIDby(floating_itembar,id))*2+2;
                    HGLOBAL hMem=GlobalAlloc(GMEM_MOVEABLE,len);
                    memcpy(GlobalLock(hMem),getHWIDby(floating_itembar,id),len);
                    GlobalUnlock(hMem);
                    OpenClipboard(0);
                    EmptyClipboard();
                    SetClipboardData(CF_UNICODETEXT,hMem);
                    CloseClipboard();
                }
            }

            if(HIWORD(wParam)==CBN_SELCHANGE)
            {
                if(wp==ID_LANG)
                {
                    i=SendMessage((HWND)lParam,CB_GETCURSEL,0,0);
                    SendMessage((HWND)lParam,CB_GETLBTEXT,i,(int)curlang);
                    lang_set(i);
                    lang_refresh();
                }

                if(wp==ID_THEME)
                {
                    i=SendMessage((HWND)lParam,CB_GETCURSEL,0,0);
                    SendMessage((HWND)lParam,CB_GETLBTEXT,i,(int)curtheme);
                    theme_set(i);
                    theme_refresh();
                }
            }

            if(HIWORD(wParam)==BN_CLICKED)
            switch(wp)
            {
                case ID_INSTALL:
                    if(installmode==MODE_NONE)
                    {
                        if((flags&FLAG_EXTRACTONLY)==0)
                        wsprintf(extractdir,L"%s\\SDI",manager_g->matcher->state->text+manager_g->matcher->state->temp);
                        manager_install(INSTALLDRIVERS);
                    }
                    break;

                case ID_SELECT_NONE:
                    manager_selectnone(manager_g);
                    redrawfield();
                    break;

                case ID_SELECT_ALL:
                    manager_selectall(manager_g);
                    redrawfield();
                    break;

                case ID_OPENLOGS:
                    ShellExecute(hwnd,L"explore",log_dir,0,0,SW_SHOW);
                    break;

                case ID_SNAPSHOT:
                    snapshot();
                    break;

                case ID_EXTRACT:
                    extractto();
                    break;

                case ID_DRVDIR:
                    drvdir();
                    break;

                case ID_SHOW_MISSING:
                case ID_SHOW_NEWER:
                case ID_SHOW_CURRENT:
                case ID_SHOW_OLD:
                case ID_SHOW_BETTER:
                case ID_SHOW_WORSE_RANK:
                case ID_SHOW_NF_MISSING:
                case ID_SHOW_NF_UNKNOWN:
                case ID_SHOW_NF_STANDARD:
                case ID_SHOW_ONE:
                case ID_SHOW_DUP:
                case ID_SHOW_INVALID:
                    filters=0;
                    for(j=0;j<PANELITEMS_NUM;j++)
                        if(panelitems[j].type==TYPE_CHECKBOX&&panelitems[j].checked&&panelitems[j].action_id!=ID_EXPERT_MODE)
                            filters+=1<<panelitems[j].action_id;

                    manager_filter(manager_g,filters);
                    manager_setpos(manager_g);
                    //manager_print(manager_g);
                    break;

                default:
                    break;
            }
            break;

        default:
            i=DefWindowProc(hwnd,uMsg,wParam,lParam);
            checktimer(L"MainD",timer,uMsg);
            return i;
    }
    checktimer(L"Main",timer,uMsg);
    return 0;
}

void escapeAmp(WCHAR *buf,WCHAR *source)
{
    WCHAR *p1=buf,*p2=source;

    while(*p2)
    {
        *p1=*p2;
        if(*p1==L'&')*(++p1)=L'&';
        p1++;p2++;
    }
    *p1=0;
}

void contextmenu2(int x,int y)
{
    int i;
    RECT rect;
    OSVERSIONINFOEX *platform=&manager_g->matcher->state->platform;
    HMENU
        hPopupMenu=CreatePopupMenu(),
        hSub1=CreatePopupMenu();
    int ver=platform->dwMinorVersion+10*platform->dwMajorVersion;
    int arch=manager_g->matcher->state->architecture;

    for(i=0;i<NUM_OS-1;i++)
    {
        InsertMenu(hSub1,i,MF_BYPOSITION|MF_STRING|(ver==windows_ver[i]?MF_CHECKED:0),
                   ID_HWID_WEB+i,windows_name[i]);
    }

    i=0;
    InsertMenu(hPopupMenu,i++,MF_BYPOSITION|MF_STRING|MF_POPUP,(int)hSub1,STR(STR_SYS_WINVER));
    InsertMenu(hPopupMenu,i++,MF_BYPOSITION|MF_STRING|(arch==0?MF_CHECKED:0),       ID_SHOWALT,  STR(STR_SYS_32));
    InsertMenu(hPopupMenu,i++,MF_BYPOSITION|MF_STRING|(arch==1?MF_CHECKED:0),       ID_SHOWALT,  STR(STR_SYS_64));
    InsertMenu(hPopupMenu,i++,MF_BYPOSITION|MF_SEPARATOR,0,0);
    InsertMenu(hPopupMenu,i++,MF_BYPOSITION|MF_STRING,       ID_SHOWALT,  STR(STR_SYS_DEVICEMNG));
    InsertMenu(hPopupMenu,i++,MF_BYPOSITION|MF_SEPARATOR,0,0);
    InsertMenu(hPopupMenu,i++,MF_BYPOSITION|MF_STRING|(flags&FLAG_DISABLEINSTALL?MF_CHECKED:0),       ID_SHOWALT,  STR(STR_SYS_DISINSTALL));
    InsertMenu(hPopupMenu,i++,MF_BYPOSITION|MF_STRING|(flags&FLAG_NORESTOREPOINT?MF_CHECKED:0),       ID_SHOWALT,  STR(STR_SYS_DISRESTPNT));
    SetForegroundWindow(hMain);
    GetWindowRect(hMain,&rect);
    TrackPopupMenu(hPopupMenu,TPM_LEFTALIGN,rect.left+x,rect.top+y,0,hMain,NULL);
}

void contextmenu(int x,int y)
{

    int i;
    RECT rect;
    HMENU
        hPopupMenu=CreatePopupMenu(),
        hSub1=CreatePopupMenu(),
        hSub2=CreatePopupMenu();

    itembar_t *itembar=&manager_g->items_list[floating_itembar];
    int flags1=itembar->checked?MF_CHECKED:0;
    int flags2=itembar->isactive&2?MF_CHECKED:0;

    if(floating_itembar==SLOT_RESTORE_POINT)
    {
        i=0;
        InsertMenu(hPopupMenu,i++,MF_BYPOSITION|MF_STRING|flags1,ID_SCHEDULE, STR(STR_REST_SCHEDULE));
        InsertMenu(hPopupMenu,i++,MF_BYPOSITION|MF_STRING,       ID_SHOWALT,  STR(STR_REST_ROLLBACK));
        SetForegroundWindow(hMain);
        GetWindowRect(hField,&rect);
        TrackPopupMenu(hPopupMenu,TPM_LEFTALIGN,rect.left+x,rect.top+y,0,hMain,NULL);
        return;
    }
    if(floating_itembar<RES_SLOTS)return;
    //printf("itembar %d\n",floating_itembar);

    devicematch_t *devicematch_f=manager_g->items_list[floating_itembar].devicematch;
    driver_t *cur_driver=0;
    WCHAR *p;
    char *t=manager_g->matcher->state->text;
    if(devicematch_f->device->driver_index>=0)cur_driver=&manager_g->matcher->state->drivers_list[devicematch_f->device->driver_index];
    int flags3=cur_driver?0:MF_GRAYED;
    WCHAR buf[512];

    i=0;
    if(devicematch_f->device->HardwareID)
    {
        p=(WCHAR *)(t+devicematch_f->device->HardwareID);
        while(*p)
        {
            escapeAmp(buf,p);
            InsertMenu(hSub1,i,MF_BYPOSITION|MF_STRING,ID_HWID_WEB+i,buf);
            InsertMenu(hSub2,i,MF_BYPOSITION|MF_STRING,ID_HWID_CLIP+i,buf);
            p+=lstrlen(p)+1;
            i++;
        }
    }
    if(devicematch_f->device->CompatibleIDs)
    {
        p=(WCHAR *)(t+devicematch_f->device->CompatibleIDs);
        while(*p)
        {
            escapeAmp(buf,p);
            InsertMenu(hSub1,i,MF_BYPOSITION|MF_STRING,ID_HWID_WEB+i,buf);
            InsertMenu(hSub2,i,MF_BYPOSITION|MF_STRING,ID_HWID_CLIP+i,buf);
            p+=lstrlen(p)+1;
            i++;
        }
    }

    i=0;
    InsertMenu(hPopupMenu,i++,MF_BYPOSITION|MF_STRING|flags1,ID_SCHEDULE, STR(STR_CONT_INSTALL));
    InsertMenu(hPopupMenu,i++,MF_BYPOSITION|MF_STRING|flags2,ID_SHOWALT,  STR(STR_CONT_SHOWALT));
    InsertMenu(hPopupMenu,i++,MF_BYPOSITION|MF_SEPARATOR,0,0);
    InsertMenu(hPopupMenu,i++,MF_BYPOSITION|MF_STRING|MF_POPUP,(int)hSub1,STR(STR_CONT_HWID_SEARCH));
    InsertMenu(hPopupMenu,i++,MF_BYPOSITION|MF_STRING|MF_POPUP,(int)hSub2,STR(STR_CONT_HWID_CLIP));
    InsertMenu(hPopupMenu,i++,MF_BYPOSITION|MF_SEPARATOR,0,0);
    InsertMenu(hPopupMenu,i++,MF_BYPOSITION|MF_STRING|flags3,ID_OPENINF,  STR(STR_CONT_OPENINF));
    InsertMenu(hPopupMenu,i++,MF_BYPOSITION|MF_STRING|flags3,ID_LOCATEINF,STR(STR_CONT_LOCATEINF));
    SetForegroundWindow(hMain);
    GetWindowRect(hField,&rect);
    TrackPopupMenu(hPopupMenu,TPM_LEFTALIGN,rect.left+x,rect.top+y,0,hMain,NULL);
}

LRESULT CALLBACK WindowGraphProcedure(HWND hwnd,UINT message,WPARAM wParam,LPARAM lParam)
{
    SCROLLINFO si;
    RECT rect;
    short x,y;
    long long timer=GetTickCount();
    int i;

    x=LOWORD(lParam);
    y=HIWORD(lParam);
    if(WndProcCommon(hwnd,message,wParam,lParam))
    switch(message)
    {
        case WM_CREATE:
            canvas_init(&canvasField);
            break;

        case WM_PAINT:
            y=getscrollpos();

            GetClientRect(hwnd,&rect);
            canvas_begin(&canvasField,hwnd,rect.right,rect.bottom);

            BitBlt(canvasField.hdcMem,0,0,rect.right,rect.bottom,canvasMain.hdcMem,D(DRVLIST_OFSX),D(DRVLIST_OFSY),SRCCOPY);
            box_draw(canvasField.hdcMem,0,0,rect.right,rect.bottom,BOX_DRVLIST);
            manager_draw(manager_g,canvasField.hdcMem,y);

            canvas_end(&canvasField);
            break;

        case WM_DESTROY:
            canvas_free(&canvasField);
            break;

        case WM_ERASEBKGND:
            return 1;

        case WM_SIZE:
            mainx_c=x;
            mainy_c=y;
            D(DRVITEM_WX)=x-D(DRVITEM_OFSX)*2;
            break;

        case WM_VSCROLL:
            si.cbSize=sizeof(si);
            si.fMask=SIF_ALL;
            si.nPos=getscrollpos();
            GetScrollInfo(hwnd,SB_VERT,&si);
            switch(LOWORD(wParam))
            {
                case SB_LINEUP:si.nPos-=35;break;
                case SB_LINEDOWN:si.nPos+=35;break;
                case SB_PAGEUP:si.nPos-=si.nPage;break;
                case SB_PAGEDOWN:si.nPos+=si.nPage;break;
                case SB_THUMBTRACK:si.nPos=si.nTrackPos;break;
                default:break;
            }
            setscrollpos(si.nPos);
            redrawfield();
            break;

        case WM_LBUTTONUP:
            if(!mouseclick)break;
            manager_hitscan(manager_g,x,y,&floating_itembar,&i);
            if(floating_itembar==SLOT_SNAPSHOT)
            {
                statemode=0;
                PostMessage(hMain,WM_DEVICECHANGE,7,2);
            }
            if(floating_itembar==SLOT_DPRDIR)
            {
                *drpext_dir=0;
                PostMessage(hMain,WM_DEVICECHANGE,7,2);
            }
            if(floating_itembar==SLOT_EXTRACTING)
            {
                if(installmode==MODE_INSTALLING)
                    installmode=MODE_STOPPING;
                else if(installmode==MODE_NONE)
                    manager_clear(manager_g);
            }
            if(floating_itembar>=0&&(i==1||i==0))
            {
                manager_toggle(manager_g,floating_itembar);
                if(wParam&MK_SHIFT&&installmode==MODE_NONE)
                {
                    if((flags&FLAG_EXTRACTONLY)==0)
                    wsprintf(extractdir,L"%s\\SDI",manager_g->matcher->state->text+manager_g->matcher->state->temp);
                    manager_install(INSTALLDRIVERS);
                }
                redrawfield();
            }
            if(floating_itembar>=0&&i==2)
            {
                manager_expand(manager_g,floating_itembar);
                manager_setpos(manager_g);
            }
            break;

        case WM_RBUTTONDOWN:
            manager_hitscan(manager_g,x,y,&floating_itembar,&i);
            if(floating_itembar>=0&&i==0)
                contextmenu(x,y);
            break;

        case WM_MBUTTONDOWN:
            mousedown=3;
            mousex=x;
            mousey=y;
            SetCursor(LoadCursor(0,IDC_SIZEALL));
            SetCapture(hwnd);
            break;

        case WM_MOUSEMOVE:
            si.cbSize=sizeof(si);
            if(mousedown==3)
            {
                si.fMask=SIF_ALL;
                si.nPos=0;
                GetScrollInfo(hwnd,SB_VERT,&si);
                si.nPos+=mousey-y;
                si.fMask=SIF_POS;
                SetScrollInfo(hwnd,SB_VERT,&si,TRUE);

                mousex=x;
                mousey=y;
                redrawfield();
            }
            {
                int type=FLOATING_NONE;
                int itembar_i;

                if(space_down)type=FLOATING_DRIVERLST;else
                if(ctrl_down||expertmode)type=FLOATING_CMPDRIVER;

                manager_hitscan(manager_g,x,y,&itembar_i,&i);
                if(i==0&&itembar_i>=RES_SLOTS&&(ctrl_down||space_down||expertmode))
                    drawpopup(itembar_i,type,x,y,hField);
                else if(itembar_i==SLOT_VIRUS_AUTORUN)
                    drawpopup(STR_VIRUS_AUTORUN_H,FLOATING_TOOLTIP,x,y,hField);
                else if(itembar_i==SLOT_VIRUS_RECYCLER)
                    drawpopup(STR_VIRUS_RECYCLER_H,FLOATING_TOOLTIP,x,y,hField);
                else if(itembar_i==SLOT_VIRUS_HIDDEN)
                    drawpopup(STR_VIRUS_HIDDEN_H,FLOATING_TOOLTIP,x,y,hField);
                else if(itembar_i==SLOT_RESTORE_POINT)
                    drawpopup(STR_RESTOREPOINT_H,FLOATING_TOOLTIP,x,y,hField);
                else if(i==0&&itembar_i>=RES_SLOTS)
                    drawpopup(STR_HINT_DRIVER,FLOATING_TOOLTIP,x,y,hField);
                else
                    drawpopup(-1,FLOATING_NONE,0,0,hField);

                if(itembar_i!=field_lasti||i!=field_lastz)redrawfield();
                field_lasti=itembar_i;
                field_lastz=i;
            }
            break;

        default:
            i=DefWindowProc(hwnd,message,wParam,lParam);
            checktimer(L"ListD",timer,message);
            return i;
    }
    checktimer(L"List",timer,message);
    return 0;
}

LRESULT CALLBACK PopupProcedure(HWND hwnd,UINT message,WPARAM wParam,LPARAM lParam)
{
    HDC hdcMem;
    RECT rect;
    WINDOWPOS *wp;

    switch(message)
    {
        case WM_WINDOWPOSCHANGING:
            if(floating_type!=FLOATING_TOOLTIP)break;

            wp=(WINDOWPOS*)lParam;
            hdcMem=GetDC(hwnd);
            GetClientRect(hwnd,&rect);
            rect.right=D(POPUP_WX);
            rect.bottom=floating_y;
            SelectObject(hdcMem,hFont);
            DrawText(hdcMem,STR(floating_itembar),-1,&rect,DT_WORDBREAK|DT_CALCRECT);
            AdjustWindowRectEx(&rect,WS_POPUPWINDOW|WS_VISIBLE,0,0);
            wp->cx=rect.right+D(POPUP_OFSX)*2;
            wp->cy=rect.bottom+D(POPUP_OFSY)*2;
            ReleaseDC(hwnd,hdcMem);
            break;

        case WM_CREATE:
            canvas_init(&canvasPopup);
            break;

        case WM_DESTROY:
            canvas_free(&canvasPopup);
            break;

        case WM_PAINT:
            GetClientRect(hwnd,&rect);
            canvas_begin(&canvasPopup,hwnd,rect.right,rect.bottom);

            //log_con("Redraw Popup %d\n",cntd++);
            box_draw(canvasPopup.hdcMem,0,0,rect.right,rect.bottom,BOX_POPUP);
            switch(floating_type)
            {
                case FLOATING_SYSINFO:
                    SelectObject(canvasPopup.hdcMem,hFont);
                    popup_sysinfo(manager_g,canvasPopup.hdcMem);
                    break;

                case FLOATING_TOOLTIP:
                    rect.left+=D(POPUP_OFSX);
                    rect.top+=D(POPUP_OFSY);
                    rect.right-=D(POPUP_OFSX);
                    rect.bottom-=D(POPUP_OFSY);
                    SelectObject(canvasPopup.hdcMem,hFont);
                    SetTextColor(canvasPopup.hdcMem,D(POPUP_TEXT_COLOR));
                    DrawText(canvasPopup.hdcMem,STR(floating_itembar),-1,&rect,DT_WORDBREAK);
                    break;

                case FLOATING_CMPDRIVER:
                    SelectObject(canvasPopup.hdcMem,hFont);
                    popup_drivercmp(manager_g,canvasPopup.hdcMem,rect,floating_itembar);
                    break;

                case FLOATING_DRIVERLST:
                    SelectObject(canvasPopup.hdcMem,hFont);
                    popup_driverlist(manager_g,canvasPopup.hdcMem,rect,floating_itembar);
                    break;

                case FLOATING_ABOUT:
                    SelectObject(canvasPopup.hdcMem,hFont);
                    popup_about(canvasPopup.hdcMem);
                    break;

                default:
                    break;
            }

            canvas_end(&canvasPopup);
            break;

        case WM_ERASEBKGND:
            return 1;

        default:
            return DefWindowProc(hwnd,message,wParam,lParam);
    }
    return 0;
}

BOOL CALLBACK LicenseProcedure(HWND hwnd,UINT Message,WPARAM wParam,LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);

    HWND hEditBox;
    void *s;int sz;

    switch(Message)
    {
        case WM_INITDIALOG:
            get_resource(IDR_LICENSE,&s,&sz);
            hEditBox=GetDlgItem(hwnd,IDC_EDIT1);
            SetWindowTextA(hEditBox,s);
            SendMessage(hEditBox,EM_SETREADONLY,1,0);
            return TRUE;

        case WM_COMMAND:
            switch(LOWORD(wParam))
            {
                case IDOK:
                    license=1;
                    EndDialog(hwnd,IDOK);
                    return TRUE;

                case IDCANCEL:
                    license=0;
                    EndDialog(hwnd,IDCANCEL);
                    return TRUE;

                default:
                    break;
            }
            break;

        default:
            break;;
    }
    return FALSE;
}
//}
