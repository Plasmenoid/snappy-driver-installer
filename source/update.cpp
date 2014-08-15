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

#ifndef _WIN64

#include "libtorrent/config.hpp"
#include "libtorrent/entry.hpp"
#include "libtorrent/bencode.hpp"
#include "libtorrent/alert_types.hpp"
#include "libtorrent/session.hpp"

#define TORRENT_URL "http://driveroff.net/SDI_Update.torrent"
#define SMOOTHING_FACTOR 0.005
using namespace libtorrent;

extern "C"
{
#define _WIN32_IE 0x0400
#include "main.h"
}

int cxn[]=
{
    199,
    60,
    44,
    70,
    70,
    90,
};

session *sessionhandle=0;
torrent_handle updatehandle;

volatile int downloadmangar_exitflag=0;
HANDLE downloadmangar_event;
HANDLE thandle_download;
long long torrenttime=0;

session_settings settings;
dht_settings dht;

int totalsize=0,numfiles;
HWND hUpdate=0;
int bMouseInWindow=0;
WNDPROC wpOrigButtonProc;

int finisheddownloading=0,finishedupdating=0;
int averageSpeed;
torrent_status_t torrentstatus;

int yes1(libtorrent::torrent_status const&);

int getnewver(const char *ptr)
{
    char bff[BUFLEN];
    char *s=bff;

    strcpy(bff,ptr);
    while(*s)
    {
        if(*s=='_'&&s[1]>='0'&&s[1]<='9')
            return atoi(s+1);

        s++;
    }
    return 0;
}

int getcurver(const char *ptr)
{
    WCHAR bffw[BUFLEN];
    WCHAR *s=bffw;

    wsprintf(bffw,L"%S",ptr);
    while(*s)
    {
        if(*s=='_'&&s[1]>='0'&&s[1]<='9')
        {
            *s=0;
            s=finddrp(bffw);
            if(!s)return 0;
            while(*s)
            {
                if(*s==L'_'&&s[1]>=L'0'&&s[1]<=L'9')
                    return _wtoi(s+1);

                s++;
            }
            return 0;
        }
        s++;
    }
    return 0;
}

void delolddrp(const char *ptr)
{
    WCHAR bffw[BUFLEN];
    WCHAR buf[BUFLEN];
    WCHAR *s=bffw;

    wsprintf(bffw,L"%S",ptr);
    log_con("dep '%ws'\n",bffw);
    while(*s)
    {
        if(*s=='_'&&s[1]>='0'&&s[1]<='9')
        {
            *s=0;
            s=finddrp(bffw);
            if(!s)return;
            wsprintf(buf,L"drivers\\%s",s);
            log_con("'del %ws'\n",buf);
            _wremove(buf);
            return;
        }
        s++;
    }
}

void upddlg_updatelang()
{
    LVCOLUMN lvc;
    WCHAR buf[BUFLEN];
    int i;
    HWND hwnd=hUpdate;
    if(!hwnd)return;

    wsprintf(buf,STR(STR_UPD_TOTALSIZE),totalsize);

    SetWindowText(hwnd,STR(STR_UPD_TITLE));
    SetWindowText(GetDlgItem(hwnd,IDCHECKALL),STR(STR_UPD_BTN_ALL));
    SetWindowText(GetDlgItem(hwnd,IDUNCHECKALL),STR(STR_UPD_BTN_NONE));
    SetWindowText(GetDlgItem(hwnd,IDCHECKTHISPC),STR(STR_UPD_BTN_THISPC));
    SetWindowText(GetDlgItem(hwnd,IDOK),STR(STR_UPD_BTN_OK));
    SetWindowText(GetDlgItem(hwnd,IDCANCEL),STR(STR_UPD_BTN_CANCEL));
    SetWindowText(GetDlgItem(hwnd,IDACCEPT),STR(STR_UPD_BTN_ACCEPT));
    SetWindowText(GetDlgItem(hwnd,IDTOTALSIZE),buf);

    lvc.mask=LVCF_TEXT;
    for(i=0;i<6;i++)
    {
        lvc.pszText=(WCHAR *)STR(STR_UPD_COL_NAME+i);
        ListView_SetColumn(GetDlgItem(hwnd,IDLIST),i,&lvc);
    }
}

//}

