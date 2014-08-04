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
#include "libtorrent/session.hpp"

#define TORRENT_URL "samdrivers.torrent"
using namespace libtorrent;

extern "C"
{
#define _WIN32_IE 0x0400
#include "main.h"
}

session *sessionhandle=0;
torrent_handle updatehandle;

volatile int downloadmangar_exitflag=0;
HANDLE downloadmangar_event;
HANDLE thandle_download;
long long torrenttime=0;

//add_torrent_params p11;
session_settings settings;
dht_settings dht;

int totalsize=0,numfiles;
HWND hUpdate=0;
volatile int downloadtread_a=0;

int yes1(libtorrent::torrent_status const&);

int getver(const char *ptr)
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

void updatelang()
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

unsigned int __stdcall thread_download(void *arg)
{
    int r=0;
    log_con("{thread_download\n");

    WaitForSingleObject(downloadmangar_event,INFINITE);
    r=populatelist(0);
    log_con("Latest version: R%d\nUpdated driverpacks available: %d\n",r>>8,r&0xFF);
    if(r)
    {
        manager_g->items_list[SLOT_DOWNLOAD].isactive=1;
        manager_setpos(manager_g);
        manager_g->items_list[SLOT_DOWNLOAD].val1=r;
    }

    while(!downloadmangar_exitflag)
    {
        WaitForSingleObject(downloadmangar_event,INFINITE);
        if(downloadmangar_exitflag)break;

        while(sessionhandle&&downloadtread_a)
        {
            InvalidateRect(hPopup,0,0);
            Sleep(500);
            if(downloadmangar_exitflag)break;
        }
    }
    log_con("}thread_download\n");
    return 0;
}

int populatelist(HWND hList)
{
    error_code ec;
    file_entry fe;
    LVITEM lvI;
    int i;
    int basesize=0;
    const char *filename;
    WCHAR buf[128];
    int newver=0;
    int ret=0;

    time_chkupdate=GetTickCount();

    update_start();
    //Sleep(2000);

    boost::intrusive_ptr<torrent_info const> ti;
    std::vector<size_type> file_progress;
    ti=updatehandle.torrent_file();
    updatehandle.file_progress(file_progress);

    //numfiles=updatehandle.torrent_file()->num_files();
    //log_con("Num:%d\n",ti->num_files());
    numfiles=ti->num_files();
    for(i=0;i<numfiles;i++)
    {
        fe=ti->file_at(i);
        filename=fe.path.c_str();
        if(!StrStrIA(filename,"drivers\\"))
        {
            basesize+=fe.size;
            if(StrStrIA(filename,"sdi_R"))
                newver=atol(StrStrIA(filename,"sdi_R")+5);
        }
    }

    lvI.mask      =LVIF_TEXT|LVIF_STATE|LVIF_PARAM;
    lvI.stateMask =0;
    lvI.iSubItem  =0;
    lvI.state     =0;
    lvI.iItem     =0;
    lvI.lParam    =-1;
    if(/*newver>SVN_REV&&*/1)ret+=newver<<8;
    if(/*newver>SVN_REV&&*/hList)
    {
        lvI.pszText=(WCHAR *)STR(STR_UPD_BASEFILES);
        ListView_InsertItem(hList,&lvI);
        wsprintf(buf,L"%d %s",basesize/1024/1024,STR(STR_UPD_MB));
        ListView_SetItemText(hList,0,1,buf);
        wsprintf(buf,L"%d%%",0);
        ListView_SetItemText(hList,0,2,buf);
        wsprintf(buf,L" SDI_R%d",newver);
        ListView_SetItemText(hList,0,3,buf);
        wsprintf(buf,L" SDI_R%d",SVN_REV);
        ListView_SetItemText(hList,0,4,buf);
        ListView_SetItemText(hList,0,5,(WCHAR *)STR(STR_UPD_YES));
    }

    for(i=0;i<numfiles;i++)
    {
        fe=ti->file_at(i);
        filename=fe.path.c_str()+20;
        if(StrStrIA(filename,".7z"))
        {
            int oldver;

            wsprintf(buf,L"%S",filename);
            lvI.pszText=buf;
            int sz=fe.size/1024/1024;
            if(!sz)sz=1;

            newver=getver(filename);
            oldver=getcurver(filename);

            if(newver>oldver)ret++;
            if(newver>oldver&&hList)
            {
                lvI.lParam=i;
                int j=ListView_InsertItem(hList,&lvI);
                wsprintf(buf,L"%d %s",sz,STR(STR_UPD_MB));
                ListView_SetItemText(hList,j,1,buf);
                wsprintf(buf,L"%d%%",file_progress[i]*1000/ti->file_at(i).size);
                ListView_SetItemText(hList,j,2,buf);
                wsprintf(buf,L"%d",newver);
                ListView_SetItemText(hList,j,3,buf);
                wsprintf(buf,L"%d",oldver);
                ListView_SetItemText(hList,j,4,buf);
                wsprintf(buf,L"%S",filename);
                wsprintf(buf,L"%ws",STR(STR_UPD_YES+manager_drplive(buf)));
                ListView_SetItemText(hList,j,5,buf);
            }
        }
    }
    time_chkupdate=GetTickCount()-time_chkupdate;
    return ret;
}

int yes1(libtorrent::torrent_status const&){return true;}

