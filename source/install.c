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
long long total;
int itembar_act;
WCHAR extractdir[BUFLEN];
int instflag;
int needreboot=0;

// Clicker
const wnddata_t clicktbl[NUM_CLICKDATA]=
{
    // Windows XP
    {
        396,315,
        390,283,
#ifdef AUTOCLICKER_CONFIRM
        107,249,// continue
#else
        245,249,  // stop
#endif
        132,23
    },
    // Windows 7 and Windows 8.1
    {
        500,270,
        500,270,
#ifdef AUTOCLICKER_CONFIRM
        47,139,  // continue
        448,87   // continue
#else
        47,67,     // stop
        448,72     // stop
#endif
    }
};
volatile int clicker_flag;
//}

void _7z_total(long long i)
{
    total=i;
}

#define S_OK    ((HRESULT)0x00000000L)
#define E_ABORT ((HRESULT)0x80004004L)

int _7z_setcomplited(long long i)
{
    int j;
    int _totalitems=0;
    int _processeditems=0;

    if(statemode==STATEMODE_EXIT)return S_OK;
    itembar_t *itembar=manager_g->items_list;
    if(installmode==MODE_STOPPING)return E_ABORT;
    if(!manager_g->items_list[itembar_act].checked)return E_ABORT;
    for(j=0;j<manager_g->items_handle.items;j++,itembar++)
    if(j>=RES_SLOTS)
    {
        if(itembar->checked||itembar->install_status){_totalitems++;}
        if(itembar->install_status&&!itembar->checked){_processeditems++;}
    }

    itembar_settext(manager_g,itembar_act,L"",(int)(i*(instflag&INSTALLDRIVERS?900.:1000.)/total));
    double d=(manager_g->items_list[itembar_act].percent)/_totalitems;
    itembar_settext(manager_g,SLOT_EXTRACTING,L"",(int)(_processeditems*1000./_totalitems+d));
    manager_g->items_list[SLOT_EXTRACTING].val1=_processeditems;
    manager_g->items_list[SLOT_EXTRACTING].val2=_totalitems;
    return S_OK;
}

void driver_install(WCHAR *hwid,WCHAR *inf,int *ret,int *needrb)
{
    WCHAR cmd[BUFLEN];
    WCHAR buf[BUFLEN];
    void *install64bin;
    HANDLE thr;
    int size;
    FILE *f;

    *ret=3;*needrb=0;
    wsprintf(cmd,L"%s\\install64.exe",extractdir);
    if(!PathFileExists(cmd))
    {
        mkdir_r(extractdir);
        log_err("Dir: (%ws)\n",extractdir);
        f=_wfopen(cmd,L"wb");
        if(f)
            log_err("Created '%ws'\n",cmd);
        else
            log_err("Failed to create '%ws'\n",cmd);
        get_resource(IDR_INSTALL64,&install64bin,&size);
        fwrite(install64bin,1,size,f);
        fclose(f);
    }

    clicker_flag=1;
    thr=(HANDLE)_beginthreadex(0,0,&thread_clicker,0,0,0);
    {
        if(flags&FLAG_DISABLEINSTALL)
            Sleep(2000);
        else
            *ret=UpdateDriverForPlugAndPlayDevices(0,hwid,inf,INSTALLFLAG_FORCE,needrb);
    }

    if(!*ret)*ret=GetLastError();
    if((unsigned)*ret==0xE0000235)//ERROR_IN_WOW64
    {
        wsprintf(buf,L"\"%s\" \"%s\"",hwid,inf);
        wsprintf(cmd,L"%s\\install64.exe",extractdir);
        log_err("'%ws %ws'\n",cmd,buf);
        *ret=RunSilent(cmd,buf,SW_HIDE,1);
        if((*ret&0x7FFFFFFF)==1)
        {
            *needrb=*ret&0x80000000?1:0;
            *ret&=~0x80000000;
        }
    }
    clicker_flag=0;
    WaitForSingleObject(thr,INFINITE);
    CloseHandle_log(thr,L"driver_install",L"thr");
}

