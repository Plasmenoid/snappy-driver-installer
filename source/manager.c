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

const status_t statustnl[NUM_STATUS]=
{
    {FILTER_SHOW_CURRENT,   STATUS_CURRENT},
    {FILTER_SHOW_NEWER,     STATUS_NEW},
    {FILTER_SHOW_OLD,       STATUS_OLD},
    {FILTER_SHOW_WORSE_RANK,STATUS_WORSE},
    {FILTER_SHOW_BETTER,    STATUS_BETTER},
    {FILTER_SHOW_MISSING,   STATUS_MISSING},
};
//}

//{ Manager
void manager_init(manager_t *manager,matcher_t *matcher)
{
    itembar_t *itembar;
    int i;

    manager->matcher=matcher;
    heap_init(&manager->items_handle,ID_MANAGER,(void **)&manager->items_list,0,sizeof(itembar_t));

    for(i=0;i<RES_SLOTS;i++)
    {
        itembar=(itembar_t *)heap_allocitem_ptr(&manager->items_handle);
        itembar_init(itembar,0,0,i);
    }
}

void manager_free(manager_t *manager)
{
    heap_free(&manager->items_handle);
}

void manager_populate(manager_t *manager)
{
    matcher_t *matcher=manager->matcher;
    devicematch_t *devicematch;
    hwidmatch_t *hwidmatch;
    int i,j,id=RES_SLOTS;

    manager->items_handle.used=sizeof(itembar_t)*RES_SLOTS;
    manager->items_handle.items=RES_SLOTS;

    devicematch=matcher->devicematch_list;
    for(i=0;i<matcher->devicematch_handle.items;i++,devicematch++)
    {
        hwidmatch=&matcher->hwidmatch_list[devicematch->start_matches];
        for(j=0;j<devicematch->num_matches;j++,hwidmatch++)
        {
            itembar_init(heap_allocitem_ptr(&manager->items_handle),devicematch,hwidmatch,i+RES_SLOTS);
            id++;
        }
        if(!devicematch->num_matches)
        {
            itembar_init(heap_allocitem_ptr(&manager->items_handle),devicematch,0,i+RES_SLOTS);
            id++;
        }
    }
}

void manager_filter(manager_t *manager,int options)
{
    devicematch_t *devicematch;
    itembar_t *itembar;
    int i,j,k;
    int cnt[NUM_STATUS+1];
    int o1=options&FILTER_SHOW_ONE;

    manager->items_list[SLOT_RESTORE_POINT].isactive=statemode==STATEMODE_LOAD?0:1;
    manager->items_list[SLOT_RESTORE_POINT].install_status=STR_RESTOREPOINT;
    for(i=RES_SLOTS;i<manager->items_handle.items;i++)
    {
        devicematch=manager->items_list[i].devicematch;
        memset(cnt,0,sizeof(cnt));
        if(!devicematch)continue;
        itembar=&manager->items_list[devicematch->start_matches+RES_SLOTS];
        for(j=0;j<devicematch->num_matches;j++,itembar++)
        {
            itembar->isactive=0;

            if((options&FILTER_SHOW_INVALID)==0&&!isdrivervalid(itembar->hwidmatch))
                continue;

            if((options&FILTER_SHOW_DUP)==0&&itembar->hwidmatch->status&STATUS_DUP)
                continue;

            if((options&FILTER_SHOW_DUP)&&itembar->hwidmatch->status&STATUS_DUP)
            {
                itembar_t *itembar1=&manager->items_list[devicematch->start_matches+RES_SLOTS];
                for(k=0;k<j;k++,itembar1++)
                    if(itembar1->isactive&&
                       itembar1->index==itembar->index&&
                       getdrp_infcrc(itembar1->hwidmatch)==getdrp_infcrc(itembar->hwidmatch))
                        break;

                if(k!=j)
                    itembar->isactive=1;
            }

            for(k=0;k<NUM_STATUS;k++)
                if((!o1||!cnt[NUM_STATUS])&&(options&statustnl[k].filter)&&itembar->hwidmatch->status&statustnl[k].status)
            {
                if((options&FILTER_SHOW_WORSE_RANK)==0&&(options&FILTER_SHOW_OLD)==0&&devicematch->device->problem==0
                   &&itembar->hwidmatch->altsectscore<2/*&&manager->matcher->state->platform.dwMajorVersion>=6*/)continue;
                cnt[k]++;
                cnt[NUM_STATUS]++;
                itembar->isactive=1;
            }
        }
        if(!devicematch->num_matches)
        {
            itembar->isactive=0;
            if(options&FILTER_SHOW_NF_STANDARD&&devicematch->status&STATUS_NF_STANDARD)itembar->isactive=1;
            if(options&FILTER_SHOW_NF_UNKNOWN&&devicematch->status&STATUS_NF_UNKNOWN)itembar->isactive=1;
            if(options&FILTER_SHOW_NF_MISSING&&devicematch->status&STATUS_NF_MISSING)itembar->isactive=1;
        }
    }
}

void manager_print(manager_t *manager)
{
    itembar_t *itembar;
    int k,act=0;
    int limits[7];

    if((log_verbose&LOG_VERBOSE_MANAGER)==0)return;
    log("{manager_print\n");
    memset(limits,0,sizeof(limits));

    itembar=manager->items_list;
    for(k=0;k<manager->items_handle.items;k++,itembar++)
        if(itembar->isactive&&itembar->hwidmatch)
            hwidmatch_calclen(itembar->hwidmatch,limits);

    itembar=manager->items_list;
    for(k=0;k<manager->items_handle.items;k++,itembar++)
        if(itembar->isactive&&itembar->hwidmatch)
            {hwidmatch_print(itembar->hwidmatch,limits);act++;}

    log("}manager_print[%d]\n\n",act);
}
//}

//{ User interaction
void manager_hitscan(manager_t *manager,int x,int y,int *r,int *zone)
{
    itembar_t *itembar;
    int i;
    int pos;

    *r=-2;
    *zone=0;
    y-=-D(ITEM_DIST_Y0);
    if(x<D(DRVITEM_OFSX)||x>D(DRVITEM_OFSX)+D(DRVITEM_WX))return;

    itembar=manager->items_list;
    for(i=0;i<manager->items_handle.items;i++,itembar++)
    {
        pos=itembar->curpos>>16;
        if(itembar->isactive&&i<SLOT_RESTORE_POINT)
        pos+=getscrollpos();
        if(y>pos&&y<pos+D(DRVITEM_WY))
        {
            *r=i;
            x-=D(ITEM_CHECKBOX_OFS_X)+D(DRVITEM_OFSX);
            y-=D(ITEM_CHECKBOX_OFS_Y)+pos;
            //printf("%d,%d\n",x,y);
            if(x>0&&x<D(ITEM_CHECKBOX_SIZE)&&y>0&&y<D(ITEM_CHECKBOX_SIZE))*zone=1;
            if(x>D(DRVITEM_WX)-50)*zone=2;
            return;
        }
    }
    *r=-1;
}

void _7z_total(long long i)
{
    total=i;
}

void _7z_setcomplited(long long i)
{
    int j;
    int _totalitems=0;
    int _processeditems=0;

    itembar_t *itembar=manager_g->items_list;
    for(j=0;j<manager_g->items_handle.items;j++,itembar++)
    if(j>=RES_SLOTS)
    {
        if(itembar->checked||itembar->install_status){_totalitems++;}
        if(itembar->install_status&&!itembar->checked){_processeditems++;}
    }

    itembar_settext(manager_g,itembar_act,L"",(int)(i*(instflag&INSTALLDRIVERS?900.:1000.)/total));
    //double d=(i*1000./total)/_totalitems;
    double d=(manager_g->items_list[itembar_act].percent)/_totalitems;
    itembar_settext(manager_g,SLOT_EXTRACTING,L"",(int)(_processeditems*1000./_totalitems+d));
    manager_g->items_list[SLOT_EXTRACTING].val1=_processeditems;
    manager_g->items_list[SLOT_EXTRACTING].val2=_totalitems;
}

