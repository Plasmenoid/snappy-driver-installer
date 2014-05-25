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


#include "libtorrent/config.hpp"
#include "libtorrent/entry.hpp"
#include "libtorrent/bencode.hpp"
#include "libtorrent/session.hpp"

extern "C"
{
#define _WIN32_IE 0x0300
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


typedef struct _driverpack_update_t
{
    WCHAR name[4096];
    int index;
    int needed;
    int oldver,newver;
    int percent;
}driverpack_update_t;

int drp_num;
driverpack_update_t drp_list[128];

char bff[4096];
WCHAR bffw[4096];
int getver(const char *ptr)
{
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
    WCHAR *s=bffw;

    wsprintf(bffw,L"%S",ptr);
    while(*s)
    {
        if(*s=='_'&&s[1]>='0'&&s[1]<='9')
        {
            *s=0;
            s=finddrp(bffw);
            if(!s)return 0;
            //log_con("%s,%ws\n",ptr,s);
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
    int i;

    SetWindowText(hwnd,STR(STR_UPD_TITLE));
    SetWindowText(GetDlgItem(hwnd,IDCHECKALL),STR(STR_UPD_BTN_ALL));
    SetWindowText(GetDlgItem(hwnd,IDUNCHECKALL),STR(STR_UPD_BTN_NONE));
    SetWindowText(GetDlgItem(hwnd,IDCHECKTHISPC),STR(STR_UPD_BTN_THISPC));
    SetWindowText(GetDlgItem(hwnd,IDOK),STR(STR_UPD_BTN_OK));
    SetWindowText(GetDlgItem(hwnd,IDCANCEL),STR(STR_UPD_BTN_CANCEL));
    SetWindowText(GetDlgItem(hwnd,IDACCEPT),STR(STR_UPD_BTN_ACCEPT));
    SetWindowText(GetDlgItem(hwnd,IDTOTALSIZE),STR(STR_UPD_TOTALSIZE));

    lvc.mask=LVCF_TEXT;
    for(i=0;i<6;i++)
    {
        lvc.pszText=(WCHAR *)STR(STR_UPD_COL_NAME+i);
        ListView_SetColumn(GetDlgItem(hwnd,IDLIST),i,&lvc);
    }
}

}
using namespace libtorrent;
void populatelist(HWND hList)
{
    session s;
    add_torrent_params p;
    error_code ec;
    torrent_handle th;
    file_entry fe;
    LVITEM lvI;
    int i;
    int basesize=0;
    const char *filename;
    WCHAR buf[4096];
    int newver=0;

    p.save_path="./update";
    p.ti=new torrent_info("samdrivers.torrent",ec);
    log_con("Listen %d\n",s.is_listening());
    th=s.add_torrent(p,ec);
    th.set_priority(0);

    for(i=0;i<p.ti->num_files();i++)
    {
        fe=p.ti->file_at(i);
        filename=fe.path.c_str();
        if(!StrStrIA(filename,"drivers\\"))
        {
            basesize+=fe.size;
            th.file_priority(1,2);
            //log_con("%.0f %s\n",fe.size/1024./1024.,filename);
            if(StrStrIA(filename,"sdi_R"))
                newver=atol(StrStrIA(filename,"sdi_R")+5);
        }
    }
    EnterCriticalSection(&sync);

    lvI.mask      = LVIF_TEXT|LVIF_STATE;
    lvI.stateMask = 0;
    lvI.iSubItem  = 0;
    lvI.state     = 0;
    lvI.iItem     = 0;
    //if(newver>SVN_REV)
    {
        wsprintf(lvI.pszText,L"Base");
        ListView_InsertItem(hList, &lvI);
        wsprintf(buf,L"%d MB",basesize/1024/1024);
        ListView_SetItemText(hList,0,1,buf);
        wsprintf(buf,L"%d%%",0);
        ListView_SetItemText(hList,0,2,buf);
        wsprintf(buf,L" SDI_R%d",newver);
        ListView_SetItemText(hList,0,3,buf);
        wsprintf(buf,L" SDI_R%d",SVN_REV);
        ListView_SetItemText(hList,0,4,buf);
    }

    drp_num=0;
    for(i=0;i<p.ti->num_files();i++)
    {
        fe=p.ti->file_at(i);
        filename=fe.path.c_str()+20;
        if(StrStrIA(filename,".7z"))
        {
            int oldver;
            lvI.iItem=drp_num+1;

            lvI.pszText=drp_list[drp_num].name;
            wsprintf(lvI.pszText,L"%S",filename);
            int sz=fe.size/1024/1024;
            if(!sz)sz=1;
            newver=2;
            oldver=1;

            newver=getver(filename);
            oldver=getcurver(filename);

            if(newver>oldver)
            {
                ListView_InsertItem(hList,&lvI);
                wsprintf(buf,L"%d MB",sz);
                ListView_SetItemText(hList,drp_num+1,1,buf);
                wsprintf(buf,L"%d%%",0);
                ListView_SetItemText(hList,drp_num+1,2,buf);
                wsprintf(buf,L"%d",newver);
                ListView_SetItemText(hList,drp_num+1,3,buf);
                wsprintf(buf,L"%d",oldver);
                ListView_SetItemText(hList,drp_num+1,4,buf);
                wsprintf(buf,L"%S",filename);
                wsprintf(buf,L"%ws",STR(STR_UPD_YES+manager_drplive(buf)));
                ListView_SetItemText(hList,drp_num+1,5,buf);
                drp_num++;
            }
        }
    }
    LeaveCriticalSection(&sync);
    log_con("Start\n");
    s.listen_on(std::make_pair(59442,59443), ec);
    if (ec)
    {
        log_con("failed to open listen socket: %s\n", ec.message().c_str());
    }

    //Sleep(20000);
    log_con("Finish %d\n",(int)s.status().total_download);
}


extern "C"
{

int cxn[]=
{
    199,
    60,
    40,
    70,
    70,
    90,
};

BOOL CALLBACK UpdateProcedure(HWND hwnd,UINT Message,WPARAM wParam,LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    HWND hList;
    int i;

    switch(Message)
    {
        case WM_INITDIALOG:
            {
                hList=GetDlgItem(hwnd,IDLIST);
                ListView_SetExtendedListViewStyle(hList,LVS_EX_CHECKBOXES);
                LVCOLUMN lvc;

                lvc.mask=LVCF_FMT|LVCF_WIDTH|LVCF_SUBITEM|LVCF_TEXT;
                for(i=0;i<6;i++)
                {
                    lvc.pszText=(WCHAR *)L"";
                    lvc.cx=cxn[i];
                    lvc.iSubItem = i;
                    lvc.fmt = i?LVCFMT_RIGHT:LVCFMT_LEFT;
                    ListView_InsertColumn(hList, i, &lvc);
                }

                //ListView_SetCheckState(hList,2,1);
                populatelist(hList);
                updatelang(hwnd);
            }
            return TRUE;

        case WM_COMMAND:
            switch(LOWORD(wParam))
            {
                case IDOK:
                    EndDialog(hwnd,IDOK);
                    return TRUE;

                case IDCANCEL:
                    EndDialog(hwnd,IDCANCEL);
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