void update_movefiles()
{
    file_entry fe;
    int i;
    boost::intrusive_ptr<torrent_info const> ti;
    const char *filenamefull;
    WCHAR buf [BUFLEN];
    WCHAR filenamefull_src[BUFLEN];
    WCHAR buf2[BUFLEN];
    WCHAR buf3[BUFLEN],*p;

    ti=updatehandle.torrent_file();

    monitor_pause=1;
    for(i=0;i<numfiles;i++)
        if(updatehandle.file_priority(i)&&
           StrStrIA(ti->file_at(i).path.c_str(),"indexes\\SDI"))
            break;
    log_con("New index files: %d\n",i==numfiles);
    if(i!=numfiles)
    {
        wsprintf(buf,L"/c del %ws\\_*.bin",index_dir);
        RunSilent(L"cmd",buf,SW_HIDE,1);
    }

    for(i=0;i<numfiles;i++)
    if(updatehandle.file_priority(i))
    {
        fe=ti->file_at(i);
        filenamefull=strchr(fe.path.c_str(),'\\')+1;
        wsprintf(filenamefull_src,L"update\\%S",fe.path.c_str());
        wsprintf(buf2,L"%S",filenamefull);
        //wsprintf(buf2,L"res\\%S",filename);
        wsprintf(buf3,L"%s",buf2);
        p=wcsstr(buf2,L"indexes\\SDI\\");

        if(StrStrIA(filenamefull,"drivers\\"))delolddrp(filenamefull+8);

        if(p)
        {
            //CopyFile(filenamefull_src,buf2,0);
            p[12]=L'_';
        }
        p=buf3;
        while(wcschr(p,L'\\'))p=wcschr(p,L'\\')+1;
        //log_con("Complited '%s' [%ws][%ws]{%ws}\n",filename,buf1,buf2,buf3);
        if(p[-1]==L'\\')
        {
            *--p=0;
            mkdir_r(buf3);
        }
        //CopyFile(buf1,buf2,0);
        log_con("File '%ws'\n",buf2);
        if(!StrStrIA(filenamefull,"autorun.inf")&&
           !StrStrIA(filenamefull,".bat"))
        {
            int r=MoveFileEx(filenamefull_src,buf2,MOVEFILE_REPLACE_EXISTING);
            if(!r)log_err("ERROR in MoveFile:%d\n",GetLastError());
        }
    }
    RunSilent(L"cmd",L" /c rd /s /q update",SW_HIDE,1);
}

unsigned int __stdcall thread_download(void *arg)
{
    UNREFERENCED_PARAMETER(arg)

    log_con("{thread_download\n");

    WaitForSingleObject(downloadmangar_event,INFINITE);
    if(downloadmangar_exitflag)return 0;

    update_start();
    update_getstatus(&torrentstatus);

    ResetEvent(downloadmangar_event);
    while(!downloadmangar_exitflag)
    {
        WaitForSingleObject(downloadmangar_event,INFINITE);
        if(downloadmangar_exitflag)break;
        log_con("{torrent_start\n");

        while(sessionhandle)
        {
            update_getstatus(&torrentstatus);
            InvalidateRect(hPopup,0,0);
            Sleep(500);
            if(downloadmangar_exitflag)break;
            if(finisheddownloading)
            {
                log_con("-torrent_finished\n");
                sessionhandle->pause();

                log_con("Flushing cache...");
                updatehandle.flush_cache();
                while(1)
                {
                    alert const* a=sessionhandle->wait_for_alert(seconds(60*2));
                    if(!a)
                    {
                        log_con("time out\n");
                        break;
                    }
                    std::auto_ptr<alert> holder=sessionhandle->pop_alert();
                    if(alert_cast<cache_flushed_alert>(a))
                    {
                        log_con("done\n");
                        break;
                    }
                }

                update_movefiles();
                updatehandle.force_recheck();
                upddlg_populatelist(0);
                break;
            }
        }
        finishedupdating=1;
        sessionhandle->pause();
        update_getstatus(&torrentstatus);
        upddlg_populatelist(0);
        monitor_pause=0;
        PostMessage(hMain,WM_DEVICECHANGE,7,2);
        log_con("}torrent_stop\n");
        ResetEvent(downloadmangar_event);
    }
    log_con("}thread_download\n");
    return 0;
}

int CALLBACK CompareFunc(LPARAM lParam1,LPARAM lParam2,LPARAM lParamSort)
{
    UNREFERENCED_PARAMETER(lParamSort)
    return lParam1-lParam2;
}