unsigned int __stdcall thread_install(void *arg)
{
    UNREFERENCED_PARAMETER(arg)

    itembar_t *itembar,*itembar1;
    WCHAR cmd[BUFLEN];
    WCHAR hwid[BUFLEN];
    WCHAR inf[BUFLEN];
    WCHAR buf[BUFLEN];
    int i,j,size;
    FILE *f;
    void *install64bin;
    RESTOREPOINTINFOW pRestorePtSpec;
    STATEMGRSTATUS pSMgrStatus;
    HINSTANCE hinstLib=0;
    MYPROC ProcAdd;
    manager_t *manager=manager_g;
    int r=0;

    // Prepare extract dir
    mkdir_r(extractdir);
    log_err("Dir: (%ws)\n",extractdir);
    wsprintf(cmd,L"%s\\install64.exe",extractdir);
    f=_wfopen(cmd,L"wb");
    if(f)
        log_err("Created '%ws'\n",cmd);
    else
        log_err("Failed to create '%ws'\n",cmd);
    get_resource(IDR_INSTALL64,&install64bin,&size);
    fwrite(install64bin,1,size,f);
    fclose(f);

    installmode=1;
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

            printf("rt rest point{ %d(%d)\n",r,pSMgrStatus.nStatus);
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
        if(hinstLib)
        {
            //pRestorePtSpec.dwRestorePtType=END_SYSTEM_CHANGE;
            //pRestorePtSpec.llSequenceNumber=pSMgrStatus.llSequenceNumber;
            //ProcAdd(&pRestorePtSpec,&pSMgrStatus);
            //r=printf("rt rest point} %d(%d)\n",r,pSMgrStatus.nStatus);
            FreeLibrary(hinstLib);
        }
        manager->items_list[SLOT_RESTORE_POINT].checked=0;
    }
goaround:
    itembar=manager->items_list;
    for(i=0;i<manager->items_handle.items&&installmode==1;i++,itembar++)
        if(i>=RES_SLOTS&&itembar->checked&&itembar->isactive&&itembar->hwidmatch)
    {
        int unpacked=0;
        itembar_act=i;
        hwidmatch_t *hwidmatch=itembar->hwidmatch;
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
        if(instflag&INSTALLDRIVERS)
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

            if(installmode==1&&itembar->checked)
            {
                if(flags&FLAG_DISABLEINSTALL)
                    Sleep(2000);
                else
                    ret=UpdateDriverForPlugAndPlayDevices(0,hwid,inf,INSTALLFLAG_FORCE|INSTALLFLAG_NONINTERACTIVE,&needrb);
            }
            else
                ret=1;

            if(!ret)ret=GetLastError();
            if((unsigned)ret==0xE0000235)//ERROR_IN_WOW64
            {
                wsprintf(buf,L"\"%s\" \"%s\"",hwid,inf);
                wsprintf(cmd,L"%s\\install64.exe",extractdir);
                log_err("'%ws %ws'\n",cmd,buf);
                //if(installmode==1&&itembar->checked)
                ret=RunSilent(cmd,buf,SW_HIDE,1);
                    //Sleep(1000);
            //    else
                    //ret=1;
                if((ret&0x7FFFFFFF)==1)
                {
                    needrb=ret&0x80000000?1:0;
                    ret&=~0x80000000;
                }
            }
            //ret=1;needrb=1;
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
                //print_error(ret,L"");
            }

            if(needrb)needreboot=1;
            if(installmode==2||!itembar->checked)
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
    if(installmode==1)
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

    installmode=0;
    manager->items_list[SLOT_EXTRACTING].percent=1000;
    redrawfield();

    return 0;
}

void manager_install(extractinfo_t *ei)
{
    wcscpy(extractdir,ei->dir);
    instflag=ei->flags;
    _beginthreadex(0,0,&thread_install,ei,0,0);
}

void manager_clear(manager_t *manager)
{
    itembar_t *itembar;
    int i;

    log_err("Clear\n");
    itembar=manager->items_list;
    for(i=0;i<manager->items_handle.items;i++,itembar++)
    {
        itembar->install_status=0;
        itembar->isactive=0;
        itembar->percent=0;
    }
    PostMessage(hMain,WM_DEVICECHANGE,7,0);

    manager->items_list[SLOT_EXTRACTING].isactive=0;
    manager->items_list[SLOT_RESTORE_POINT].install_status=STR_RESTOREPOINT;
    manager_setpos(manager);
}

void manager_testitembars(manager_t *manager)
{
    itembar_t *itembar;
    int i,j=0,index=1;

    itembar=manager->items_list;

    manager_filter(manager,FILTER_SHOW_CURRENT|FILTER_SHOW_NEWER);
    wcscpy(drpext_dir,L"drpext");
    manager->items_list[SLOT_EMPTY].percent=1;

    for(i=0;i<manager->items_handle.items;i++,itembar++)
    if(i>SLOT_EMPTY&&i<RES_SLOTS)
    {
        itembar->index=index;
        itembar->isactive=1;
    }
    else if(itembar->isactive)
    {
        itembar->checked=0;
        if(j==0||j==6||j==9||j==18||j==21)index++;
        itembar->index=index;
        switch(j++)
        {
            case  0:itembar->install_status=STR_INST_EXTRACT;itembar->percent=300;itembar->checked=1;break;
            case  1:itembar->install_status=STR_INST_INSTALL;itembar->percent=900;itembar->checked=1;break;
            case  2:itembar->install_status=STR_INST_EXTRACT;itembar->percent=400;break;
            case  3:itembar->install_status=STR_INST_OK;break;
            case  4:itembar->install_status=STR_INST_REBOOT;break;
            case  5:itembar->install_status=STR_INST_FAILED;break;

            case  6:itembar->hwidmatch->status=STATUS_INVALID;break;
            case  7:itembar->hwidmatch->status=STATUS_MISSING;break;
            case  8:itembar->hwidmatch->status=STATUS_CURRENT|STATUS_SAME|STATUS_DUP;break;

            case  9:itembar->hwidmatch->status=STATUS_NEW|STATUS_BETTER;break;
            case 10:itembar->hwidmatch->status=STATUS_NEW|STATUS_SAME;break;
            case 11:itembar->hwidmatch->status=STATUS_NEW|STATUS_WORSE;break;
            case 12:itembar->hwidmatch->status=STATUS_CURRENT|STATUS_BETTER;break;
            case 13:itembar->hwidmatch->status=STATUS_CURRENT|STATUS_SAME;break;
            case 14:itembar->hwidmatch->status=STATUS_CURRENT|STATUS_WORSE;break;
            case 15:itembar->hwidmatch->status=STATUS_OLD|STATUS_BETTER;break;
            case 16:itembar->hwidmatch->status=STATUS_OLD|STATUS_SAME;break;
            case 17:itembar->hwidmatch->status=STATUS_OLD|STATUS_WORSE;break;

            case 18:itembar->devicematch->status=STATUS_NF_MISSING;itembar->hwidmatch=0;break;
            case 19:itembar->devicematch->status=STATUS_NF_STANDARD;itembar->hwidmatch=0;break;
            case 20:itembar->devicematch->status=STATUS_NF_UNKNOWN;itembar->hwidmatch=0;break;
            default:itembar->isactive=0;
        }
    }

}

void manager_toggle(manager_t *manager,int index)
{
    itembar_t *itembar,*itembar1;
    int i,group;

    itembar1=&manager->items_list[index];
    itembar1->checked^=1;
    group=itembar1->index;

    itembar=manager->items_list;
    for(i=0;i<manager->items_handle.items;i++,itembar++)
        if(itembar!=itembar1&&itembar->index==group)
            itembar->checked=0;
}

void manager_expand(manager_t *manager,int index)
{
    itembar_t *itembar,*itembar1;
    int i,group;

    itembar1=&manager->items_list[index];
    group=itembar1->index;

    itembar=manager->items_list;
    if((itembar1->isactive&2)==0)
    {
        for(i=0;i<manager->items_handle.items;i++,itembar++)
            if(itembar->index==group)
                itembar->isactive|=2;
    }
    else
    {
        for(i=0;i<manager->items_handle.items;i++,itembar++)
            if(itembar->index==group)
                itembar->isactive&=1;
    }
}

void manager_selectnone(manager_t *manager)
{
    itembar_t *itembar;
    int i;

    itembar=manager->items_list;
    for(i=0;i<manager->items_handle.items;i++,itembar++)itembar->checked=0;
}

void manager_selectall(manager_t *manager)
{
    itembar_t *itembar;
    int i,group=-1;

    itembar=manager->items_list;
    for(i=0;i<manager->items_handle.items;i++,itembar++)
    {
        itembar->checked=0;
        if(itembar->isactive)
        //printf("%d:%d,%d,%d\n",i,itembar->index,group,group!=itembar->index);

        if(itembar->isactive&&group!=itembar->index)
        {
            //printf("*\n");
            itembar->checked=1;
            group=itembar->index;
        }
    }
}
//}

//{ Helpers
void itembar_init(itembar_t *item,devicematch_t *devicematch,hwidmatch_t *hwidmatch,int groupindex)
{
    memset(item,0,sizeof(itembar_t));
    item->devicematch=devicematch;
    item->hwidmatch=hwidmatch;
    item->curpos=(-D(ITEM_DIST_Y0))<<16;
    item->tagpos=(-D(ITEM_DIST_Y0))<<16;
    item->index=groupindex;
}