unsigned int __stdcall thread_install(void *arg)
{
    UNREFERENCED_PARAMETER(arg)

    itembar_t *itembar,*itembar1;
    WCHAR cmd[BUFLEN];
    WCHAR hwid[BUFLEN];
    WCHAR inf[BUFLEN];
    WCHAR buf[BUFLEN];
    int i,j;
    RESTOREPOINTINFOW pRestorePtSpec;
    STATEMGRSTATUS pSMgrStatus;
    HINSTANCE hinstLib=0;
    MYPROC ProcAdd;
    manager_t *manager=manager_g;
    int r=0;

    // Prepare extract dir
    installmode=MODE_INSTALLING;
    manager->items_list[SLOT_EXTRACTING].isactive=1;
    manager_setpos(manager);

    // Restore point
    if(manager->items_list[SLOT_RESTORE_POINT].checked)
    {
        hinstLib=LoadLibrary(L"SrClient.dll");
        ProcAdd=(MYPROC)GetProcAddress(hinstLib,"SRSetRestorePointW");

        if(hinstLib&&ProcAdd)
        {
            manager_g->items_list[SLOT_RESTORE_POINT].percent=500;
            manager_g->items_list[SLOT_RESTORE_POINT].install_status=STR_REST_CREATING;
            itembar_act=SLOT_RESTORE_POINT;
            redrawfield();

            memset(&pRestorePtSpec,0,sizeof(RESTOREPOINTINFOW));
            pRestorePtSpec.dwEventType=BEGIN_SYSTEM_CHANGE;
            pRestorePtSpec.dwRestorePtType=DEVICE_DRIVER_INSTALL;
            wcscpy(pRestorePtSpec.szDescription,L"Installed drivers");
            r=0;
            if(flags&FLAG_DISABLEINSTALL)
                Sleep(2000);
            else
                r=ProcAdd(&pRestorePtSpec,&pSMgrStatus);

            log_err("rt rest point{ %d(%d)\n",r,pSMgrStatus.nStatus);
            manager_g->items_list[SLOT_RESTORE_POINT].percent=1000;
            if(r)
            {
                manager_g->items_list[SLOT_RESTORE_POINT].install_status=STR_REST_CREATED;

            }else
            {
                manager_g->items_list[SLOT_RESTORE_POINT].install_status=STR_REST_FAILED;
                log_err("ERROR in thread_install: Failed to create restore point\n");
            }
        }
        else
        {
            manager_g->items_list[SLOT_RESTORE_POINT].install_status=STR_REST_FAILED;
            log_err("ERROR in thread_install: Failed to create restore point %d,%d\n",hinstLib,ProcAdd);
        }
        redrawfield();
        if(hinstLib)FreeLibrary(hinstLib);
        manager->items_list[SLOT_RESTORE_POINT].checked=0;
    }
goaround:
    itembar=manager->items_list;
    for(i=0;i<manager->items_handle.items&&installmode==MODE_INSTALLING;i++,itembar++)
        if(i>=RES_SLOTS&&itembar->checked&&itembar->isactive&&itembar->hwidmatch)
    {
        int unpacked=0;
        int limits[7];

        memset(limits,0,sizeof(limits));
        itembar_act=i;
        hwidmatch_t *hwidmatch=itembar->hwidmatch;
        log_err("Installing $%04d\n",i);
        hwidmatch_print(hwidmatch,limits);
        wsprintf(cmd,L"%s\\%S",extractdir,getdrp_infpath(hwidmatch));

        // Extract
        if(PathFileExists(cmd))
        {
            log_err("Already unpacked\n");
            _7z_total(100);
            _7z_setcomplited(100);
            redrawfield();
        }
        else
        if(wcsstr(getdrp_packname(hwidmatch),L"unpacked.7z"))
        {
            printf("Unpacked '%ws'\n",getdrp_packpath(hwidmatch));
            unpacked=1;
        }
        else
        {
            wsprintf(cmd,L"app x -y \"%s\\%s\" -o\"%s\"",getdrp_packpath(hwidmatch),getdrp_packname(hwidmatch),
                    extractdir,
                    getdrp_infpath(hwidmatch));

            itembar1=itembar;
            for(j=i;j<manager->items_handle.items;j++,itembar1++)
                if(itembar1->checked&&
                   !wcscmp(getdrp_packpath(hwidmatch),getdrp_packpath(itembar1->hwidmatch))&&
                   !wcscmp(getdrp_packname(hwidmatch),getdrp_packname(itembar1->hwidmatch)))
            {
                wsprintf(buf,L" \"%S\"",getdrp_infpath(itembar1->hwidmatch));
                if(!wcsstr(cmd,buf))wcscat(cmd,buf);
            }
            log_err("Extracting via '%ws'\n",cmd);
            itembar->install_status=STR_INST_EXTRACT;
            redrawfield();
            r=Extract7z(cmd);
            log_err("Ret %d\n",r);
        }

        // Install driver
        if(instflag&INSTALLDRIVERS&&itembar->checked)
        {
            int needrb=0,ret=1;
            wsprintf(inf,L"%s\\%S%S",
                   unpacked?getdrp_packpath(hwidmatch):extractdir,
                   getdrp_infpath(hwidmatch),
                   getdrp_infname(hwidmatch));
            wsprintf(hwid,L"%S",getdrp_drvHWID(hwidmatch));
            log_err("Install32 '%ws','%ws'\n",hwid,inf);
            itembar->install_status=STR_INST_INSTALL;
            redrawfield();

            if(installmode==MODE_INSTALLING)
                driver_install(hwid,inf,&ret,&needrb);
            else
                ret=1;

            log_err("Ret %d(%X),%d\n\n",ret,ret,needrb);

            if(ret==1)
            {
                if(needrb)
                    itembar->install_status=STR_INST_REBOOT;
                else
                    itembar->install_status=STR_INST_OK;
            }
            else
            {
                itembar->install_status=STR_INST_FAILED;
                itembar->val1=ret;
            }

            if(needrb)needreboot=1;
            if(installmode==MODE_STOPPING||!itembar->checked)
            {
                itembar->percent=0;
                itembar->checked=0;
                itembar->install_status=0;
            }
            else
            {
                itembar->checked=0;
                itembar->percent=0;
            }
            redrawfield();
        }
        else
        {
            itembar->checked=0;
            itembar->install_status=0;
            itembar->percent=0;
            redrawfield();
        }
    }
    if(installmode==MODE_INSTALLING)
    {
        itembar=manager_g->items_list;
        for(j=0;j<manager_g->items_handle.items;j++,itembar++)
            if(j>=RES_SLOTS&&itembar->checked)
                goto goaround;
    }
    // Instalation competed by this point

    if(instflag&OPENFOLDER)
    {
        WCHAR *p=extractdir+wcslen(extractdir);
        while(*(--p)!='\\');
        *p=0;
        log_err("%ws\n",extractdir);
        ShellExecute(0,L"explore",extractdir,0,0,SW_SHOW);
        manager->items_list[SLOT_EXTRACTING].isactive=0;
        manager_setpos(manager);
    }
    if(instflag&INSTALLDRIVERS)
    {
        wsprintf(buf,L" /c rd /s /q \"%s\"",extractdir);
        RunSilent(L"cmd",buf,SW_HIDE,1);
    }

    if(installmode==MODE_STOPPING)
    {
        flags&=~FLAG_AUTOINSTALL;
        installmode=MODE_NONE;
    }
    if(installmode==MODE_INSTALLING)installmode=MODE_SCANNING;
    log_err("Mode:%d\n",installmode);
    PostMessage(hMain,WM_DEVICECHANGE,7,0);
    manager->items_list[SLOT_EXTRACTING].percent=1000;
    redrawfield();

    return 0;
}