int upddlg_populatelist(HWND hList)
{
    error_code ec;
    file_entry fe;
    LVITEM lvI;
    int i;
    int basesize=0,basedownloaded=0;
    int indexsize=0,indexdownloaded=0;
    const char *filename,*filenamefull;
    WCHAR buf[BUFLEN];
    int newver=0;
    int ret=0;
    int missingindexes=0;

    boost::intrusive_ptr<torrent_info const> ti;
    std::vector<size_type> file_progress;
    ti=updatehandle.torrent_file();
    numfiles=0;
    if(!ti)return 0;
    updatehandle.file_progress(file_progress);

    numfiles=ti->num_files();
    for(i=0;i<numfiles;i++)
    {
        fe=ti->file_at(i);
        filenamefull=strchr(fe.path.c_str(),'\\')+1;

        if(StrStrIA(filenamefull,"indexes\\"))
        {
            indexsize+=fe.size;
            indexdownloaded+=file_progress[i];
            wsprintf(buf,L"%S",filenamefull);
            *wcsstr(buf,L"DP_")=L'_';
            if(!PathFileExists(buf))  //hardcoded path
            {
                log_con("Missing index: '%ws'\n",buf);
                missingindexes=1;
            }
        }
        else if(!StrStrIA(filenamefull,"drivers\\"))
        {
            basesize+=fe.size;
            basedownloaded+=file_progress[i];
            if(StrStrIA(filenamefull,"sdi_R"))
                newver=atol(StrStrIA(filenamefull,"sdi_R")+5);
        }
    }

    lvI.mask      =LVIF_TEXT|LVIF_STATE|LVIF_PARAM;
    lvI.stateMask =0;
    lvI.iSubItem  =0;
    lvI.state     =0;
    lvI.iItem     =0;
    lvI.lParam    =-2;
    //newver=200;
    if(newver>SVN_REV)ret+=newver<<8;
    if(newver>SVN_REV&&hList)
    {
        lvI.pszText=(WCHAR *)STR(STR_UPD_APP);
        i=ListView_InsertItem(hList,&lvI);
        wsprintf(buf,L"%d %s",basesize/1024/1024,STR(STR_UPD_MB));
        ListView_SetItemText(hList,i,1,buf);
        wsprintf(buf,L"%d%%",basedownloaded*100/basesize);
        ListView_SetItemText(hList,i,2,buf);
        wsprintf(buf,L" SDI_R%d",newver);
        ListView_SetItemText(hList,i,3,buf);
        wsprintf(buf,L" SDI_R%d",SVN_REV);
        ListView_SetItemText(hList,i,4,buf);
        ListView_SetItemText(hList,i,5,(WCHAR *)STR(STR_UPD_YES));
    }

    lvI.lParam    =-1;
    if(missingindexes&&hList)
    {
        lvI.pszText=(WCHAR *)STR(STR_UPD_INDEXES);
        i=ListView_InsertItem(hList,&lvI);
        wsprintf(buf,L"%d %s",indexsize/1024/1024,STR(STR_UPD_MB));
        ListView_SetItemText(hList,i,1,buf);
        wsprintf(buf,L"%d%%",indexdownloaded*100/indexsize);
        ListView_SetItemText(hList,i,2,buf);
        ListView_SetItemText(hList,i,5,(WCHAR *)STR(STR_UPD_YES));
    }

    for(i=0;i<numfiles;i++)
    {
        fe=ti->file_at(i);
        filenamefull=strchr(fe.path.c_str(),'\\')+1;
        filename=strchr(filenamefull,'\\')+1;
        if(!filename)filename=filenamefull;

        if(StrStrIA(filenamefull,".7z"))
        {
            int oldver;

            wsprintf(buf,L"%S",filename);
            lvI.pszText=buf;
            int sz=fe.size/1024/1024;
            if(!sz)sz=1;

            newver=getnewver(filenamefull);
            oldver=getcurver(filename);

            if(newver>oldver)ret++;
            if(newver>oldver&&hList)
            {
                lvI.lParam=i;
                int j=ListView_InsertItem(hList,&lvI);
                wsprintf(buf,L"%d %s",sz,STR(STR_UPD_MB));
                ListView_SetItemText(hList,j,1,buf);
                wsprintf(buf,L"%d%%",file_progress[i]*100/ti->file_at(i).size);
                ListView_SetItemText(hList,j,2,buf);
                wsprintf(buf,L"%d",newver);
                ListView_SetItemText(hList,j,3,buf);
                wsprintf(buf,L"%d",oldver);
                if(!oldver)wsprintf(buf,L"%ws",STR(STR_UPD_MISSING));
                ListView_SetItemText(hList,j,4,buf);
                wsprintf(buf,L"%S",filename);
                wsprintf(buf,L"%ws",STR(STR_UPD_YES+manager_drplive(buf)));
                ListView_SetItemText(hList,j,5,buf);
            }
        }
    }
    ListView_SortItems(hList,CompareFunc,0);

    log_con("Latest version: R%d\nUpdated driverpacks available: %d\n",ret>>8,ret&0xFF);
    manager_g->items_list[SLOT_DOWNLOAD].isactive=ret?1:0;
    manager_setpos(manager_g);
    manager_g->items_list[SLOT_DOWNLOAD].val1=ret;
    return ret;
}