void itembar_settext(manager_t *manager,int i,WCHAR *txt1,int percent)
{
    itembar_t *itembar=&manager->items_list[i];
    wcscpy(itembar->txt1,txt1);
    itembar->percent=percent;
    itembar->isactive=1;
    redrawfield();
}

void itembar_setpos(itembar_t *itembar,int *pos,int *cnt)
{
    if(itembar->isactive)
    {
        if(*pos<0)
            *pos=DRVITEM_OFSX;
        else
            *pos+=*cnt?D(ITEM_DIST_Y1):D(ITEM_DIST_Y0);

        (*cnt)--;
        //printf("%d,%d\n",itembar->index,*pos);
    }
    //*pos=0;
    itembar->oldpos=itembar->curpos;
    itembar->tagpos=*pos<<16;
    itembar->accel=(itembar->tagpos-itembar->curpos)/(1000/2);
    if(itembar->accel==0)itembar->accel=(itembar->tagpos<itembar->curpos)?500:-500;
}

int isdrivervalid(hwidmatch_t *hwidmatch)
{
    if(hwidmatch->altsectscore>0&&hwidmatch->decorscore>0)return 1;
    return 0;
}

void str_status(WCHAR *buf,itembar_t *itembar)
{
    devicematch_t *devicematch=itembar->devicematch;

    buf[0]=0;

    if(itembar->hwidmatch)
    {
        int status=itembar->hwidmatch->status;
        if(status&STATUS_INVALID)
            wcscat(buf,STR(STR_STATUS_INVALID));
        else
        {
            if(status&STATUS_MISSING)
                wsprintf(buf,L"%s,%d",STR(STR_STATUS_MISSING),itembar->devicematch->device->problem);
            else
            {
                if(status&STATUS_BETTER&&status&STATUS_NEW)        wcscat(buf,STR(STR_STATUS_BETTER_NEW));
                if(status&STATUS_SAME  &&status&STATUS_NEW)        wcscat(buf,STR(STR_STATUS_SAME_NEW));
                if(status&STATUS_WORSE &&status&STATUS_NEW)        wcscat(buf,STR(STR_STATUS_WORSE_NEW));

                if(status&STATUS_BETTER&&status&STATUS_CURRENT)    wcscat(buf,STR(STR_STATUS_BETTER_CUR));
                if(status&STATUS_SAME  &&status&STATUS_CURRENT)    wcscat(buf,STR(STR_STATUS_SAME_CUR));
                if(status&STATUS_WORSE &&status&STATUS_CURRENT)    wcscat(buf,STR(STR_STATUS_WORSE_CUR));

                if(status&STATUS_BETTER&&status&STATUS_OLD)        wcscat(buf,STR(STR_STATUS_BETTER_OLD));
                if(status&STATUS_SAME  &&status&STATUS_OLD)        wcscat(buf,STR(STR_STATUS_SAME_OLD));
                if(status&STATUS_WORSE &&status&STATUS_OLD)        wcscat(buf,STR(STR_STATUS_WORSE_OLD));
            }
        }
        if(status&STATUS_DUP)wcscat(buf,STR(STR_STATUS_DUP));
        if(itembar->hwidmatch->altsectscore<2/*&&manager_g->matcher->state->platform.dwMajorVersion>=6*/)
        {
            wcscat(buf,STR(STR_STATUS_NOTSIGNED));
        }
    }
    else
    //if(devicematch)
    {
        if(devicematch->status&STATUS_NF_STANDARD)wcscat(buf,STR(STR_STATUS_NF_STANDARD));
        if(devicematch->status&STATUS_NF_UNKNOWN) wcscat(buf,STR(STR_STATUS_NF_UNKNOWN));
        if(devicematch->status&STATUS_NF_MISSING) wcscat(buf,STR(STR_STATUS_NF_MISSING));
    }
}

int box_status(int index)
{
    itembar_t *itembar=&manager_g->items_list[index];
    devicematch_t *devicematch=itembar->devicematch;

    switch(index)
    {
        case SLOT_VIRUS_AUTORUN:
        case SLOT_VIRUS_RECYCLER:
        case SLOT_VIRUS_HIDDEN:
            return BOX_DRVITEM_VI;

        case SLOT_INFO:
        case SLOT_DPRDIR:
        case SLOT_SNAPSHOT:
            return BOX_DRVITEM_IF;

        default:
            break;
    }
    if(itembar->hwidmatch)
    {
        int status=itembar->hwidmatch->status;
        if(status&STATUS_INVALID)
            return BOX_DRVITEM_IN;
        else
        {
            switch(itembar->install_status)
            {
                case STR_INST_EXTRACT:
                case STR_INST_INSTALL:
                    return BOX_DRVITEM_D0;

                case STR_INST_OK:
                    return BOX_DRVITEM_D1;

                case STR_INST_REBOOT:
                    return BOX_DRVITEM_D2;

                case STR_INST_FAILED:
                    return BOX_DRVITEM_DE;

                default:break;
            }
            if(status&STATUS_MISSING)
                return BOX_DRVITEM_MS;
            else
            {
                if(status&STATUS_BETTER&&status&STATUS_NEW)        return BOX_DRVITEM_BN;
                if(status&STATUS_SAME  &&status&STATUS_NEW)        return BOX_DRVITEM_SN;
                if(status&STATUS_WORSE &&status&STATUS_NEW)        return BOX_DRVITEM_WN;

                if(status&STATUS_BETTER&&status&STATUS_CURRENT)    return BOX_DRVITEM_BC;
                if(status&STATUS_SAME  &&status&STATUS_CURRENT)    return BOX_DRVITEM_SC;
                if(status&STATUS_WORSE &&status&STATUS_CURRENT)    return BOX_DRVITEM_WC;

                if(status&STATUS_BETTER&&status&STATUS_OLD)        return BOX_DRVITEM_BO;
                if(status&STATUS_SAME  &&status&STATUS_OLD)        return BOX_DRVITEM_SO;
                if(status&STATUS_WORSE &&status&STATUS_OLD)        return BOX_DRVITEM_WO;
            }
        }
    }
    else
    if(devicematch)
    {
        if(devicematch->status&STATUS_NF_STANDARD)  return BOX_DRVITEM_NS;
        if(devicematch->status&STATUS_NF_UNKNOWN)   return BOX_DRVITEM_NU;
        if(devicematch->status&STATUS_NF_MISSING)   return BOX_DRVITEM_NM;
    }
        //if(status&STATUS_DUP)wcscat(buf,STR(STR_STATUS_DUP));
    return BOX_DRVITEM;
}

void str_date(version_t *v,WCHAR *buf)
{
    SYSTEMTIME tm;
    FILETIME ft;

    memset(&tm,0,sizeof(SYSTEMTIME));
    tm.wDay=v->d;
    tm.wMonth=v->m;
    tm.wYear=v->y;
    SystemTimeToFileTime(&tm,&ft);
    FileTimeToSystemTime(&ft,&tm);

    if(v->y<1000)
        wsprintf(buf,STR(STR_HINT_UNKNOWN));
    else
        GetDateFormat(manager_g->matcher->state->locale,0,&tm,0,buf,100);
}

WCHAR *str_version(version_t *ver)
{
    static WCHAR unkver[128];

    wsprintf(unkver,L"%s%s",STR(STR_HINT_VERSION),STR(STR_HINT_UNKNOWN));
    return ver->v1<0?unkver:L"%s%d.%d.%d.%d";
}
//}

//{ Driver list
void manager_setpos(manager_t *manager)
{
    devicematch_t *devicematch;
    itembar_t *itembar;
    int k;
    int cnt=0;
    int pos=D(DRVITEM_OFSY);
    int group=0;
    int lastmatch=0;

    itembar=manager->items_list;
    for(k=0;k<manager->items_handle.items;k++,itembar++)
    {
        devicematch=itembar->devicematch;
        cnt=group==itembar->index?1:0;
        if(devicematch&&!devicematch->num_matches&&!lastmatch)cnt=1;
        itembar_setpos(itembar,&pos,&cnt);
        if(itembar->isactive)
        {
            group=itembar->index;
            if(devicematch)lastmatch=devicematch->num_matches;
        }
    }
    SetTimer(hMain,1,1000/60,0);
    manager->animstart=GetTickCount();
}

int manager_animate(manager_t *manager)
{
    itembar_t *itembar;
    int i;
    int pos=0;
    int chg=0;
    long tt1=GetTickCount()-manager->animstart;

    itembar=manager->items_list;
    for(i=0;i<manager->items_handle.items;i++,itembar++)
    {
        if(itembar->curpos==itembar->tagpos)continue;
        chg=1;
        pos=itembar->oldpos+itembar->accel*tt1;
        if(itembar->accel>0&&pos>itembar->tagpos)pos=itembar->tagpos;
        if(itembar->accel<0&&pos<itembar->tagpos)pos=itembar->tagpos;
        itembar->curpos=pos;
    }
    return chg;
}