void manager_install(int flagsv)
{
    instflag=flagsv;
    _beginthreadex(0,0,&thread_install,0,0,0);
}

void calcwnddata(wnddata_t *w,HWND hwnd)
{
    WINDOWINFO pwi,pwb;
    HWND parent=GetParent(hwnd);
    pwb.cbSize=pwi.cbSize=sizeof(WINDOWINFO);

    GetWindowInfo(parent,&pwi);
    w->wnd_wx=pwi.rcWindow.right-pwi.rcWindow.left;
    w->wnd_wy=pwi.rcWindow.bottom-pwi.rcWindow.top;
    w->cln_wx=pwi.rcClient.right-pwi.rcClient.left;
    w->cln_wy=pwi.rcClient.bottom-pwi.rcClient.top;

    GetWindowInfo(hwnd,&pwb);
    w->btn_x =pwb.rcClient.left-pwi.rcClient.left;
    w->btn_y =pwb.rcClient.top-pwi.rcClient.top;
    w->btn_wx=pwb.rcClient.right-pwb.rcClient.left;
    w->btn_wy=pwb.rcClient.bottom-pwb.rcClient.top;
}

BOOL CALLBACK EnumWindowsProc(HWND hwnd,LPARAM lParam)
{
    WCHAR buf[BUFLEN];
    WINDOWINFO pwi;
    wnddata_t w;
    int i;

    pwi.cbSize=sizeof(WINDOWINFO);
    GetWindowInfo(hwnd,&pwi);

    if(lParam&2)
    {
        GetWindowText(hwnd,buf,BUFLEN);
        log("Window %06X,%06X '%ws'\n",hwnd,GetParent(hwnd),buf);
        GetClassName(hwnd,buf,BUFLEN);
        log("Class: '%ws'\n",buf);
        RealGetWindowClass(hwnd,buf,BUFLEN);
        log("RealClass: '%ws'\n",buf);
        log("\n");
    }

    if((lParam&1)==1)
    {
        if(lParam&2)
        {
            log("* MainWindow (%d,%d) (%d,%d)\n",w.wnd_wx,w.wnd_wy,w.cln_wx,w.cln_wy);
            log("* Child (%d,%d,%d,%d)\n",w.btn_x,w.btn_y,w.btn_wx,w.btn_wy);
            log("\n");
        }
        calcwnddata(&w,hwnd);

        if((lParam&2)==0)for(i=0;i<NUM_CLICKDATA;i++)
            if(!memcmp(&w,&clicktbl[i],sizeof(wnddata_t)))
        {
            SwitchToThisWindow(hwnd,0);
            GetWindowInfo(GetParent(hwnd),&pwi);
            SetCursorPos(pwi.rcClient.left+w.btn_x+w.btn_wx/2,pwi.rcClient.top+w.btn_y+w.btn_wy/2);
            Sleep(3000);
            if(IsWindow(hwnd))
            {
                GetWindowInfo(hwnd,&pwi);
                calcwnddata(&w,hwnd);

                if(!memcmp(&w,&clicktbl[i],sizeof(wnddata_t)))
                {
                    //SetActiveWindow(hwnd);
                    //SendMessage(hwnd,BM_CLICK,0,0);

                    GetWindowInfo(GetParent(hwnd),&pwi);
                    SetCursorPos(pwi.rcClient.left+w.btn_x+w.btn_wx/2,pwi.rcClient.top+w.btn_y+w.btn_wy/2);
                    int x=w.btn_x+w.btn_wx/2;
                    int y=w.btn_y+w.btn_wy/2;
                    int pos=(int)((y<<16)|x);
                    SendMessage(GetParent(hwnd),WM_LBUTTONDOWN,0,pos);
                    SendMessage(GetParent(hwnd),WM_LBUTTONUP,  0,pos);
                    log_err("Autoclicker fired\n");
                }
            }
        }
    }

    if((lParam&1)==0)
        EnumChildWindows(hwnd,EnumWindowsProc,lParam|1);

    return 1;
}

void wndclicker(int mode)
{
    EnumChildWindows(GetDesktopWindow(),EnumWindowsProc,mode);
}

unsigned int __stdcall thread_clicker(void *arg)
{
    UNREFERENCED_PARAMETER(arg);

    while(clicker_flag)
    {
        EnumChildWindows(GetDesktopWindow(),EnumWindowsProc,0);
        Sleep(100);
    }
    return 0;
}