void update_start()
{
    error_code ec;
    add_torrent_params params;

    if(!sessionhandle)
    {
        sessionhandle=new session(/*fingerprint("LT",LIBTORRENT_VERSION_MAJOR,LIBTORRENT_VERSION_MINOR,0,0)*/);
        //sessionhandle=new session(fingerprint("UT",3,4,0,0));

        sessionhandle->start_lsd();
        sessionhandle->start_upnp();
        sessionhandle->start_natpmp();
        int port=50171;

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

        settings.user_agent = std::string("SDI 1.3.5");
        settings.choking_algorithm = session_settings::auto_expand_choker;
        settings.disk_cache_algorithm = session_settings::avoid_readback;
        settings.always_send_user_agent=true;
        settings.volatile_read_cache = false;
        sessionhandle->set_settings(settings);

        params.save_path="update";
        params.ti=new torrent_info(TORRENT_URL,ec);
        params.flags=0;
        //log_con("(%s)\n","http://dl.dropboxusercontent.com/s/j8ut11k7x5vmtfk/SamDrivers.torrent");
        updatehandle=sessionhandle->add_torrent(params,ec);

        /*p11.save_path = "update";
        p11.ti = new torrent_info("kick.torrent", ec);
        p11.flags=0;
        if (ec)
        {
            log_con("%s\n", ec.message().c_str());
            //return 1;
        }*/
        //updatehandle=sessionhandle->add_torrent(p11, ec);


        log_con("Start update\n");
        torrenttime=GetTickCount();
        downloadtread_a=1;
        SetEvent(downloadmangar_event);
        //sessionhandle->pause();
        updatehandle.resume();
        Sleep(5000);
    }
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
        t->remaining=(t->downloadsize-t->downloaded)/t->downloadspeed*1000;
    }
}

void update_stop()
{
    if(sessionhandle)
    {
        downloadtread_a=0;
        log_con("Finish %d\n",(int)sessionhandle->status().total_download);
        if(sessionhandle)delete sessionhandle;
        sessionhandle=0;
        Sleep(2000);
    }
}

void updatestatus(HWND hList)
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
    updatelang();
}

void updatepriorities(HWND hList)
{
    char filelist[BUFLEN];
    int i;
    LVITEM item;

    item.mask=LVIF_PARAM;
    memset(filelist,0,BUFLEN);
    for(i=0;i<ListView_GetItemCount(hList);i++)
    {
        item.iItem=i;
        ListView_GetItem(hList,&item);
        if(item.lParam>=0)
        {
            filelist[item.lParam]=1;
            updatehandle.file_priority(item.lParam,ListView_GetCheckState(hList,i)?1:0);
        }
    }
    for(i=0;i<numfiles;i++)
    if(!filelist[i])
    {
        updatehandle.file_priority(i,ListView_GetCheckState(hList,0)?2:0);
    }

}

void updatecheckboxes(HWND hList)
{
    char filelist[BUFLEN];
    int i;
    LVITEM item;

    item.mask=LVIF_PARAM;
    memset(filelist,0,BUFLEN);
    for(i=0;i<ListView_GetItemCount(hList);i++)
    {
        item.iItem=i;
        ListView_GetItem(hList,&item);
        if(item.lParam>=0)
        {
            filelist[item.lParam]=1;
            //updatehandle.file_priority(item.lParam,ListView_GetCheckState(hList,i)?1:0);
            ListView_SetCheckState(hList,i,updatehandle.file_priority(item.lParam));
        }
    }
    /*for(i=0;i<numfiles;i++)
    if(!filelist[i])
    {
        updatehandle.file_priority(i,ListView_GetCheckState(hList,0)?2:0);
    }*/

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

BOOL CALLBACK UpdateProcedure(HWND hwnd,UINT Message,WPARAM wParam,LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    LVCOLUMN lvc;
    //LPNMITEMACTIVATE lpnmitem;
    HWND hList;
    WCHAR buf[32];
    int i;

    hList=GetDlgItem(hwnd,IDLIST);
    switch(Message)
    {
        case WM_INITDIALOG:
            {
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
                populatelist(hList);
                updatelang();
                update_start();
                updatecheckboxes(hList);
            }
            return TRUE;

        case WM_NOTIFY:
            if(((LPNMHDR)lParam)->code==LVN_ITEMCHANGED)
            {
                updatestatus(hList);
                /*lpnmitem = (LPNMITEMACTIVATE) lParam;
                i=ListView_GetCheckState(hList,lpnmitem->iItem);
                //log_con("%d,%d,%d\n",lpnmitem->iSubItem,lpnmitem->uOldState,lpnmitem->uNewState);
                i^=1;
                ListView_SetCheckState(hList,lpnmitem->iItem,i);*/
                return TRUE;
            }
            break;

        case WM_COMMAND:
            switch(LOWORD(wParam))
            {
                case IDOK:
                    hUpdate=0;
                    updatepriorities(hList);
                    EndDialog(hwnd,IDOK);
                    return TRUE;

                case IDACCEPT:
                    updatepriorities(hList);
                    return TRUE;

                case IDCANCEL:
                    hUpdate=0;
                    EndDialog(hwnd,IDCANCEL);
                    return TRUE;

                case IDCHECKALL:
                case IDUNCHECKALL:
                    for(i=0;i<ListView_GetItemCount(hList);i++)
                        ListView_SetCheckState(hList,i,LOWORD(wParam)==IDCHECKALL?1:0);
                    updatestatus(hList);
                    return TRUE;

                case IDCHECKTHISPC:
                    for(i=0;i<ListView_GetItemCount(hList);i++)
                    {
                        ListView_GetItemText(hList,i,5,buf,32);
                        ListView_SetCheckState(hList,i,StrStrIW(buf,STR(STR_UPD_YES))?1:0);
                    }
                    updatestatus(hList);
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