int groupsize(manager_t *manager,int index)
{
    itembar_t *itembar;
    int i;
    int num=0;

    itembar=manager->items_list;
    for(i=0;i<manager->items_handle.items;i++,itembar++)
        if(itembar->index==index)
            num++;

    return num;
}

void manager_drawitem(manager_t *manager,HDC hdc,int pos,itembar_t *itembar,int index)
{
    HICON hIcon;
    WCHAR bufw[BUFLEN];
    HRGN hrgn=0;
    int x=D(DRVITEM_OFSX);
    int cur_i,zone;

{
    SCROLLINFO si;
    POINT p;
    si.fMask=SIF_POS;
    si.nPos=0;
    GetScrollInfo(hField,SB_VERT,&si);

    GetCursorPos(&p);
    ScreenToClient(hField,&p);

    manager_hitscan(manager,p.x,p.y+si.nPos,&cur_i,&zone);
        if(index<SLOT_RESTORE_POINT)pos+=si.nPos;
}
    if(pos<=-D(ITEM_DIST_Y0))return;
    if(pos>mainy_c)return;
    if(D(DRVITEM_WX)<0)return;
    //return;
    //printf("%d/%d\n",pos,0);

    SelectObject(hdc,hFont);

    box_draw(hdc,x,pos,D(DRVITEM_OFSX)+D(DRVITEM_WX),pos+D(DRVITEM_WY),
             box_status(index)+((cur_i==index)?1:0));

    hrgn=CreateRectRgn(x,pos,D(DRVITEM_OFSX)+D(DRVITEM_WX),pos+D(DRVITEM_WY));
    SelectClipRgn(hdc,hrgn);

    if(itembar->percent)
    {
        //printf("%d\n",itembar->percent);
        int a=BOX_PROGR;
        if(index==SLOT_EXTRACTING&&installmode==2)a=BOX_PROGR_S;
        if(index>=RES_SLOTS&&(!itembar->checked||installmode==2))a=BOX_PROGR_S;
        box_draw(hdc,x,pos,D(DRVITEM_OFSX)+D(DRVITEM_WX)*itembar->percent/1000.,pos+D(DRVITEM_WY),a);
    }

    SetTextColor(hdc,0);
    switch(index)
    {
        case SLOT_RESTORE_POINT:
            drawcheckbox(hdc,x+D(ITEM_CHECKBOX_OFS_X),pos+D(ITEM_CHECKBOX_OFS_Y),
                         D(ITEM_CHECKBOX_SIZE),D(ITEM_CHECKBOX_SIZE),
                         itembar->checked,cur_i==index);

            wcscpy(bufw,STR(itembar->install_status));
            TextOut(hdc,x+D(ITEM_TEXT_OFS_X),pos+D(ITEM_TEXT_DIST_Y)/2,bufw,wcslen(bufw));
            break;

        case SLOT_INDEXING:
            wsprintf(bufw,L"%s (%d%s%d)",STR(STR_INDEXING),
                        manager->items_list[SLOT_INDEXING].val1,STR(STR_OF),
                        manager->items_list[SLOT_INDEXING].val2);
            TextOut(hdc,x+D(ITEM_TEXT_OFS_X),pos,bufw,wcslen(bufw));

            if(*itembar->txt1)
            {
                wsprintf(bufw,L"%s",itembar->txt1);
                TextOut(hdc,x+D(ITEM_TEXT_OFS_X),pos+D(ITEM_TEXT_DIST_Y),bufw,wcslen(bufw));
            }
            break;

        case SLOT_EXTRACTING:
            pos+=D(ITEM_TEXT_OFS_Y);
            if(installmode)
            {
                wsprintf(bufw,L"%s (%d%s%d)",STR(instflag&INSTALLDRIVERS?STR_INST_INSTALL_ALL:STR_INST_EXTRACT_ALL),
                        manager->items_list[SLOT_EXTRACTING].val1+1,STR(STR_OF),
                        manager->items_list[SLOT_EXTRACTING].val2);
                if(itembar_act==SLOT_RESTORE_POINT)wcscpy(bufw,STR(STR_REST_CREATING));
                if(installmode==2)wcscpy(bufw,STR(STR_INST_STOPPING));
                SetTextColor(hdc,D(boxindex[box_status(index)]+14));
                TextOut(hdc,x+D(ITEM_TEXT_OFS_X),pos,bufw,wcslen(bufw));
                if(itembar_act>=RES_SLOTS)
                {
                    wsprintf(bufw,L"%S",getdrp_drvdesc(manager->items_list[itembar_act].hwidmatch));
                    SetTextColor(hdc,D(boxindex[box_status(index)]+15));
                    TextOut(hdc,x+D(ITEM_TEXT_OFS_X),pos+D(ITEM_TEXT_DIST_Y),bufw,wcslen(bufw));
                }
            }else
            {
                wsprintf(bufw,L"%s",STR(needreboot?STR_INST_COMPLITED_RB:STR_INST_COMPLITED));
                SetTextColor(hdc,D(boxindex[box_status(index)]+14));
                TextOut(hdc,x+D(ITEM_TEXT_OFS_X),pos,bufw,wcslen(bufw));
                wsprintf(bufw,L"%s",STR(STR_INSR_CLOSE));
                SetTextColor(hdc,D(boxindex[box_status(index)]+15));
                TextOut(hdc,x+D(ITEM_TEXT_OFS_X),pos+D(ITEM_TEXT_DIST_Y),bufw,wcslen(bufw));
            }
            break;

        case SLOT_INFO:
            pos+=D(ITEM_TEXT_OFS_Y);
            wsprintf(bufw,L"%s",STR(STR_EMPTYDRP));
            SetTextColor(hdc,D(boxindex[box_status(index)]+14));
            TextOut(hdc,x+D(ITEM_TEXT_OFS_X),pos,bufw,wcslen(bufw));
            wsprintf(bufw,L"%s",manager->matcher->col->driverpack_dir);
            SetTextColor(hdc,D(boxindex[box_status(index)]+15));
            TextOut(hdc,x+D(ITEM_TEXT_OFS_X),pos+D(ITEM_TEXT_DIST_Y),bufw,wcslen(bufw));
            break;

        case SLOT_SNAPSHOT:
            pos+=D(ITEM_TEXT_OFS_Y);
            wsprintf(bufw,L"%s",state_file);
            SetTextColor(hdc,D(boxindex[box_status(index)]+14));
            TextOut(hdc,x+D(ITEM_TEXT_OFS_X),pos,bufw,wcslen(bufw));
            wsprintf(bufw,L"%s",STR(STR_CLOSE_SNAPSHOT));
            SetTextColor(hdc,D(boxindex[box_status(index)]+15));
            TextOut(hdc,x+D(ITEM_TEXT_OFS_X),pos+D(ITEM_TEXT_DIST_Y),bufw,wcslen(bufw));
            break;

        case SLOT_DPRDIR:
            pos+=D(ITEM_TEXT_OFS_Y);
            wsprintf(bufw,L"%s",drpext_dir);
            SetTextColor(hdc,D(boxindex[box_status(index)]+14));
            TextOut(hdc,x+D(ITEM_TEXT_OFS_X),pos,bufw,wcslen(bufw));
            wsprintf(bufw,L"%s",STR(STR_CLOSE_DRPEXT));
            SetTextColor(hdc,D(boxindex[box_status(index)]+15));
            TextOut(hdc,x+D(ITEM_TEXT_OFS_X),pos+D(ITEM_TEXT_DIST_Y),bufw,wcslen(bufw));
            break;

        case SLOT_VIRUS_AUTORUN:
            pos+=D(ITEM_TEXT_OFS_Y);
            wsprintf(bufw,L"%s",STR(STR_VIRUS));
            SetTextColor(hdc,D(boxindex[box_status(index)]+14));
            TextOut(hdc,x+D(ITEM_TEXT_OFS_X),pos,bufw,wcslen(bufw));
            wsprintf(bufw,L"%s",STR(STR_VIRUS_AUTORUN));
            SetTextColor(hdc,D(boxindex[box_status(index)]+15));
            TextOut(hdc,x+D(ITEM_TEXT_OFS_X),pos+D(ITEM_TEXT_DIST_Y),bufw,wcslen(bufw));
            break;

        case SLOT_VIRUS_RECYCLER:
            pos+=D(ITEM_TEXT_OFS_Y);
            wsprintf(bufw,L"%s",STR(STR_VIRUS));
            SetTextColor(hdc,D(boxindex[box_status(index)]+14));
            TextOut(hdc,x+D(ITEM_TEXT_OFS_X),pos,bufw,wcslen(bufw));
            wsprintf(bufw,L"%s",STR(STR_VIRUS_RECYCLER));
            SetTextColor(hdc,D(boxindex[box_status(index)]+15));
            TextOut(hdc,x+D(ITEM_TEXT_OFS_X),pos+D(ITEM_TEXT_DIST_Y),bufw,wcslen(bufw));
            break;

        case SLOT_VIRUS_HIDDEN:
            pos+=D(ITEM_TEXT_OFS_Y);
            wsprintf(bufw,L"%s",STR(STR_VIRUS));
            SetTextColor(hdc,D(boxindex[box_status(index)]+14));
            TextOut(hdc,x+D(ITEM_TEXT_OFS_X),pos,bufw,wcslen(bufw));
            wsprintf(bufw,L"%s",STR(STR_VIRUS_HIDDEN));
            SetTextColor(hdc,D(boxindex[box_status(index)]+15));
            TextOut(hdc,x+D(ITEM_TEXT_OFS_X),pos+D(ITEM_TEXT_DIST_Y),bufw,wcslen(bufw));
            break;

        default:
            if(itembar->hwidmatch)
            {
                // Checkbox
                drawcheckbox(hdc,x+D(ITEM_CHECKBOX_OFS_X),pos+D(ITEM_CHECKBOX_OFS_Y),
                         D(ITEM_CHECKBOX_SIZE),D(ITEM_CHECKBOX_SIZE),
                         itembar->checked,cur_i==index);

                // Available driver desc
                pos+=D(ITEM_TEXT_OFS_Y);
                wsprintf(bufw,L"%S",getdrp_drvdesc(itembar->hwidmatch));
                SetTextColor(hdc,D(boxindex[box_status(index)]+14));
                TextOut(hdc,x+D(ITEM_TEXT_OFS_X),pos,bufw,wcslen(bufw));

                // Available driver status
                SetTextColor(hdc,D(boxindex[box_status(index)]+15));
                str_status(bufw,itembar);
                if(itembar->install_status)
                {
                    wsprintf(bufw,itembar->install_status==STR_INST_FAILED?L"%s %X":L"%s",
                             STR(itembar->install_status),itembar->val1);
//                    if(itembar->install_status==STR_INST_INSTALL&&(installmode==2||!itembar->checked))
//                    if(installmode==2&&!itembar->checked)
                    if(itembar->install_status==STR_INST_EXTRACT&&(installmode==2||!itembar->checked))
                        wcscpy(bufw,STR(STR_INST_STOPPING));

                    SetTextColor(hdc,0);
                }
                TextOut(hdc,x+D(ITEM_TEXT_OFS_X),pos+D(ITEM_TEXT_DIST_Y),bufw,wcslen(bufw));
            }
            else
            {
                // Device desc
                if(itembar->devicematch)
                {
                    wsprintf(bufw,L"%ws",manager->matcher->state->text+itembar->devicematch->device->Devicedesc);
                    SetTextColor(hdc,D(boxindex[box_status(index)]+14));
                    TextOut(hdc,x+D(ITEM_TEXT_OFS_X),pos,bufw,wcslen(bufw));

                    str_status(bufw,itembar);
                    SetTextColor(hdc,D(boxindex[box_status(index)]+15));
                    TextOut(hdc,x+D(ITEM_TEXT_OFS_X),pos+D(ITEM_TEXT_DIST_Y),bufw,wcslen(bufw));
                }
            }
            // Device icon
            if(itembar->devicematch&&SetupDiLoadClassIcon(&itembar->devicematch->device->DeviceInfoData.ClassGuid,&hIcon,0))
            {
                DrawIconEx(hdc,x+D(ITEM_ICON_OFS_X),pos+D(ITEM_ICON_OFS_Y),hIcon,D(ITEM_ICON_SIZE),D(ITEM_ICON_SIZE),0,0,DI_NORMAL);
                DestroyIcon(hIcon);
            }

            // Expand icon
            if(groupsize(manager,itembar->index)>1)
                image_draw(hdc,&icon[(itembar->isactive&2?0:2)+((cur_i==index&&zone==2)?1:0)],x+D(DRVITEM_WX)-D(ITEM_ICON_SIZE)*2+10,pos,32,32,0,HSTR|VSTR);
            break;

    }

    SelectClipRgn(hdc,canvasField.clipping);
    DeleteObject(hrgn);
}