int yes1(libtorrent::torrent_status const&){return true;}

void update_start()
{
    error_code ec;
    int i;
    add_torrent_params params;

    time_chkupdate=GetTickCount();
    sessionhandle=new session();

    sessionhandle->start_lsd();
    sessionhandle->start_upnp();
    sessionhandle->start_natpmp();
    int port=50171;
    //port=0;

    sessionhandle->listen_on(std::make_pair(port,port),ec);
    if(ec)
    {
        log_con("failed to open listen socket: %s\n",ec.message().c_str());
    }
    log_con("Listen %d,%d\n",sessionhandle->is_listening(),sessionhandle->listen_port());

    dht.privacy_lookups=true;
    sessionhandle->set_dht_settings(dht);
    settings.use_dht_as_fallback = false;
    sessionhandle->add_dht_router(std::make_pair(std::string("router.bittorrent.com"),port));
    sessionhandle->add_dht_router(std::make_pair(std::string("router.utorrent.com"),port));
    sessionhandle->add_dht_router(std::make_pair(std::string("router.bitcomet.com"),port));
    sessionhandle->start_dht();
    sessionhandle->set_alert_mask(alert::error_notification|alert::storage_notification);

    settings.user_agent = "Snappy Driver Installer/" SVN_REV2;
    settings.always_send_user_agent=true;
    settings.anonymous_mode=false;
    settings.choking_algorithm = session_settings::auto_expand_choker;
    settings.disk_cache_algorithm = session_settings::avoid_readback;
    settings.volatile_read_cache = false;
    sessionhandle->set_settings(settings);

    params.save_path="update";
    params.url=TORRENT_URL;
    if(ec)
    {
        log_con("failed to init torrentinfo: %s\n",ec.message().c_str());
    }
    params.flags=add_torrent_params::flag_paused;
    updatehandle=sessionhandle->add_torrent(params,ec);
    if(ec)
    {
        log_con("failed to add torrent: %s\n",ec.message().c_str());
    }
    sessionhandle->pause();
    updatehandle.resume();
    log_con("Waiting for torrent");
    for(i=0;i<100;i++)
    {
        log_con(".");
        if(updatehandle.torrent_file())
        {
            log_con("DONE\n");
            break;
        }
        if(downloadmangar_exitflag)break;
        Sleep(100);
    }
    log_con(updatehandle.torrent_file()?"":"FAILED\n");
    upddlg_populatelist(0);
    for(i=0;i<numfiles;i++)updatehandle.file_priority(i,0);

    time_chkupdate=GetTickCount()-time_chkupdate;
}

