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

extern "C"
{
#define _WIN32_IE 0x0400
#define BUFLEN              4096

#include <commctrl.h>
#include <Shlwapi.h>
#include "resources.h"
#include "common.h"
#include "indexing.h"
#include "theme.h"
#include "update.h"
#include "svnrev.h"
void log_con(CHAR const *format,...);
int  manager_drplive(WCHAR *s);
extern CRITICAL_SECTION sync;

int totalsize=0,numfiles;

int getver(const char *ptr)
{
    char bff[4096];
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
    WCHAR bffw[4096];
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

void updatelang(HWND hwnd)
{
    LVCOLUMN lvc;
    WCHAR buf[BUFLEN];
    int i;

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

}
using namespace libtorrent;
session *sessionhandle=0;
torrent_handle updatehandle;
add_torrent_params params;

void populatelist(HWND hList)
{
    error_code ec;
    file_entry fe;
    LVITEM lvI;
    int i;
    int basesize=0;
    const char *filename;
    WCHAR buf[128];
    int newver=0;

    params.save_path="./update";
    params.ti=new torrent_info("samdrivers.torrent",ec);

    numfiles=params.ti->num_files();
    for(i=0;i<numfiles;i++)
    {
        fe=params.ti->file_at(i);
        filename=fe.path.c_str();
        if(!StrStrIA(filename,"drivers\\"))
        {
            basesize+=fe.size;
            //updatehandle.file_priority(1,2);
            //log_con("%.0f %s\n",fe.size/1024./1024.,filename);
            if(StrStrIA(filename,"sdi_R"))
                newver=atol(StrStrIA(filename,"sdi_R")+5);
        }
    }
    EnterCriticalSection(&sync);

    lvI.mask      =LVIF_TEXT|LVIF_STATE|LVIF_PARAM;
    lvI.stateMask =0;
    lvI.iSubItem  =0;
    lvI.state     =0;
    lvI.iItem     =0;
    lvI.lParam    =-1;
    //if(newver>SVN_REV)
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
        fe=params.ti->file_at(i);
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

            if(newver>oldver)
            {
                lvI.lParam=i;
                int j=ListView_InsertItem(hList,&lvI);
                wsprintf(buf,L"%d %s",sz,STR(STR_UPD_MB));
                ListView_SetItemText(hList,j,1,buf);
                wsprintf(buf,L"%d%%",0);
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
    LeaveCriticalSection(&sync);
}

void update_start()
{
    error_code ec;

    if(!sessionhandle)
    {
        sessionhandle=new session();
        log_con("Listen %d\n",sessionhandle->is_listening());
        updatehandle=sessionhandle->add_torrent(params,ec);

        //sessionhandle.listen_on(std::make_pair(59442,59443), ec);
        if (ec)
        {
            log_con("failed to open listen socket: %s\n", ec.message().c_str());
        }
        log_con("Start update\n");
    }
    log_con("Downloaded %d\n",(int)sessionhandle->status().total_download);
}

void update_stop()
{
    if(sessionhandle)
    {
        log_con("Finish %d\n",(int)sessionhandle->status().total_download);
        if(sessionhandle)delete sessionhandle;
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
    updatelang(GetParent(hList));
}

void updatepriorities(HWND hList)
{
    char filelist[BUFLEN];
    int i;
    LVITEM item;

    log_con("Accept,%d\n",numfiles);
    item.mask=LVIF_PARAM;
    memset(filelist,0,BUFLEN);
    for(i=0;i<ListView_GetItemCount(hList);i++)
    {
        item.iItem=i;
        ListView_GetItem(hList,&item);
        if(item.lParam>=0)
        {
            filelist[item.lParam]=1;
            log_con("%5d,%5d,%5d\n",i,item.lParam,ListView_GetCheckState(hList,i));
        }
    }
    for(i=0;i<numfiles;i++)
    if(!filelist[i])
    {

        log_con("A %5d\n",i);
    }

}

extern "C"
{

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

                populatelist(hList);
                updatelang(hwnd);
                update_start();
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
                    EndDialog(hwnd,IDOK);
                    return TRUE;

                case IDACCEPT:
                    updatepriorities(hList);
                    return TRUE;

                case IDCANCEL:
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

}
#endif