int isbehind(manager_t *manager,int pos,int ofs,int j)
{
    itembar_t *itembar;

    j--;

    if(pos<=-D(ITEM_DIST_Y0))return 1;
    if(pos>mainy_c)return 1;
    if(j<0)return 0;

    itembar=&manager->items_list[j];
    if((itembar->curpos>>16)-ofs==pos)return 1;

    return 0;
}

void manager_draw(manager_t *manager,HDC hdc,int ofsy)
{
    itembar_t *itembar;
    int i;
    int pos=0;
    int maxpos=0;
    int nm=0;

    for(i=manager->items_handle.items-1;i>=0;i--)
    {
        itembar=&manager->items_list[i];
        if(itembar->isactive)continue;
        if(isbehind(manager,(itembar->curpos>>16)-ofsy,ofsy,i))continue;
        nm++;

        pos=(itembar->curpos>>16)-ofsy-D(ITEM_DIST_Y0);
        manager_drawitem(manager,hdc,pos,itembar,i);
        //printf("1[%d]:%d\n",nm,pos);
    }
    for(i=manager->items_handle.items-1;i>=0;i--)
    {
        itembar=&manager->items_list[i];
        if(itembar->isactive==0)continue;
        pos=(itembar->curpos>>16)-ofsy-D(ITEM_DIST_Y0);
        if(itembar->curpos>maxpos)maxpos=itembar->curpos;
        if(pos<=-D(ITEM_DIST_Y0)&&i>=SLOT_RESTORE_POINT)continue;
        if(pos>mainy_c)continue;
        nm++;
        manager_drawitem(manager,hdc,pos,itembar,i);
        //printf("2[%d]:%d\n",nm,pos);
    }
    //printf("nm:%d\n",nm);
    setscrollrange((maxpos>>16)+20);
}

int itembar_cmp(itembar_t *a,itembar_t *b,CHAR *ta,CHAR *tb)
{
    if(wcscmp((WCHAR*)(ta+a->devicematch->device->Driver),(WCHAR*)(tb+b->devicematch->device->Driver)))return 0;
    if(a->hwidmatch&&b->hwidmatch&&a->hwidmatch->HWID_index!=b->hwidmatch->HWID_index)return 0;

    return 1;
}

void manager_restorepos(manager_t *manager_new,manager_t *manager_old)
{
    itembar_t *itembar_new,*itembar_old;
    CHAR *t_new,*t_old;
    int i,j;

    t_old=manager_old->matcher->state->text;
    t_new=manager_new->matcher->state->text;

    if(manager_old->items_list[SLOT_EMPTY].percent==1)
    {
        return;
    }

    itembar_new=&manager_new->items_list[RES_SLOTS];
    for(i=RES_SLOTS;i<manager_new->items_handle.items;i++,itembar_new++)
    {
        itembar_old=&manager_old->items_list[RES_SLOTS];
        for(j=RES_SLOTS;j<manager_old->items_handle.items;j++,itembar_old++)
        {
            if(itembar_old->isactive!=9&&itembar_cmp(itembar_new,itembar_old,t_new,t_old))
            {
                itembar_new->checked=itembar_old->checked;
                itembar_new->curpos=itembar_old->curpos;
                itembar_new->percent=itembar_old->percent;
                itembar_new->install_status=itembar_old->install_status;
                itembar_new->val1=itembar_old->val1;
                itembar_new->val2=itembar_old->val2;
                wcscpy(itembar_new->txt1,itembar_old->txt1);

                itembar_old->isactive=9;
                break;
            }
        }
/*        if(j==manager_old->items_handle.items)
        {
            log_err("Added %d\n",i);
            device_print(itembar_new->devicematch,manager_new->matcher->state);
            int limits[7];
            hwidmatch_calclen(itembar_new->hwidmatch,limits);
            hwidmatch_print(itembar_new->hwidmatch,limits);
        }*/
    }

/*    log("{Updated\n");
    itembar_old=&manager_old->items_list[RES_SLOTS];
    for(j=RES_SLOTS;j<manager_old->items_handle.items;j++,itembar_old++)
    {
        if(itembar_old->isactive!=9)
        {
            int limits[7];
            hwidmatch_calclen(itembar_old->hwidmatch,limits);
            hwidmatch_print(itembar_old->hwidmatch,limits);
//            log_err("UNK %d\n",j);
        }
    }
    log("}Updated\n");*/
}
//}