void update_getstatus(torrent_status_t *t)
{
    std::vector<torrent_status> temp;

    memset(t,0,sizeof(torrent_status_t));
    if(sessionhandle==0)
    {
        wcscpy(t->error,STR(STR_DWN_ERRSES));
        return;
    }
    sessionhandle->get_torrent_status(&temp,&yes1,0);

    if(temp.empty())
    {
        wcscpy(t->error,STR(STR_DWN_ERRTOR));
        return;
    }
    torrent_status& st=temp[0];

    t->downloaded=st.total_wanted_done;
    t->downloadsize=st.total_wanted;
    t->uploaded=st.total_payload_upload;

    t->elapsed=13;
    t->status=(WCHAR *)STR(STR_TR_ST0+(int)st.state);
    if(sessionhandle->is_paused())t->status=(WCHAR *)STR(STR_TR_ST4);
    finisheddownloading=st.is_finished;

    wcscpy(t->error,L"");

    t->uploadspeed=st.upload_payload_rate;
    t->downloadspeed=st.download_payload_rate;

    t->seedstotal=st.list_seeds;
    t->peerstotal=st.list_peers;
    t->seedsconnected=st.num_seeds;
    t->peersconnected=st.num_peers;

    t->wasted=st.total_redundant_bytes;
    t->wastedhashfailes=st.total_failed_bytes;

    if(torrenttime)t->elapsed=GetTickCount()-torrenttime;
    if(t->downloadspeed)
    {
        averageSpeed=SMOOTHING_FACTOR*t->downloadspeed+(1-SMOOTHING_FACTOR)*averageSpeed;
        t->remaining=(t->downloadsize-t->downloaded)/averageSpeed*1000;
    }

    t->sessionpaused=sessionhandle->is_paused();
    t->torrentpaused=st.paused;

    if(torrentstatus.downloadsize)
        manager_g->items_list[SLOT_DOWNLOAD].percent=torrentstatus.downloaded*1000/torrentstatus.downloadsize;
    redrawfield();
}

void update_stop()
{
    if(!sessionhandle)return;

    log_con("Closing torrent sesstion...");
    delete sessionhandle;
    log_con("DONE\n");
}

void update_resume()
{
    if(!sessionhandle||!updatehandle.torrent_file())
    {
        finisheddownloading=1;
        finishedupdating=1;
        return;
    }
    if(sessionhandle->is_paused())
    {
        updatehandle.force_recheck();
        log_con("torrent_resume\n");
        SetEvent(downloadmangar_event);
    }
    sessionhandle->resume();
    finisheddownloading=0;
    finishedupdating=0;
    torrenttime=GetTickCount();
}

void upddlg_calctotalsize(HWND hList)
{
    WCHAR buf[BUFLEN];
    int i;

    totalsize=0;
    for(i=0;i<ListView_GetItemCount(hList);i++)
    if(ListView_GetCheckState(hList,i))
    {
        ListView_GetItemText(hList,i,1,buf,32);
        totalsize+=_wtoi(buf);
    }
}

void upddlg_setpriorities(HWND hList)
{
    int i;
    LVITEM item;
    int base_pri=0,indexes_pri=0;

    for(i=0;i<numfiles;i++)
    {
        item.iItem=i;
        ListView_GetItem(hList,&item);
        if(item.lParam==-2)base_pri=ListView_GetCheckState(hList,i)?2:0;
        if(item.lParam==-1)indexes_pri=ListView_GetCheckState(hList,i)?2:0;
    }

    for(i=0;i<numfiles;i++)
    if(StrStrIA(updatehandle.torrent_file()->file_at(i).path.c_str(),"drivers\\"))
    {
        updatehandle.file_priority(i,0);
    }
    else
    {
        updatehandle.file_priority(i,StrStrIA(updatehandle.torrent_file()->file_at(i).path.c_str(),"indexes\\")?
                                   indexes_pri:base_pri);
        //updatehandle.file_priority(i,ListView_GetCheckState(hList,0)?2:0);
        //if(ListView_GetCheckState(hList,0))
        //    log_con("Needs %s\n",updatehandle.torrent_file()->file_at(i).path.c_str());
    }

    item.mask=LVIF_PARAM;
    for(i=0;i<ListView_GetItemCount(hList);i++)
    {
        item.iItem=i;
        ListView_GetItem(hList,&item);
        if(item.lParam>=0&&ListView_GetCheckState(hList,i))
        {
            updatehandle.file_priority(item.lParam,1);
            //log_con("Needs %s\n",updatehandle.torrent_file()->file_at(i).path.c_str());
        }
    }
}

void upddlg_setpriorities_driverpack(const WCHAR *name,int pri)
{
    int i;
    char buf[BUFLEN];

    sprintf(buf,"%ws",name);
    //log_con("<%s> %d\n",buf,pri);
    for(i=0;i<numfiles;i++)
    if(StrStrIA(updatehandle.torrent_file()->file_at(i).path.c_str(),buf))
    {
        updatehandle.file_priority(i,pri);
        log_con("%ws,%d\n",name,pri);
    }
}