//{ Draw
void TextOut_CM(HDC hdcMem,int x,int y,WCHAR *str,int color,int *maxsz,int mode)
{
    SIZE ss;
    GetTextExtentPoint32(hdcMem,str,wcslen(str),&ss);
    if(ss.cx>*maxsz)*maxsz=ss.cx;

    if(!mode)return;
    SetTextColor(hdcMem,color);
    TextOut(hdcMem,x,y,str,wcslen(str));
    SetTextColor(hdcMem,0);
}

void TextOutP(textdata_t *td,WCHAR *format,...)
{
    WCHAR buffer[BUFLEN];
    va_list args;
    va_start(args,format);
    _vsnwprintf(buffer,BUFLEN,format,args);

    TextOut_CM(td->hdcMem,td->x,td->y,buffer,td->col,&td->limits[td->i],td->mode);
    td->x+=td->limits[td->i];
    td->i++;
    va_end(args);
}

void TextOutF(textdata_t *td,int col,WCHAR *format,...)
{
    WCHAR buffer[BUFLEN];
    va_list args;
    va_start(args,format);
    _vsnwprintf(buffer,BUFLEN,format,args);

    TextOut_CM(td->hdcMem,td->x,td->y,buffer,col,&td->maxsz,1);
    td->y+=td->wy;
    va_end(args);
}

void TextOutSF(textdata_t *td,WCHAR *str,WCHAR *format,...)
{
    WCHAR buffer[BUFLEN];
    va_list args;
    va_start(args,format);
    _vsnwprintf (buffer,BUFLEN,format,args);
    TextOut_CM(td->hdcMem,td->x,td->y,str,td->col,&td->maxsz,1);
    TextOut_CM(td->hdcMem,td->x+95,td->y,buffer,td->col,&td->maxsz,1);
    td->y+=td->wy;
    va_end(args);
}
//}

//{ Popup
void popup_resize(int x,int y)
{
    if(floating_x!=x||floating_y!=y)
    {
        POINT p1;

        floating_x=x;
        floating_y=y;
        GetCursorPos(&p1);
        SetCursorPos(p1.x+1,p1.y);
        SetCursorPos(p1.x,p1.y);
    }
}

void popup_driverline(hwidmatch_t *hwidmatch,int *limits,HDC hdcMem,int y,int mode)
{
    version_t *v;
    char buf[BUFLEN];
    WCHAR bufw[BUFLEN];
    textdata_t td;

    td.hdcMem=hdcMem;
    td.i=0;
    td.limits=limits;
    td.x=D(POPUP_OFSX)+horiz_sh;
    td.y=y;
    td.col=0;
    td.mode=mode;

    v=getdrp_drvversion(hwidmatch);
    getdrp_drvsection(hwidmatch,buf);
    str_date(v,bufw);

    if(!hwidmatch->altsectscore)td.col=D(POPUP_LST_INVALID_COLOR);
    else
    {
        if(hwidmatch->status&STATUS_BETTER||hwidmatch->status&STATUS_NEW)td.col=D(POPUP_LST_BETTER_COLOR);else
        if(hwidmatch->status&STATUS_WORSE||hwidmatch->status&STATUS_OLD)td.col=D(POPUP_LST_WORSE_COLOR);else
        td.col=D(POPUP_TEXT_COLOR);
    }

    TextOutP(&td,L"%d",hwidmatch->altsectscore);
    TextOutP(&td,L"| %08X",hwidmatch->score);
    TextOutP(&td,L"| %s",bufw);
    TextOutP(&td,L"| %3d",hwidmatch->decorscore);
    TextOutP(&td,L"| %d",hwidmatch->markerscore);
    TextOutP(&td,L"| %3X",hwidmatch->status);
    TextOutP(&td,L"| %S",buf);
    TextOutP(&td,L"| %s\\%s",getdrp_packpath(hwidmatch),getdrp_packname(hwidmatch));
    TextOutP(&td,L"| %08X%",getdrp_infcrc(hwidmatch));
    TextOutP(&td,L"| %S%S",getdrp_infpath(hwidmatch),getdrp_infname(hwidmatch));
    TextOutP(&td,L"| %S",getdrp_drvmanufacturer(hwidmatch));
    TextOutP(&td,L"| %d.%d.%d.%d",v->v1,v->v2,v->v3,v->v4);
    TextOutP(&td,L"| %S",getdrp_drvHWID(hwidmatch));wsprintf(bufw,L"%S",getdrp_drvdesc(hwidmatch));
    TextOutP(&td,L"| %s",bufw);
}

void popup_driverlist(manager_t *manager,HDC hdcMem,RECT rect,int i)
{
    itembar_t *itembar;
    POINT p;
    WCHAR i_hwid[BUFLEN];
    WCHAR bufw[BUFLEN];
    int lne=12;
    int k;
    int maxsz=0;
    int limits[30];
    int c0=D(POPUP_TEXT_COLOR);
    textdata_t td;

    if(i<RES_SLOTS)return;

    td.hdcMem=hdcMem;
    td.i=0;
    td.limits=limits;
    td.x=D(POPUP_OFSX)+horiz_sh;
    td.y=D(POPUP_OFSY);
    td.col=0;
    td.mode=1;

    int group=manager->items_list[i].index;
    driver_t *cur_driver=manager->items_list[i].devicematch->driver;
    char *t=manager->matcher->state->text;

    memset(limits,0,sizeof(limits));

    itembar=manager->items_list;
    for(k=0;k<manager->items_handle.items;k++,itembar++)
        if(itembar->index==group&&itembar->hwidmatch)
            popup_driverline(itembar->hwidmatch,limits,hdcMem,td.y,0);


    TextOut_CM(hdcMem,10,td.y,STR(STR_HINT_INSTDRV),c0,&maxsz,1);td.y+=lne;

    if(cur_driver)
    {
        wsprintf(bufw,L"%s",t+cur_driver->MatchingDeviceId);
        for(k=0;bufw[k];k++)i_hwid[k]=toupper(bufw[k]);i_hwid[k]=0;
        str_date(&cur_driver->version,bufw);

        td.x+=limits[td.i++];
        td.col=c0;
        TextOutP(&td,L"| %08X",calc_score_h(cur_driver,manager->matcher->state));
        TextOutP(&td,L"| %s",bufw);
        for(k=0;k<6;k++)td.x+=limits[td.i++];
        TextOutP(&td,L"| %s%s",t+manager->matcher->state->windir,t+cur_driver->InfPath);
        TextOutP(&td,L"| %s",t+cur_driver->ProviderName);
        TextOutP(&td,cur_driver->version.v1<0?STR(STR_HINT_UNKNOWN):L"| %d.%d.%d.%d",cur_driver->version.v1,cur_driver->version.v2,cur_driver->version.v3,cur_driver->version.v4);
        TextOutP(&td,L"| %s",i_hwid);
        TextOutP(&td,L"| %s",t+cur_driver->DriverDesc);
        td.y+=lne;
    }
    td.y+=lne;
    TextOut_CM(hdcMem,10,td.y,STR(STR_HINT_AVAILDRVS),c0,&maxsz,1);td.y+=lne;

    itembar=manager->items_list;
    for(k=0;k<manager->items_handle.items;k++,itembar++)
        if(itembar->index==group&&itembar->hwidmatch)
    {
        if(k==i)
        {
            SelectObject(hdcMem,GetStockObject(DC_BRUSH));
            SelectObject(hdcMem,GetStockObject(DC_PEN));
//            SetDCBrushColor(hdcMem,RGB(115,125,255));
            SetDCBrushColor(hdcMem,RGB(255,255,255));
            Rectangle(hdcMem,D(POPUP_OFSX)+horiz_sh,td.y,rect.right+horiz_sh-D(POPUP_OFSX),td.y+lne);
        }
        popup_driverline(itembar->hwidmatch,limits,hdcMem,td.y,1);
        td.y+=lne;
    }

    GetWindowRect(GetDesktopWindow(),&rect);
    p.y=0;p.x=0;
    ClientToScreen(hPopup,&p);

    maxsz=0;
    for(k=0;k<30;k++)maxsz+=limits[k];
    if(p.x+maxsz+D(POPUP_OFSX)*3>rect.right)
    {
        td.y+=lne;
        TextOut_CM(hdcMem,D(POPUP_OFSX),td.y,STR(STR_HINT_SCROLL),c0,&maxsz,1);
        td.y+=lne;
    }
    popup_resize(maxsz+D(POPUP_OFSX)*3,td.y+D(POPUP_OFSY));
}