void upddlg_setcheckboxes(HWND hList)
{
    char filelist[BUFLEN];
    int i;
    LVITEM item;

    if(torrentstatus.sessionpaused)return;
    item.mask=LVIF_PARAM;
    memset(filelist,0,BUFLEN);
    for(i=0;i<ListView_GetItemCount(hList);i++)
    {
        item.iItem=i;
        ListView_GetItem(hList,&item);
        if(item.lParam>=0)
        {
            filelist[item.lParam]=1;
            ListView_SetCheckState(hList,i,updatehandle.file_priority(item.lParam));
        }
    }

    ListView_SetCheckState(hList,0,0);
    for(i=0;i<numfiles;i++)
    if(updatehandle.file_priority(i)==2)
    {
        ListView_SetCheckState(hList,0,1);
        break;
    }
}

LRESULT CALLBACK NewButtonProc(HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
    short x,y;

    x=LOWORD(lParam);
    y=HIWORD(lParam);

    switch(uMsg)
    {
        case WM_MOUSEMOVE:
            drawpopup(STR_UPD_BTN_THISPC_H,FLOATING_TOOLTIP,x,y,hWnd);
            if(!bMouseInWindow)
            {
                bMouseInWindow=1;
                TRACKMOUSEEVENT tme;
                tme.cbSize=sizeof(tme);
                tme.dwFlags=TME_LEAVE;
                tme.hwndTrack=hWnd;
                TrackMouseEvent(&tme);
            }
            break;

        case WM_MOUSELEAVE:
            bMouseInWindow=0;
            drawpopup(-1,FLOATING_NONE,0,0,hWnd);
            break;

        default:
            return CallWindowProc(wpOrigButtonProc,hWnd,uMsg,wParam,lParam);
    }
    return true;
}

BOOL CALLBACK UpdateProcedure(HWND hwnd,UINT Message,WPARAM wParam,LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    LVCOLUMN lvc;
    HWND hList,thispcbut;
    WCHAR buf[32];
    int i;

    hList=GetDlgItem(hwnd,IDLIST);
    thispcbut=GetDlgItem(hwnd,IDCHECKTHISPC);

    switch(Message)
    {
        case WM_INITDIALOG:
            ListView_SetExtendedListViewStyle(hList,LVS_EX_CHECKBOXES|LVS_EX_FULLROWSELECT);

            lvc.mask=LVCF_FMT|LVCF_WIDTH|LVCF_SUBITEM|LVCF_TEXT;
            lvc.pszText=(WCHAR *)L"";
            for(i=0;i<6;i++)
            {
                lvc.cx=cxn[i];
                lvc.iSubItem=i;
                lvc.fmt=i?LVCFMT_RIGHT:LVCFMT_LEFT;
                ListView_InsertColumn(hList,i,&lvc);
            }

            hUpdate=hwnd;
            upddlg_populatelist(hList);
            upddlg_updatelang();
            upddlg_setcheckboxes(hList);

            wpOrigButtonProc=(WNDPROC)SetWindowLong(thispcbut,GWLP_WNDPROC,(LONG)NewButtonProc);

            return TRUE;

        case WM_NOTIFY:
            if(((LPNMHDR)lParam)->code==LVN_ITEMCHANGED)
            {
                upddlg_calctotalsize(hList);
                upddlg_updatelang();
                return TRUE;
            }
            break;

        case WM_DESTROY:
            SetWindowLong(thispcbut,GWLP_WNDPROC,(LONG)wpOrigButtonProc);
            break;

        case WM_COMMAND:
            switch(LOWORD(wParam))
            {
                case IDOK:
                    hUpdate=0;
                    upddlg_setpriorities(hList);
                    update_resume();
                    EndDialog(hwnd,IDOK);
                    return TRUE;

                case IDACCEPT:
                    upddlg_setpriorities(hList);
                    update_resume();
                    return TRUE;

                case IDCANCEL:
                    hUpdate=0;
                    EndDialog(hwnd,IDCANCEL);
                    return TRUE;

                case IDCHECKALL:
                case IDUNCHECKALL:
                    for(i=0;i<ListView_GetItemCount(hList);i++)
                        ListView_SetCheckState(hList,i,LOWORD(wParam)==IDCHECKALL?1:0);
                    return TRUE;

                case IDCHECKTHISPC:
                    for(i=0;i<ListView_GetItemCount(hList);i++)
                    {
                        ListView_GetItemText(hList,i,5,buf,32);
                        ListView_SetCheckState(hList,i,StrStrIW(buf,STR(STR_UPD_YES))?1:0);
                    }
                    return TRUE;

                default:
                    break;
            }
            break;

        default:
            break;
    }
    return FALSE;
}

#endif