int pickcat(hwidmatch_t *hwidmatch,state_t *state)
{
    if(state->architecture==1&&*getdrp_drvcat(hwidmatch,CatalogFile_ntamd64))
    {
        return CatalogFile_ntamd64;
    }
    else if(*getdrp_drvcat(hwidmatch,CatalogFile_ntx86))
    {
        return CatalogFile_ntx86;
    }

    if(*getdrp_drvcat(hwidmatch,CatalogFile_nt))
       return CatalogFile_nt;

    if(*getdrp_drvcat(hwidmatch,CatalogFile))
       return CatalogFile;

    return 0;
}

int isvalidcat(hwidmatch_t *hwidmatch,state_t *state)
{
    CHAR bufa[BUFLEN];
    int n=pickcat(hwidmatch,state);
    char *s=getdrp_drvcat(hwidmatch,n);

    sprintf(bufa,"2:%d.%d",
            state->platform.dwMajorVersion,
            state->platform.dwMinorVersion);
    if(!*s)return 0;
    return strstr(s,bufa)?1:0;
}

void popup_drivercmp(manager_t *manager,HDC hdcMem,RECT rect,int i)
{
    WCHAR bufw[BUFLEN];
    WCHAR i_hwid[BUFLEN];
    WCHAR a_hwid[BUFLEN];
    char *t=manager->matcher->state->text;
    int maxln=0;
    int bolder=rect.right/2;
    WCHAR *p;
    devicematch_t *devicematch_f=0;
    hwidmatch_t *hwidmatch_f=0;
    driver_t *cur_driver=0;
    textdata_t td;
    version_t *a_v=0;
    unsigned score=0;
    int cm_ver=0,cm_date=0,cm_score=0,cm_hwid=0;
    int c0=D(POPUP_TEXT_COLOR),cb=D(POPUP_CMP_BETTER_COLOR);
    int p0=D(POPUP_OFSX),p1=D(POPUP_OFSX)+10;

    if(i<RES_SLOTS)return;

    devicematch_f=manager->items_list[i].devicematch;
    hwidmatch_f=manager->items_list[i].hwidmatch;

    td.y=D(POPUP_OFSY);
    td.wy=D(POPUP_WY);
    td.hdcMem=hdcMem;
    td.maxsz=0;

    if(!devicematch_f||!devicematch_f->device)MessageBox(0,L"Derp",L"Derp",0);

    if(devicematch_f->device->driver_index>=0)
    {
        cur_driver=&manager->matcher->state->drivers_list[devicematch_f->device->driver_index];
        wsprintf(bufw,L"%s",t+cur_driver->MatchingDeviceId);
        for(i=0;bufw[i];i++)i_hwid[i]=toupper(bufw[i]);i_hwid[i]=0;
    }
    if(hwidmatch_f)
    {
        a_v=getdrp_drvversion(hwidmatch_f);
        wsprintf(a_hwid,L"%S",getdrp_drvHWID(hwidmatch_f));
    }
    if(cur_driver&&hwidmatch_f)
    {
        int r=cmpdate(&cur_driver->version,a_v);
        if(r>0)cm_date=1;
        if(r<0)cm_date=2;

        score=calc_score_h(cur_driver,manager->matcher->state);
        if(score<hwidmatch_f->score)cm_score=1;
        if(score>hwidmatch_f->score)cm_score=2;

        r=cmpversion(&cur_driver->version,a_v);
        if(r>0)cm_ver=1;
        if(r<0)cm_ver=2;
    }

    td.x=p0;TextOutF(&td,c0,L"%s",STR(STR_HINT_ANALYSIS));td.x=p1;

    if(hwidmatch_f)
    {
        TextOutF(&td,isvalidcat(hwidmatch_f,manager->matcher->state)?cb:D(POPUP_CMP_INVALID_COLOR),
                 L"cat: (%d)%S",pickcat(hwidmatch_f,manager->matcher->state),getdrp_drvcat(hwidmatch_f,pickcat(hwidmatch_f,manager->matcher->state)));

        td.x=p0;TextOutF(&td,c0,L"%s",STR(STR_HINT_DRP));td.x=p1;
        TextOutF(&td,c0,L"%s\\%s",getdrp_packpath(hwidmatch_f),getdrp_packname(hwidmatch_f));
        TextOutF(&td,(!isLaptop&&strstr(getdrp_infpath(hwidmatch_f),"_nb\\"))!=0?D(POPUP_CMP_INVALID_COLOR):c0
                 ,L"%S%S",getdrp_infpath(hwidmatch_f),getdrp_infname(hwidmatch_f));
    }

    SetupDiGetClassDescription(&devicematch_f->device->DeviceInfoData.ClassGuid,bufw,BUFLEN,0);

    td.x=p0;TextOutF(&td,c0,L"%s",STR(STR_HINT_DEVICE));td.x=p1;
    TextOutF(&td,c0,L"%s",t+devicematch_f->device->Devicedesc);
    TextOutF(&td,c0,L"%s%s",STR(STR_HINT_MANUF),t+devicematch_f->device->Mfg);
    TextOutF(&td,c0,L"%s",bufw);

    maxln=td.y;
    td.y=D(POPUP_OFSY);
    if(devicematch_f->device->HardwareID)
    {
        td.x=p0+bolder;TextOutF(&td,c0,L"%s",STR(STR_HINT_HARDWAREID));td.x=p1+bolder;
        p=(WCHAR *)(t+devicematch_f->device->HardwareID);
        while(*p)
        {
            int pp=0;
            if(!wcsicmp(i_hwid,p))pp|=1;
            if(!wcsicmp(a_hwid,p))pp|=2;
            if(!cm_hwid&&(pp==1||pp==2))cm_hwid=pp;
            TextOutF(&td,pp?D(POPUP_HWID_COLOR):c0,L"%s",p);
            p+=lstrlen(p)+1;
        }
    }
    if(devicematch_f->device->CompatibleIDs)
    {
        td.x=p0+bolder;TextOutF(&td,c0,L"%s",STR(STR_HINT_COMPID));td.x=p1+bolder;
        p=(WCHAR *)(t+devicematch_f->device->CompatibleIDs);
        while(*p)
        {
            int pp=0;
            if(!wcsicmp(i_hwid,p))pp|=1;
            if(!wcsicmp(a_hwid,p))pp|=2;
            if(!cm_hwid&&(pp==1||pp==2))cm_hwid=pp;
            TextOutF(&td,pp?D(POPUP_HWID_COLOR):c0,L"%s",p);
            p+=lstrlen(p)+1;
        }
    }
    if(!cur_driver||!hwidmatch_f)cm_hwid=0;
    if(td.y>maxln)maxln=td.y;
    maxln+=td.wy;
    td.y=maxln;

    if(cur_driver||hwidmatch_f)
    {
        MoveToEx(hdcMem,0,td.y-td.wy/2,0);
        LineTo(hdcMem,rect.right,td.y-td.wy/2);
    }
    if(devicematch_f->device->HardwareID||hwidmatch_f)
    {
        MoveToEx(hdcMem,bolder,0,0);
        LineTo(hdcMem,bolder,rect.bottom);
    }
    if(cur_driver)
    {
        str_date(&cur_driver->version,bufw);

        td.x=p0;
        TextOutF(&td,               c0,L"%s",STR(STR_HINT_INSTDRV));td.x=p1;
        TextOutF(&td,               c0,L"%s",t+cur_driver->DriverDesc);
        TextOutF(&td,               c0,L"%s%s",STR(STR_HINT_PROVIDER),t+cur_driver->ProviderName);
        TextOutF(&td,cm_date ==1?cb:c0,L"%s%s",STR(STR_HINT_DATE),bufw);
        TextOutF(&td,cm_ver  ==1?cb:c0,str_version(&cur_driver->version),STR(STR_HINT_VERSION),cur_driver->version.v1,cur_driver->version.v2,cur_driver->version.v3,cur_driver->version.v4);
        TextOutF(&td,cm_hwid ==1?cb:c0,L"%s%s",STR(STR_HINT_ID),i_hwid);
        TextOutF(&td,               c0,L"%s%s",STR(STR_HINT_INF),t+cur_driver->InfPath);
        TextOutF(&td,               c0,L"%s%s%s",STR(STR_HINT_SECTION),t+cur_driver->InfSection,t+cur_driver->InfSectionExt);
        TextOutF(&td,cm_score==1?cb:c0,L"%s%08X",STR(STR_HINT_SCORE),score);
    }

    if(hwidmatch_f)
    {
        td.y=maxln;
        str_date(a_v,bufw);
        getdrp_drvsection(hwidmatch_f,(CHAR *)(bufw+500));

        td.x=p0+bolder;
        TextOutF(&td,               c0,L"%s",STR(STR_HINT_AVAILDRV));td.x=p1+bolder;wsprintf(bufw+1000,L"%S",getdrp_drvdesc(hwidmatch_f));
        TextOutF(&td,               c0,L"%s",bufw+1000);
        TextOutF(&td,               c0,L"%s%S",STR(STR_HINT_PROVIDER),getdrp_drvmanufacturer(hwidmatch_f));
        TextOutF(&td,cm_date ==2?cb:c0,L"%s%s",STR(STR_HINT_DATE),bufw);
        TextOutF(&td,cm_ver  ==2?cb:c0,str_version(a_v),STR(STR_HINT_VERSION),a_v->v1,a_v->v2,a_v->v3,a_v->v4);
        TextOutF(&td,cm_hwid ==2?cb:c0,L"%s%S",STR(STR_HINT_ID),getdrp_drvHWID(hwidmatch_f));
        TextOutF(&td,               c0,L"%s%S%S",STR(STR_HINT_INF),getdrp_infpath(hwidmatch_f),getdrp_infname(hwidmatch_f));
        TextOutF(&td,hwidmatch_f->decorscore?c0:D(POPUP_CMP_INVALID_COLOR),L"%s%S",STR(STR_HINT_SECTION),bufw+500);
        TextOutF(&td,cm_score==2?cb:c0,L"%s%08X",STR(STR_HINT_SCORE),hwidmatch_f->score);
    }

    if(!devicematch_f->device->HardwareID&&!hwidmatch_f)td.maxsz/=2;

    popup_resize((td.maxsz+10+p0*2)*2,td.y+D(POPUP_OFSY));
}

void popup_about(HDC hdcMem)
{
    textdata_t td;
    RECT rect;
    int p0=D(POPUP_OFSX);

    td.col=D(POPUP_TEXT_COLOR);
    td.wy=D(POPUP_WY);
    td.y=D(POPUP_OFSY);
    td.hdcMem=hdcMem;
    td.maxsz=0;

    td.x=p0;
    TextOutF(&td,td.col,L"Snappy Driver Installer %s",STR(STR_ABOUT_VER));
    td.y+=td.wy;
    rect.left=td.x;
    rect.top=td.y;
    rect.right=D(POPUP_WX)-p0*2;
    rect.bottom=80;
    DrawText(hdcMem,STR(STR_ABOUT_LICENSE),-1,&rect,DT_WORDBREAK);
    td.y+=td.wy*3;
    TextOutF(&td,td.col,L"%s%s",STR(STR_ABOUT_DEV_TITLE),STR(STR_ABOUT_DEV_LIST));
    TextOutF(&td,td.col,L"%s%s",STR(STR_ABOUT_TESTERS_TITLE),STR(STR_ABOUT_TESTERS_LIST));

    popup_resize(D(POPUP_WX),td.y+D(POPUP_OFSY));
}

void popup_sysinfo(manager_t *manager,HDC hdcMem)
{
    WCHAR bufw[BUFLEN];
    textdata_t td;
    state_t *state=manager->matcher->state;
    SYSTEM_POWER_STATUS *battery;
    WCHAR *buf;
    int i,x,y;
    int p0=D(POPUP_OFSX),p1=D(POPUP_OFSX)+10;

    td.col=D(POPUP_TEXT_COLOR);
    td.y=D(POPUP_OFSY);
    td.wy=D(POPUP_WY);
    td.hdcMem=hdcMem;
    td.maxsz=0;

    td.x=p0;
    TextOutF(&td,td.col,STR(STR_SYSINF_WINDOWS));td.x=p1;

    TextOutSF(&td,STR(STR_SYSINF_VERSION),L"%s (%d.%d.%d)",get_winverstr(manager_g),state->platform.dwMajorVersion,state->platform.dwMinorVersion,state->platform.dwBuildNumber);
    TextOutSF(&td,STR(STR_SYSINF_UPDATE),L"%s",state->platform.szCSDVersion);
    TextOutSF(&td,STR(STR_SYSINF_CPU_ARCH),L"%s",state->architecture?STR(STR_SYSINF_64BIT):STR(STR_SYSINF_32BIT));
    TextOutSF(&td,STR(STR_SYSINF_LOCALE),L"%X",state->locale);
    //TextOutSF(&td,STR(STR_SYSINF_PLATFORM),L"%d",state->platform.dwPlatformId);
    /*if(state->platform.dwOSVersionInfoSize == sizeof(OSVERSIONINFOEX))
    {
        TextOutSF(&td,STR(STR_SYSINF_SERVICEPACK),L"%d.%d",state->platform.wServicePackMajor,state->platform.wServicePackMinor);
        TextOutSF(&td,STR(STR_SYSINF_SUITEMASK),L"%d",state->platform.wSuiteMask);
        TextOutSF(&td,STR(STR_SYSINF_PRODUCTTYPE),L"%d",state->platform.wProductType);
    }*/
    td.x=p0;
    TextOutF(&td,td.col,STR(STR_SYSINF_ENVIRONMENT));td.x=p1;
    TextOutSF(&td,STR(STR_SYSINF_WINDIR),L"%s",state->text+state->windir);
    TextOutSF(&td,STR(STR_SYSINF_TEMP),L"%s",state->text+state->temp);

    td.x=p0;
    TextOutF(&td,td.col,STR(STR_SYSINF_MOTHERBOARD));td.x=p1;
    TextOutSF(&td,STR(STR_SYSINF_PRODUCT),L"%s",state->text+state->product);
    TextOutSF(&td,STR(STR_SYSINF_MODEL),L"%s",state->text+state->model);
    TextOutSF(&td,STR(STR_SYSINF_MANUF),L"%s",state->text+state->manuf);
    TextOutSF(&td,STR(STR_SYSINF_TYPE),L"%s",isLaptop?STR(STR_SYSINF_LAPTOP):STR(STR_SYSINF_DESKTOP));

    td.x=p0;
    TextOutF(&td,td.col,STR(STR_SYSINF_BATTERY));td.x=p1;
    battery=(SYSTEM_POWER_STATUS *)(state->text+state->battery);
    switch(battery->ACLineStatus)
    {
        case 0:wcscpy(bufw,STR(STR_SYSINF_OFFLINE));break;
        case 1:wcscpy(bufw,STR(STR_SYSINF_ONLINE));break;
        default:
        case 255:wcscpy(bufw,STR(STR_SYSINF_UNKNOWN));break;
    }
    TextOutSF(&td,STR(STR_SYSINF_AC_STATUS),L"%s",bufw);

    i=battery->BatteryFlag;
    *bufw=0;
    if(i&1)wcscat(bufw,STR(STR_SYSINF_HIGH));
    if(i&2)wcscat(bufw,STR(STR_SYSINF_LOW));
    if(i&4)wcscat(bufw,STR(STR_SYSINF_CRITICAL));
    if(i&8)wcscat(bufw,STR(STR_SYSINF_CHARGING));
    if(i&128)wcscat(bufw,STR(STR_SYSINF_NOBATTERY));
    if(i==255)wcscat(bufw,STR(STR_SYSINF_UNKNOWN));
    TextOutSF(&td,STR(STR_SYSINF_FLAGS),L"%s",bufw);

    if(battery->BatteryLifePercent!=255)
        TextOutSF(&td,STR(STR_SYSINF_CHARGED),L"%d",battery->BatteryLifePercent);
    if(battery->BatteryLifeTime!=0xFFFFFFFF)
        TextOutSF(&td,STR(STR_SYSINF_LIFETIME),L"%d %s",battery->BatteryLifeTime/60,STR(STR_SYSINF_MINS));
    if(battery->BatteryFullLifeTime!=0xFFFFFFFF)
        TextOutSF(&td,STR(STR_SYSINF_FULLLIFETIME),L"%d %s",battery->BatteryFullLifeTime/60,STR(STR_SYSINF_MINS));

    buf=(WCHAR *)(state->text+state->monitors);
    td.x=p0;
    TextOutF(&td,td.col,STR(STR_SYSINF_MONITORS));td.x=p1;
    for(i=0;i<buf[0];i++)
    {
        x=buf[1+i*2];
        y=buf[2+i*2];
        td.maxsz+=95;
        TextOutF(&td,td.col,L"%d%s x %d%s (%.1f %s) %.3f %s",
                  x,STR(STR_SYSINF_CM),
                  y,STR(STR_SYSINF_CM),
                  sqrt(x*x+y*y)/2.54,STR(STR_SYSINF_INCH),
                  (double)y/x,
                  iswide(x,y)?STR(STR_SYSINF_WIDE):L"");

        td.maxsz-=95;
    }

    td.x=p0;
    td.maxsz+=95;
    td.y+=td.wy;
    TextOutF(&td,td.col,STR(STR_SYSINF_MISC));td.x=p1;
    td.maxsz-=95;
    popup_resize((td.maxsz+95+p0+p1),td.y+D(POPUP_OFSY));
}
//}
