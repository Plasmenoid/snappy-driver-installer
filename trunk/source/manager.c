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
        itembar_init(itembar,0,0,i,0,1);
    }
}

void manager_free(manager_t *manager)
{
    heap_free(&manager->items_handle);
}

void manager_sorta(matcher_t *m,int *v)
{
    devicematch_t *devicematch_i,*devicematch_j;
    hwidmatch_t *hwidmatch_i,*hwidmatch_j;
    int i,j,num;

    num=m->devicematch_handle.items;

    for(i=0;i<num;i++)v[i]=i;

    for(i=0;i<num;i++)
    {
        for(j=i+1;j<num;j++)
        {
            devicematch_i=&m->devicematch_list[v[i]];
            devicematch_j=&m->devicematch_list[v[j]];
            hwidmatch_i=(devicematch_i->num_matches)?&m->hwidmatch_list[devicematch_i->start_matches]:0;
            hwidmatch_j=(devicematch_j->num_matches)?&m->hwidmatch_list[devicematch_j->start_matches]:0;

            if(devicematch_i->device->problem<devicematch_j->device->problem)
            {
                int t;

                t=v[i];
                v[i]=v[j];
                v[j]=t;
            }
            else
            if(devicematch_i->device->problem==devicematch_j->device->problem)
            if((hwidmatch_i&&hwidmatch_j&&wcscmp(getdrp_packname(hwidmatch_i),getdrp_packname(hwidmatch_j))>0)
               ||
               (!hwidmatch_i&&hwidmatch_j))
            {
                int t;

                t=v[i];
                v[i]=v[j];
                v[j]=t;
            }
        }
    }
}

int  manager_drplive(WCHAR *s)
{
    itembar_t *itembar;
    int k,needle=0;

    itembar=&manager_g->items_list[RES_SLOTS];
    for(k=RES_SLOTS;k<manager_g->items_handle.items;k++,itembar++)
    if(itembar->hwidmatch&&StrStrIW(getdrp_packname(itembar->hwidmatch),s))
    {
        if(itembar->isactive)
        {
            if(getdrp_packontorrent(itembar->hwidmatch))return 0;// Yes
        }
        needle=1;
    }
    return needle?1:1; // No/Unknown
}

void manager_populate(manager_t *manager)
{
    matcher_t *matcher=manager->matcher;
    devicematch_t *devicematch;
    hwidmatch_t *hwidmatch;
    int i,j,id=RES_SLOTS;
    int remap[1024];

    manager->items_handle.used=sizeof(itembar_t)*RES_SLOTS;
    manager->items_handle.items=RES_SLOTS;

    manager_sorta(matcher,remap);

    devicematch=matcher->devicematch_list;
    for(i=0;i<matcher->devicematch_handle.items;i++)
    {
        devicematch=&matcher->devicematch_list[remap[i]];
        hwidmatch=&matcher->hwidmatch_list[devicematch->start_matches];
        for(j=0;j<devicematch->num_matches;j++,hwidmatch++)
        {
            itembar_init(heap_allocitem_ptr(&manager->items_handle),devicematch,hwidmatch,i+RES_SLOTS,
                remap[i],j?2:2);
            itembar_init(heap_allocitem_ptr(&manager->items_handle),devicematch,hwidmatch,i+RES_SLOTS,
                remap[i],j?0:1);

            id++;
        }
        if(!devicematch->num_matches)
        {
            itembar_init(heap_allocitem_ptr(&manager->items_handle),devicematch,0,i+RES_SLOTS,remap[i],1);
            id++;
        }
    }
}

void manager_filter(manager_t *manager,int options)
{
    devicematch_t *devicematch;
    itembar_t *itembar,*itembar1,*itembar_drp=0,*itembar_drpcur=0;
    int i,j,k;
    int cnt[NUM_STATUS+1];
    int ontorrent;
    int o1=options&FILTER_SHOW_ONE;

    itembar=&manager->items_list[RES_SLOTS];

    for(i=RES_SLOTS;i<manager->items_handle.items;)
    {
        devicematch=itembar->devicematch;
        memset(cnt,0,sizeof(cnt));
        ontorrent=0;
        if(!devicematch){itembar++;i++;continue;}
        for(j=0;j<devicematch->num_matches;j++,itembar++,i++)
        {
            itembar->isactive=0;
            if(!itembar)log_con("ERROR a%d\n",j);
            //if(!itembar->hwidmatch)log_con("ERROR %d,%d\n",itembar->index,j);
            if(!itembar->hwidmatch)continue;


            if(itembar->first&2)
            {
                itembar->isactive=0;
                itembar_drp=itembar;
                j--;
                continue;
            }
            if(flags&FLAG_FILTERSP&&j)continue;

            if(itembar->checked||itembar->install_status)itembar->isactive=1;

            if((options&FILTER_SHOW_INVALID)==0&&!isdrivervalid(itembar->hwidmatch))
                continue;

            if((options&FILTER_SHOW_DUP)==0&&itembar->hwidmatch->status&STATUS_DUP)
                continue;

            if((options&FILTER_SHOW_DUP)&&itembar->hwidmatch->status&STATUS_DUP)
            {
                itembar1=&manager->items_list[i];
                for(k=0;k<devicematch->num_matches-j;k++,itembar1++)
                    if(itembar1->first&2)k--;
                        else
                    if(itembar1->isactive&&
                       itembar1->index==itembar->index&&
                       getdrp_infcrc(itembar1->hwidmatch)==getdrp_infcrc(itembar->hwidmatch))
                        break;

                if(k!=j)
                    itembar->isactive=1;
            }

            if((!o1||!cnt[NUM_STATUS])&&(options&FILTER_SHOW_MISSING)&&itembar->hwidmatch->status&STATUS_MISSING)
            {
                itembar->isactive=1;
                if(getdrp_packontorrent(itembar->hwidmatch)&&!ontorrent)
                    ontorrent=1;
                else
                    cnt[NUM_STATUS]++;
            }

            if(flags&FLAG_FILTERSP&&itembar->hwidmatch->altsectscore==2&&!isvalidcat(itembar->hwidmatch,manager->matcher->state))
                itembar->hwidmatch->altsectscore=1;

            for(k=0;k<NUM_STATUS;k++)
                if((!o1||!cnt[NUM_STATUS])&&(options&statustnl[k].filter)&&itembar->hwidmatch->status&statustnl[k].status)
            {
                if((options&FILTER_SHOW_WORSE_RANK)==0/*&&(options&FILTER_SHOW_OLD)==0*/&&(options&FILTER_SHOW_INVALID)==0&&
                   devicematch->device->problem==0&&devicematch->driver&&itembar->hwidmatch->altsectscore<2)continue;

                if((options&FILTER_SHOW_OLD)!=0&&(itembar->hwidmatch->status&STATUS_BETTER))continue;

                // hide if
                //[X] Newer
                //[ ] Worse
                //worse, no problem
                if((options&FILTER_SHOW_NEWER)!=0
                   &&(options&FILTER_SHOW_WORSE_RANK)==0&&(options&FILTER_SHOW_INVALID)==0
                   &&itembar->hwidmatch->status&STATUS_WORSE&&devicematch->device->problem==0&&devicematch->driver)continue;

                if(getdrp_packontorrent(itembar->hwidmatch)&&!ontorrent)
                    ontorrent=1;
                else
                {
                    cnt[k]++;
                    cnt[NUM_STATUS]++;
                }
                itembar->isactive=1;
            }


            if(itembar->isactive&&flags&FLAG_SHOWDRPNAMES2)
            {
                if(itembar_drp)
                {
                    if(!itembar_drpcur||(wcscmp(getdrp_packname(itembar_drp->hwidmatch),getdrp_packname(itembar_drpcur->hwidmatch))!=0))
                    {
                        itembar_drp->isactive=1;
                        itembar_drpcur=itembar_drp;
                    }
                }
            }

            if(!getdrp_packontorrent(itembar->hwidmatch))
                if(o1&&itembar->hwidmatch->status&STATUS_CURRENT)
                    cnt[NUM_STATUS]++;
        }
        if(!devicematch->num_matches)
        {
            itembar->isactive=0;
            if(options&FILTER_SHOW_NF_STANDARD&&devicematch->status&STATUS_NF_STANDARD)itembar->isactive=1;
            if(options&FILTER_SHOW_NF_UNKNOWN&&devicematch->status&STATUS_NF_UNKNOWN)itembar->isactive=1;
            if(options&FILTER_SHOW_NF_MISSING&&devicematch->status&STATUS_NF_MISSING)itembar->isactive=1;
            if(itembar->first&2)
            {
                itembar->isactive=0;
                itembar_drp=itembar;
            }
            itembar++;i++;
        }
    }
    i=0;
    itembar=&manager->items_list[RES_SLOTS];
    for(k=RES_SLOTS;k<manager->items_handle.items;k++,itembar++)
        if(itembar->isactive&&itembar->hwidmatch)i++;else itembar->checked=0;

    manager->items_list[SLOT_NOUPDATES].isactive=
        manager->items_handle.items==RES_SLOTS||
        (i==0&&statemode==0&&manager->matcher->col->driverpack_handle.items>1)?1:0;

    manager->items_list[SLOT_RESTORE_POINT].isactive=statemode==
        STATEMODE_LOAD||i==0||(flags&FLAG_NORESTOREPOINT)?0:1;
    //set_rstpnt(0);

    if(!manager->items_list[SLOT_RESTORE_POINT].install_status)
        manager->items_list[SLOT_RESTORE_POINT].install_status=STR_RESTOREPOINT;
}

void manager_print_tbl(manager_t *manager)
{
    itembar_t *itembar;
    int k,act=0;
    int limits[7];

    if((log_verbose&LOG_VERBOSE_MANAGER)==0)return;
    log_file("{manager_print\n");
    memset(limits,0,sizeof(limits));

    itembar=&manager->items_list[RES_SLOTS];
    for(k=RES_SLOTS;k<manager->items_handle.items;k++,itembar++)
        if(itembar->isactive&&itembar->hwidmatch)
            hwidmatch_calclen(itembar->hwidmatch,limits);

    itembar=&manager->items_list[RES_SLOTS];
    for(k=RES_SLOTS;k<manager->items_handle.items;k++,itembar++)
        if(itembar->isactive&&(itembar->first&2)==0)
        {
            log_file("$%04d|",k);
            if(itembar->hwidmatch)
                hwidmatch_print_tbl(itembar->hwidmatch,limits);
            else
                log_file("'%ws'\n",manager->matcher->state->text+itembar->devicematch->device->Devicedesc);
            act++;
        }else
        {
//            log_file("$%04d|^^ %d,%d\n",k,itembar->devicematch->num_matches,(itembar->hwidmatch)?itembar->hwidmatch->status:-1);
        }

    log_file("}manager_print[%d]\n\n",act);
}

void manager_print_hr(manager_t *manager)
{
    WCHAR buf[BUFLEN];
    itembar_t *itembar;
    int k,act=0;

    if((log_verbose&LOG_VERBOSE_MANAGER)==0)return;
    log_file("{manager_print\n");

    itembar=&manager->items_list[RES_SLOTS];
    for(k=RES_SLOTS;k<manager->items_handle.items;k++,itembar++)
        if(itembar->isactive&&(itembar->first&2)==0)
        {
            if(flags&FLAG_FILTERSP&&!isvalidcat(itembar->hwidmatch,manager->matcher->state))continue;
            str_status(buf,itembar);
            log_file("\n$%04d, %ws\n",k,buf);
            if(itembar->devicematch->device)
            {
                device_print(itembar->devicematch->device,manager->matcher->state);
                //device_printHWIDS(itembar->devicematch->device,manager->matcher->state);
            }
            if(itembar->devicematch->driver)
            {
                log_file("Installed driver\n");
                driver_print(itembar->devicematch->driver,manager->matcher->state);
            }

            if(itembar->hwidmatch)
            {
                log_file("Available driver\n");
                hwidmatch_print_hr(itembar->hwidmatch);
            }

            act++;
        }else
        {
//            log_file("$%04d|^^ %d,%d\n",k,itembar->devicematch->num_matches,(itembar->hwidmatch)?itembar->hwidmatch->status:-1);
        }

    log_file("}manager_print[%d]\n\n",act);
}

//{ User interaction
// Zones:
// 0 button
// 1 checkbox
// 2 downarrow
// 3 text
void manager_hitscan(manager_t *manager,int x,int y,int *r,int *zone)
{
    itembar_t *itembar;
    int i;
    int pos;
    int ofsy=getscrollpos();
    int cutoff=calc_cutoff(manager)+D(DRVITEM_DIST_Y0);
    int ofs=0;
    int wx=XG(D(DRVITEM_WX),Xg(D(DRVITEM_OFSX)));

    *r=-2;
    *zone=0;
    int cnt=0;

    y-=-D(DRVITEM_DIST_Y0);
    x-=Xg(D(DRVITEM_OFSX));
    if(kbpanel==KB_NONE)if(x<0||x>wx)return;
    itembar=manager->items_list;
    for(i=0;i<manager->items_handle.items;i++,itembar++)
    if(itembar->isactive&&(itembar->first&2)==0)
    {

        if(kbpanel==KB_FIELD)
        {
            *r=i;
            if(kbitem[kbpanel]==cnt)
            {
        log_con("%d\n",kbitem[kbpanel]);
                return;
            }
            cnt++;
            continue;
        }
        pos=itembar->curpos>>16;
        if(i>=SLOT_RESTORE_POINT&&y<cutoff)continue;
        if(i>=SLOT_RESTORE_POINT)pos-=ofsy;
        if(y>pos&&y<pos+D(DRVITEM_WY))
        {
            x-=D(ITEM_CHECKBOX_OFS_X);
            y-=D(ITEM_CHECKBOX_OFS_Y)+pos;
            ofs=(itembar->first&1)?0:D(DRVITEM_LINE_INTEND);
            if(x-ofs>0)*r=i;
            if(x-ofs>0&&x-ofs<D(ITEM_CHECKBOX_SIZE)&&y>0&&y<D(ITEM_CHECKBOX_SIZE))*zone=1;
            if(x>wx-50&&!ofs)*zone=expertmode?2:2;
            if(!*zone&&(x-ofs<D(ITEM_CHECKBOX_SIZE)))*zone=3;
            if(!*zone&&(x>240+190))*zone=3;
            if(kbpanel==KB_NONE)return;
        }
    }
    if(kbpanel==KB_FIELD)
    {
        kbitem[kbpanel]=cnt-1;
        log_con("%d,%d\n",kbitem[kbpanel],*r);
        return;
    }
    *r=-1;
}

void manager_clear(manager_t *manager)
{
    itembar_t *itembar;
    int i;

    itembar=&manager->items_list[RES_SLOTS];
    for(i=RES_SLOTS;i<manager->items_handle.items;i++,itembar++)
    {
        itembar->install_status=0;
        itembar->percent=0;
    }
    manager->items_list[SLOT_EXTRACTING].isactive=0;
    manager->items_list[SLOT_RESTORE_POINT].install_status=STR_RESTOREPOINT;
    manager_filter(manager,filters);
    manager_setpos(manager);
    PostMessage(hMain,WM_DEVICECHANGE,7,2);
}

void manager_testitembars(manager_t *manager)
{
    itembar_t *itembar;
    int i,j=0,index=1;

    itembar=manager->items_list;

    manager_filter(manager,FILTER_SHOW_CURRENT|FILTER_SHOW_NEWER);
    wcscpy(drpext_dir,L"drpext");
    manager->items_list[SLOT_EMPTY].curpos=1;

    for(i=0;i<manager->items_handle.items;i++,itembar++)
    if(i>SLOT_EMPTY&&i<RES_SLOTS)
    {
        if(i==SLOT_VIRUS_HIDDEN||i==SLOT_VIRUS_RECYCLER||i==SLOT_NODRIVERS||i==SLOT_DPRDIR)continue;
        itembar->index=index;
        itembar->isactive=1;
    }
    else if(itembar->isactive)
    {
        itembar->checked=0;
        if(j==0||j==6||j==9||j==18||j==21)index++;
        itembar->index=index;
        itembar->hwidmatch->altsectscore=2;
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

#ifdef USE_TORRENT
    if(installmode&&!torrentstatus.sessionpaused)
        return;
#endif

    itembar1=&manager->items_list[index];
    if(index>=RES_SLOTS&&!itembar1->hwidmatch)return;
    itembar1->checked^=1;
    if(!itembar1->checked&&installmode)
    {
        itembar1->install_status=STR_INST_STOPPING;
    }
    if(index==SLOT_RESTORE_POINT)
    {
        set_rstpnt(itembar1->checked);
    }
    group=itembar1->index;

    itembar=manager->items_list;
    for(i=0;i<manager->items_handle.items;i++,itembar++)
        if(itembar!=itembar1&&itembar->index==group)
            itembar->checked&=~1;

    if(itembar1->checked&&itembar1->isactive&2)
        manager_expand(manager,index);
    else
        redrawmainwnd();
}

void manager_expand(manager_t *manager,int index)
{
    itembar_t *itembar,*itembar1;
    int i,group;

    itembar1=&manager->items_list[index];
    group=itembar1->index;

    itembar=manager->items_list;
    if((itembar1->isactive&2)==0)// collapsed
    {
        for(i=0;i<manager->items_handle.items;i++,itembar++)
            if(itembar->index==group&&itembar->hwidmatch&&(itembar->hwidmatch->status&STATUS_INVALID)==0&&(itembar->first&2)==0)
                {
                    itembar->isactive|=2; // expand
                }
    }
    else
    {
        for(i=0;i<manager->items_handle.items;i++,itembar++)
            if(itembar->index==group&&(itembar->first&2)==0)
            {
                itembar->isactive&=1; //collapse
                if(itembar->checked)itembar->isactive|=4;
            }
    }
    manager_setpos(manager_g);
}

void manager_selectnone(manager_t *manager)
{
    itembar_t *itembar;
    int i;

#ifdef USE_TORRENT
    if(installmode&&!torrentstatus.sessionpaused)
        return;
#endif

    if(manager->items_list[SLOT_RESTORE_POINT].isactive)
    {
        set_rstpnt(0);
    }
    itembar=&manager->items_list[RES_SLOTS];
    for(i=RES_SLOTS;i<manager->items_handle.items;i++,itembar++)itembar->checked=0;
}

void manager_selectall(manager_t *manager)
{
    itembar_t *itembar;
    int i,group=-1;

#ifdef USE_TORRENT
    if(installmode&&!torrentstatus.sessionpaused)
        return;
#endif

    itembar=&manager->items_list[SLOT_RESTORE_POINT];
    if(itembar->install_status==STR_RESTOREPOINT&&itembar->isactive)
        set_rstpnt(1);

    itembar=&manager->items_list[RES_SLOTS];
    for(i=RES_SLOTS;i<manager->items_handle.items;i++,itembar++)
    {
        itembar->checked=0;
        if(itembar->isactive&&group!=itembar->index&&itembar->hwidmatch&&(itembar->first&2)==0)
        {
            if(itembar->install_status==0)itembar->checked=1;
            group=itembar->index;
        }
    }
}
//}

//{ Helpers
void itembar_init(itembar_t *item,devicematch_t *devicematch,hwidmatch_t *hwidmatch,int groupindex,int rm,int first)
{
    memset(item,0,sizeof(itembar_t));
    item->devicematch=devicematch;
    item->hwidmatch=hwidmatch;
    item->curpos=(-D(DRVITEM_DIST_Y0))<<16;
    item->tagpos=(-D(DRVITEM_DIST_Y0))<<16;
    item->index=groupindex;
    item->rm=rm;
    item->first=first;
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
        *pos+=*cnt?D(DRVITEM_DIST_Y1):D(DRVITEM_DIST_Y0);
        (*cnt)--;
    }
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
                wsprintf(buf,L"%s",STR(STR_STATUS_MISSING),itembar->devicematch->device->problem);
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
        if(getdrp_packontorrent(itembar->hwidmatch))wcscat(buf,STR(STR_UPD_WEBSTATUS));
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

        case SLOT_NODRIVERS:
        case SLOT_DPRDIR:
        case SLOT_SNAPSHOT:
            return BOX_DRVITEM_IF;

        case SLOT_DOWNLOAD:
        case SLOT_NOUPDATES:
            return manager_g->items_handle.items>RES_SLOTS?BOX_NOUPDATES:BOX_DRVITEM_IF;

        case SLOT_RESTORE_POINT:
            switch(itembar->install_status)
            {
                case STR_REST_CREATING:
                    return BOX_DRVITEM_D0;

                case STR_REST_CREATED:
                    return BOX_DRVITEM_D1;

                case STR_REST_FAILED:
                    return BOX_DRVITEM_DE;

                default:
                    break;
            }
            break;

        case SLOT_EXTRACTING:
            switch(itembar->install_status)
            {
                case STR_EXTR_EXTRACTING:
                case STR_INST_INSTALLING:
                    return BOX_DRVITEM_D0;

                case STR_INST_COMPLITED:
                    return BOX_DRVITEM_D1;

                case STR_INST_COMPLITED_RB:
                    return BOX_DRVITEM_D2;

                case STR_INST_STOPPING:
                    return BOX_DRVITEM_DE;

                default:break;
            }
            break;

        default:
            break;
    }
    if(itembar->hwidmatch)
    {
        int status=itembar->hwidmatch->status;

        if(itembar->first&2)return BOX_DRVITEM_PN;

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
                case STR_EXTR_OK:
                    return BOX_DRVITEM_D1;

                case STR_INST_REBOOT:
                    return BOX_DRVITEM_D2;

                case STR_INST_FAILED:
                case STR_EXTR_FAILED:
                    return BOX_DRVITEM_DE;

                default:break;
            }
            if(status&STATUS_MISSING)
                return BOX_DRVITEM_MS;
            else
            {
                if(itembar->hwidmatch->altsectscore<2)return BOX_DRVITEM_WO;

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
    itembar_t *itembar,*lastitembar=0;
    int k;
    int cnt=0;
    int pos=D(DRVITEM_OFSY);
    //int pos=0;
    int group=0;
    int lastmatch=0;

//0:wide
//1:narrow

    itembar=manager->items_list;
    for(k=0;k<manager->items_handle.items;k++,itembar++)
    {
        devicematch=itembar->devicematch;
        cnt=group==itembar->index?1:0;

        //if(lastitembar&&lastitembar->index<SLOT_RESTORE_POINT&&itembar->index<SLOT_RESTORE_POINT)cnt=1;
        if(devicematch&&!devicematch->num_matches&&!lastmatch&&lastitembar&&lastitembar->index>=SLOT_RESTORE_POINT)cnt=1;

        itembar_setpos(itembar,&pos,&cnt);
        if(itembar->isactive)
        {
            lastitembar=itembar;
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
    i=getscrollpos();
    if(offset_target&&i!=offset_target)
    {
        if(i<offset_target)i+=4;
        //log_con("Y:%d/%d\n",i,offset_target);
        setscrollpos(i);
        if(i>offset_target)offset_target=0;
        chg=1;
    }
    return chg||
        (installmode==MODE_NONE&&manager->items_list[SLOT_EXTRACTING].install_status);
}

int groupsize(manager_t *manager,int index)
{
    itembar_t *itembar;
    int i;
    int num=0;

    itembar=manager->items_list;
    for(i=0;i<manager->items_handle.items;i++,itembar++)
        if(itembar->index==index&&itembar->hwidmatch&&(itembar->hwidmatch->status&STATUS_INVALID)==0&&(itembar->first&2)==0)
            num++;

    return num;
}


void drawbutton(HDC hdc,int x,int pos,int index,WCHAR *str1,WCHAR *str2)
{
    pos+=D(ITEM_TEXT_OFS_Y);
    SetTextColor(hdc,D(boxindex[box_status(index)]+14));
    TextOut(hdc,x+D(ITEM_TEXT_OFS_X),pos,str1,wcslen(str1));
    SetTextColor(hdc,D(boxindex[box_status(index)]+15));
    TextOut(hdc,x+D(ITEM_TEXT_OFS_X),pos+D(ITEM_TEXT_DIST_Y),str2,wcslen(str2));
}

int  manager_drawitem(manager_t *manager,HDC hdc,int index,int ofsy,int zone,int cutoff)
{
    HICON hIcon;
    WCHAR bufw[BUFLEN];
    HRGN hrgn=0,hrgn2;
    int x=Xg(D(DRVITEM_OFSX));
    int wx=XG(D(DRVITEM_WX),x);
    int r=D(box[box_status(index)].index+3);
    int intend=0;
    int oldstyle=flags&FLAG_SHOWDRPNAMES1||flags&FLAG_OLDSTYLE;

    itembar_t *itembar=&manager->items_list[index];
    int pos=(itembar->curpos>>16)-D(DRVITEM_DIST_Y0);
    if(index>=SLOT_RESTORE_POINT)pos-=ofsy;

    if(!(itembar->first&1))
    {
        int i=index;

        while(i>=0&&!(manager->items_list[i].first&1&&manager->items_list[i].isactive))i--;
        if(manager->items_list[i].index==itembar->index)intend=i;
        //itembar->index=intend;
    }
    if(intend)
    {
        x+=D(DRVITEM_LINE_INTEND);
        wx-=D(DRVITEM_LINE_INTEND);
    }
    if(pos<=-D(DRVITEM_DIST_Y0))return 0;
    if(pos>mainy_c)return 0;
    if(wx<0)return 0;

    SelectObject(hdc,hFont);

    if(index<SLOT_RESTORE_POINT)cutoff=D(DRVITEM_OFSY);
    hrgn2=CreateRectRgn(0,cutoff,x+wx,mainy_c);
    hrgn=CreateRoundRectRgn(x,(pos<cutoff)?cutoff:pos,x+wx,pos+D(DRVITEM_WY),r,r);
    int cl=((zone>=0)?1:0);
    if(index==SLOT_EXTRACTING&&itembar->install_status&&installmode==MODE_NONE)
        cl=((GetTickCount()-manager->animstart)/200)%2;
    SelectClipRgn(hdc,hrgn2);
    if(intend&&D(DRVITEM_LINE_WIDTH)&&!(itembar->first&2))
    {
        HPEN oldpen,newpen;

        newpen=CreatePen(PS_SOLID,D(DRVITEM_LINE_WIDTH),D(DRVITEM_LINE_COLOR));
        oldpen=SelectObject(hdc,newpen);
        MoveToEx(hdc,x-D(DRVITEM_LINE_INTEND)/2,(manager->items_list[intend].curpos>>16)-D(DRVITEM_DIST_Y0)+D(DRVITEM_WY)-ofsy,0);
        LineTo(hdc,x-D(DRVITEM_LINE_INTEND)/2,pos+D(DRVITEM_WY)/2);
        LineTo(hdc,x,pos+D(DRVITEM_WY)/2);
        SelectObject(hdc,oldpen);
        DeleteObject(newpen);
    }
    box_draw(hdc,x,pos,x+wx,pos+D(DRVITEM_WY),box_status(index)+cl);
    SelectClipRgn(hdc,hrgn);

    if(itembar->percent)
    {
        //printf("%d\n",itembar->percent);
        int a=BOX_PROGR;
        //if(index==SLOT_EXTRACTING&&installmode==MODE_STOPPING)a=BOX_PROGR_S;
        //if(index>=RES_SLOTS&&(!itembar->checked||installmode==MODE_STOPPING))a=BOX_PROGR_S;
        box_draw(hdc,x,pos,x+wx*itembar->percent/1000.,pos+D(DRVITEM_WY),a);
    }

    SetTextColor(hdc,0); // todo: color
    switch(index)
    {
        case SLOT_RESTORE_POINT:
            drawcheckbox(hdc,x+D(ITEM_CHECKBOX_OFS_X),pos+D(ITEM_CHECKBOX_OFS_Y),
                         D(ITEM_CHECKBOX_SIZE),D(ITEM_CHECKBOX_SIZE),
                         itembar->checked,zone>=0);

            wcscpy(bufw,STR(itembar->install_status));
            TextOut(hdc,x+D(ITEM_TEXT_OFS_X),pos+D(ITEM_TEXT_DIST_Y)/2,bufw,wcslen(bufw));
            break;

        case SLOT_INDEXING:
            wsprintf(bufw,L"%s (%d%s%d)",STR(itembar->isactive==2?STR_INDEXLZMA:STR_INDEXING),
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
                if(installmode==MODE_INSTALLING)
                {
                wsprintf(bufw,L"%s (%d%s%d)",STR(itembar->install_status),
                        manager->items_list[SLOT_EXTRACTING].val1+1,STR(STR_OF),
                        manager->items_list[SLOT_EXTRACTING].val2);

                }
                else
                    if(itembar->install_status)wsprintf(bufw,STR(itembar->install_status),itembar->percent);

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
                wsprintf(bufw,L"%s",STR(itembar->install_status));
                SetTextColor(hdc,D(boxindex[box_status(index)]+14));
                TextOut(hdc,x+D(ITEM_TEXT_OFS_X),pos,bufw,wcslen(bufw));
                wsprintf(bufw,L"%s",STR(STR_INST_CLOSE));
                SetTextColor(hdc,D(boxindex[box_status(index)]+15));
                TextOut(hdc,x+D(ITEM_TEXT_OFS_X),pos+D(ITEM_TEXT_DIST_Y),bufw,wcslen(bufw));
            }
            break;

        case SLOT_NODRIVERS:
            drawbutton(hdc,x,pos,index,STR(STR_EMPTYDRP),manager->matcher->col->driverpack_dir);
            break;

        case SLOT_NOUPDATES:
            pos+=D(ITEM_TEXT_OFS_Y);
            wsprintf(bufw,L"%s",STR(manager->items_handle.items>RES_SLOTS?STR_NOUPDATES:STR_INITIALIZING));
            SetTextColor(hdc,D(boxindex[box_status(index)]+14));
            TextOut(hdc,x+D(ITEM_TEXT_OFS_X),pos+D(ITEM_TEXT_DIST_Y)/2,bufw,wcslen(bufw));
            break;

        case SLOT_DOWNLOAD:
            if(itembar->val1>>8)
                wsprintf(bufw,STR(itembar->val1&0xFF?STR_UPD_AVAIL3:STR_UPD_AVAIL1),itembar->val1>>8,itembar->val1&0xFF);
            else
                wsprintf(bufw,STR(STR_UPD_AVAIL2),itembar->val1&0xFF);

#ifdef USE_TORRENT
            if(!torrentstatus.sessionpaused)
            {
                WCHAR num1[64],num2[64];

                format_size(num1,torrentstatus.downloaded,0);
                format_size(num2,torrentstatus.downloadsize,0);

                wsprintf(bufw,STR(STR_UPD_PROGRES),
                         num1,
                         num2,
                         (torrentstatus.downloadsize)?torrentstatus.downloaded*100/torrentstatus.downloadsize:0);

                drawbutton(hdc,x,pos,index,bufw,STR(STR_UPD_MODIFY));
            }
            else
#endif
                drawbutton(hdc,x,pos,index,bufw,STR(STR_UPD_START));

            break;

        case SLOT_SNAPSHOT:
            drawbutton(hdc,x,pos,index,state_file,STR(STR_CLOSE_SNAPSHOT));
            break;

        case SLOT_DPRDIR:
            drawbutton(hdc,x,pos,index,drpext_dir,STR(STR_CLOSE_DRPEXT));
            break;

        case SLOT_VIRUS_AUTORUN:
            drawbutton(hdc,x,pos,index,STR(STR_VIRUS),STR(STR_VIRUS_AUTORUN));
            break;

        case SLOT_VIRUS_RECYCLER:
            drawbutton(hdc,x,pos,index,STR(STR_VIRUS),STR(STR_VIRUS_RECYCLER));
            break;

        case SLOT_VIRUS_HIDDEN:
            drawbutton(hdc,x,pos,index,STR(STR_VIRUS),STR(STR_VIRUS_HIDDEN));
            break;

        default:
            if(itembar->first&2)
            {
                    /*wsprintf(bufw,L"%ws",manager->matcher->state->text+itembar->devicematch->device->Devicedesc);
                    SetTextColor(hdc,D(boxindex[box_status(index)]+14));
                    TextOut(hdc,x+D(ITEM_TEXT_OFS_X),pos,bufw,wcslen(bufw));*/

                    //str_status(bufw,itembar);
                    wsprintf(bufw,L"%ws",getdrp_packname(itembar->hwidmatch));
                    SetTextColor(hdc,D(boxindex[box_status(index)]+15));
                    TextOut(hdc,x+D(ITEM_CHECKBOX_OFS_X),pos+D(ITEM_TEXT_DIST_Y)+5,bufw,wcslen(bufw));
                    break;
            }
            if(itembar->hwidmatch)
            {
                // Checkbox
                drawcheckbox(hdc,x+D(ITEM_CHECKBOX_OFS_X),pos+D(ITEM_CHECKBOX_OFS_Y),
                         D(ITEM_CHECKBOX_SIZE),D(ITEM_CHECKBOX_SIZE),
                         itembar->checked,zone>=0);

                // Available driver desc
                pos+=D(ITEM_TEXT_OFS_Y);
                wsprintf(bufw,L"%S",getdrp_drvdesc(itembar->hwidmatch));
                SetTextColor(hdc,D(boxindex[box_status(index)]+14));
                RECT rect;
                int wx1=wx-D(ITEM_TEXT_OFS_X)-D(ITEM_ICON_OFS_X);
                rect.left=x+D(ITEM_TEXT_OFS_X);
                rect.top=pos;
                if(intend)wx1-=D(DRVITEM_LINE_INTEND);
                rect.right=rect.left+wx1/2;
                rect.bottom=rect.top+90;
                if(oldstyle)
                    TextOut(hdc,x+D(ITEM_TEXT_OFS_X),pos,bufw,wcslen(bufw));
                else
                    DrawText(hdc,bufw,-1,&rect,DT_WORDBREAK);


                // Available driver status
                SetTextColor(hdc,D(boxindex[box_status(index)]+15));
                str_status(bufw,itembar);
                switch(itembar->install_status)
                {
                    case STR_INST_FAILED:
                    case STR_EXTR_FAILED:
                        wsprintf(bufw,L"%s %X",STR(itembar->install_status),itembar->val1);
                        break;

                    case STR_INST_EXTRACT:
                        wsprintf(bufw,STR(STR_INST_EXTRACT),(itembar->percent+100)/10);
                        break;

                    case STR_EXTR_EXTRACTING:
                        wsprintf(bufw,L"%s %d%%",STR(STR_EXTR_EXTRACTING),itembar->percent/10);
                        break;

                    case 0:
                        break;

                    default:
                        wcscpy(bufw,STR(itembar->install_status));
                }
                rect.left=x+D(ITEM_TEXT_OFS_X)+wx1/2;
                rect.top=pos;
                rect.right=rect.left+wx1/2;
                rect.bottom=rect.top+90;
                if(oldstyle)
                    TextOut(hdc,x+D(ITEM_TEXT_OFS_X),pos+D(ITEM_TEXT_DIST_Y),bufw,wcslen(bufw));
                else
                    DrawText(hdc,bufw,-1,&rect,DT_WORDBREAK);

                if(flags&FLAG_SHOWDRPNAMES1)
                {
                    int len=wcslen(manager->matcher->col->driverpack_dir);
                    int lnn=len-wcslen(getdrp_packpath(itembar->hwidmatch));

                    SetTextColor(hdc,0);// todo: color
                    wsprintf(bufw,L"%ws%ws%ws",
                            getdrp_packpath(itembar->hwidmatch)+len+(lnn?1:0),
                            lnn?L"\\":L"",
                            getdrp_packname(itembar->hwidmatch));
                    TextOut(hdc,x+wx-240,pos+D(ITEM_TEXT_DIST_Y),bufw,wcslen(bufw));
                }
            }
            else
            {
                // Device desc
                if(itembar->devicematch)
                {
                    wsprintf(bufw,L"%ws",manager->matcher->state->text+itembar->devicematch->device->Devicedesc);
                    SetTextColor(hdc,D(boxindex[box_status(index)]+14));
                    RECT rect;
                    int wx1=wx-D(ITEM_TEXT_OFS_X)-D(ITEM_ICON_OFS_X);
                    rect.left=x+D(ITEM_TEXT_OFS_X);
                    rect.top=pos;
                    rect.right=rect.left+wx1/2;
                    rect.bottom=rect.top+90;
                    if(oldstyle)
                        TextOut(hdc,x+D(ITEM_TEXT_OFS_X),pos,bufw,wcslen(bufw));
                    else
                        DrawText(hdc,bufw,-1,&rect,DT_WORDBREAK);

                    str_status(bufw,itembar);
                    SetTextColor(hdc,D(boxindex[box_status(index)]+15));
                    rect.left=x+D(ITEM_TEXT_OFS_X)+wx1/2;
                    rect.top=pos;
                    rect.right=rect.left+wx1/2;
                    rect.bottom=rect.top+90;
                    if(oldstyle)
                        TextOut(hdc,x+D(ITEM_TEXT_OFS_X),pos+D(ITEM_TEXT_DIST_Y),bufw,wcslen(bufw));
                    else
                        DrawText(hdc,bufw,-1,&rect,DT_WORDBREAK);
                }
            }
            // Device icon
            if(itembar->devicematch&&SetupDiLoadClassIcon(&itembar->devicematch->device->DeviceInfoData.ClassGuid,&hIcon,0))
            {
                DrawIconEx(hdc,x+D(ITEM_ICON_OFS_X),pos+D(ITEM_ICON_OFS_Y),hIcon,D(ITEM_ICON_SIZE),D(ITEM_ICON_SIZE),0,0,DI_NORMAL);
                DestroyIcon(hIcon);
            }

            // Expand icon
            if(groupsize(manager,itembar->index)>1&&itembar->first&1)
            {
                int xo=x+wx-D(ITEM_ICON_SIZE)*2+10;
                image_draw(hdc,&icon[(itembar->isactive&2?0:2)+(zone==2?1:0)],xo,pos,xo+32,pos+32,0,HSTR|VSTR);
            }
            break;

    }

    SelectClipRgn(hdc,0);
    DeleteObject(hrgn);
    DeleteObject(hrgn2);
    return 1;
}

int isbehind(manager_t *manager,int pos,int ofsy,int j)
{
    itembar_t *itembar;

    if(j<SLOT_RESTORE_POINT)return 0;
    if(pos-ofsy<=-D(DRVITEM_DIST_Y0))return 1;
    if(pos-ofsy>mainy_c)return 1;

    itembar=&manager->items_list[j-1];
    if((itembar->curpos>>16)==pos)return 1;

    return 0;
}

int calc_cutoff(manager_t *manager)
{
    int i,cutoff=0;

    for(i=0;i<SLOT_RESTORE_POINT;i++)
        if(manager->items_list[i].isactive)cutoff=(manager->items_list[i].curpos>>16);

    return cutoff;
}

void manager_draw(manager_t *manager,HDC hdc,int ofsy)
{
    itembar_t *itembar;
    int i;
    int maxpos=0;
    int nm=0;
    int cur_i,zone;
    int cutoff=0;
    POINT p;
    RECT rect;

    GetCursorPos(&p);
    ScreenToClient(hField,&p);
    manager_hitscan(manager,p.x,p.y,&cur_i,&zone);

    GetClientRect(hField,&rect);
    box_draw(hdc,0,0,rect.right,rect.bottom,BOX_DRVLIST);

    cutoff=calc_cutoff(manager);
    updatecur();
    updateoverall(manager);
    for(i=manager->items_handle.items-1;i>=0;i--)
    {
        itembar=&manager->items_list[i];
        if(itembar->isactive)continue;

        if(isbehind(manager,(itembar->curpos>>16),ofsy,i))continue;
        nm+=manager_drawitem(manager,hdc,i,ofsy,-1,cutoff);
    }
    for(i=manager->items_handle.items-1;i>=0;i--)
    {
        itembar=&manager->items_list[i];
        if(itembar->isactive==0)continue;

        if(itembar->curpos>maxpos)maxpos=itembar->curpos;
        nm+=manager_drawitem(manager,hdc,i,ofsy,cur_i==i?zone:-1,cutoff);

    }
    //printf("nm:%3d, ofs:%d\n",nm,ofsy);
    setscrollrange((maxpos>>16)+20);
}

int itembar_cmp(itembar_t *a,itembar_t *b,CHAR *ta,CHAR *tb)
{
    if(a->hwidmatch&&b->hwidmatch)
    {
        if(a->hwidmatch->HWID_index==b->hwidmatch->HWID_index)return 3;
        return 0;
    }
    if(wcslen((WCHAR*)(ta+a->devicematch->device->Driver))>0)
    {
        if(!wcscmp((WCHAR*)(ta+a->devicematch->device->Driver),(WCHAR*)(tb+b->devicematch->device->Driver)))return wcslen((WCHAR*)(ta+a->devicematch->device->Driver))+10;
    }
    else
    {
        if(wcslen((WCHAR*)(ta+a->devicematch->device->Devicedesc))>0)
        {
            if(!wcscmp((WCHAR*)(ta+a->devicematch->device->Devicedesc),(WCHAR*)(tb+b->devicematch->device->Devicedesc)))return 100+wcslen((WCHAR*)(ta+a->devicematch->device->Devicedesc));
        }
    }

    return 0;
}

//Keep when:
//* installing drivers
//* device update
//Discard when:
//* loading a snapshot
//* returning to real machine
//* driverpack update
void manager_restorepos(manager_t *manager_new,manager_t *manager_old)
{
    itembar_t *itembar_new,*itembar_old;
    CHAR *t_new,*t_old;
    int i,j;
    int show_changes=manager_old->items_handle.items>20;

    //if(statemode==STATEMODE_LOAD)show_changes=0;
    if((log_verbose&LOG_VERBOSE_DEVSYNC)==0)show_changes=0;
    //show_changes=1;

    t_old=manager_old->matcher->state->text;
    t_new=manager_new->matcher->state->text;

    if(manager_old->items_list[SLOT_EMPTY].curpos==1)
    {
        return;
    }
    if((instflag&RESTOREPOS)==0)
    {
        instflag^=RESTOREPOS;
        return;
    }

    log_con("{Updated %d->%d\n",manager_old->items_handle.items,manager_new->items_handle.items);
    log_console=0;
    itembar_new=&manager_new->items_list[RES_SLOTS];
    for(i=RES_SLOTS;i<manager_new->items_handle.items;i++,itembar_new++)
    {
        itembar_old=&manager_old->items_list[RES_SLOTS];

        if(itembar_act&&itembar_cmp(itembar_new,&manager_old->items_list[itembar_act],t_new,t_old))
        {
            log_con("Act %d -> %d\n",itembar_act,i);
            itembar_act=i;
        }

        for(j=RES_SLOTS;j<manager_old->items_handle.items;j++,itembar_old++)
        {
            if(itembar_old->isactive!=9)
            {
                if(itembar_cmp(itembar_new,itembar_old,t_new,t_old))
                {
                    wcscpy(itembar_new->txt1,itembar_old->txt1);
                    itembar_new->install_status=itembar_old->install_status;
                    itembar_new->val1=itembar_old->val1;
                    itembar_new->val2=itembar_old->val2;
                    itembar_new->percent=itembar_old->percent;

                    itembar_new->isactive=itembar_old->isactive;
                    itembar_new->checked=itembar_old->checked;

                    itembar_new->oldpos=itembar_old->oldpos;
                    itembar_new->curpos=itembar_old->curpos;
                    itembar_new->tagpos=itembar_old->tagpos;
                    itembar_new->accel=itembar_old->accel;

                    itembar_old->isactive=9;
                    break;
                }
            }
        }
        if(show_changes)
        if(j==manager_old->items_handle.items)
        {
            log_con("\nAdded   $%04d|%ws|%ws|",i,t_new+itembar_new->devicematch->device->Driver,
                    t_new+itembar_new->devicematch->device->Devicedesc);

            if(itembar_new->hwidmatch)
            {
                int limits[7];
                memset(limits,0,sizeof(limits));
                log_con("%d|\n",itembar_new->hwidmatch->HWID_index);
                hwidmatch_print_tbl(itembar_new->hwidmatch,limits);
            }
            else
                device_print(itembar_new->devicematch->device,manager_new->matcher->state);
        }
    }

    itembar_old=&manager_old->items_list[RES_SLOTS];
    if(show_changes)
    for(j=RES_SLOTS;j<manager_old->items_handle.items;j++,itembar_old++)
    {
        if(itembar_old->isactive!=9)
        {
            log_con("\nDeleted $%04d|%ws|%ws|",j,t_old+itembar_old->devicematch->device->Driver,
                    t_old+itembar_old->devicematch->device->Devicedesc);
            if(itembar_old->hwidmatch)
            {
                int limits[7];
                memset(limits,0,sizeof(limits));
                log_con("%d|\n",itembar_old->hwidmatch->HWID_index);
                hwidmatch_print_tbl(itembar_old->hwidmatch,limits);
            }
            else
                device_print(itembar_old->devicematch->device,manager_old->matcher->state);

        }
    }
    log_console=0;
    log_con("}Updated\n");
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
    //SetTextColor(hdcMem,0);
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
    TextOut_CM(td->hdcMem,td->x+POPUP_SYSINFO_OFS,td->y,buffer,td->col,&td->maxsz,1);
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

void popup_driverline(hwidmatch_t *hwidmatch,int *limits,HDC hdcMem,int y,int mode,int index)
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

    TextOutP(&td,L"$%04d",index);
    TextOutP(&td,L"| %d",hwidmatch->altsectscore);
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
    int lne=D(POPUP_WY);
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
            popup_driverline(itembar->hwidmatch,limits,hdcMem,td.y,0,k);


    TextOut_CM(hdcMem,10,td.y,STR(STR_HINT_INSTDRV),c0,&maxsz,1);td.y+=lne;

    if(cur_driver)
    {
        wsprintf(bufw,L"%s",t+cur_driver->MatchingDeviceId);
        for(k=0;bufw[k];k++)i_hwid[k]=toupper(bufw[k]);i_hwid[k]=0;
        str_date(&cur_driver->version,bufw);

        TextOutP(&td,L"$%04d",i);
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
        if(itembar->index==group&&itembar->hwidmatch&&(itembar->first&2)==0)
    {
        if(k==i)
        {
            SelectObject(hdcMem,GetStockObject(DC_BRUSH));
            SelectObject(hdcMem,GetStockObject(DC_PEN));
//            SetDCBrushColor(hdcMem,RGB(115,125,255));
            SetDCBrushColor(hdcMem,RGB(255,255,255));//todo: color
            Rectangle(hdcMem,D(POPUP_OFSX)+horiz_sh,td.y,rect.right+horiz_sh-D(POPUP_OFSX),td.y+lne);
        }
        popup_driverline(itembar->hwidmatch,limits,hdcMem,td.y,1,k);
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

void popup_drivercmp(manager_t *manager,HDC hdcMem,RECT rect,int index)
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

    if(index<RES_SLOTS)return;

    devicematch_f=manager->items_list[index].devicematch;
    hwidmatch_f=manager->items_list[index].hwidmatch;

    td.y=D(POPUP_OFSY);
    td.wy=D(POPUP_WY);
    td.hdcMem=hdcMem;
    td.maxsz=0;

    if(devicematch_f->device->driver_index>=0)
    {
        int i;
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
    TextOutF(&td,c0,L"$%04d",index);
    if(hwidmatch_f)
    {
        TextOutF(&td,isvalidcat(hwidmatch_f,manager->matcher->state)?cb:D(POPUP_CMP_INVALID_COLOR),
                 L"%s(%d)%S",STR(STR_HINT_SIGNATURE),pickcat(hwidmatch_f,manager->matcher->state),getdrp_drvcat(hwidmatch_f,pickcat(hwidmatch_f,manager->matcher->state)));

        td.x=p0;TextOutF(&td,c0,L"%s",STR(STR_HINT_DRP));td.x=p1;
        TextOutF(&td,c0,L"%s\\%s",getdrp_packpath(hwidmatch_f),getdrp_packname(hwidmatch_f));
        TextOutF(&td,calc_notebook(hwidmatch_f)?c0:D(POPUP_CMP_INVALID_COLOR)
                 ,L"%S%S",getdrp_infpath(hwidmatch_f),getdrp_infname(hwidmatch_f));
    }

    SetupDiGetClassDescription(&devicematch_f->device->DeviceInfoData.ClassGuid,bufw,BUFLEN,0);

    td.x=p0;TextOutF(&td,c0,L"%s",STR(STR_HINT_DEVICE));td.x=p1;
    TextOutF(&td,c0,L"%s",t+devicematch_f->device->Devicedesc);
    TextOutF(&td,c0,L"%s%s",STR(STR_HINT_MANUF),t+devicematch_f->device->Mfg);
    TextOutF(&td,c0,L"%s",bufw);
    TextOutF(&td,c0,L"%s",t+devicematch_f->device->Driver);
    wsprintf(bufw,STR(STR_STATUS_NOTPRESENT+print_status(devicematch_f->device)),devicematch_f->device->problem);
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
    rect.bottom=500;
    DrawText(hdcMem,STR(STR_ABOUT_LICENSE),-1,&rect,DT_WORDBREAK);
    td.y+=td.wy*3;
    TextOutF(&td,td.col,L"%s%s",STR(STR_ABOUT_DEV_TITLE),STR(STR_ABOUT_DEV_LIST));
    TextOutF(&td,td.col,L"%s%s",STR(STR_ABOUT_TESTERS_TITLE),STR(STR_ABOUT_TESTERS_LIST));
    td.y+=td.wy*(intptr_t)STR(STR_ABOUT_SIZE);

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
    TextOutSF(&td,STR(STR_SYSINF_PRODUCT),L"%s",state_getproduct(state));
    TextOutSF(&td,STR(STR_SYSINF_MODEL),L"%s",state_getmodel(state));
    TextOutSF(&td,STR(STR_SYSINF_MANUF),L"%s",state_getmanuf(state));
    TextOutSF(&td,STR(STR_SYSINF_TYPE),L"%s[%d]",isLaptop?STR(STR_SYSINF_LAPTOP):STR(STR_SYSINF_DESKTOP),state->ChassisType);

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
        TextOutSF(&td,STR(STR_SYSINF_CHARGED),L"%d%%",battery->BatteryLifePercent);
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
        td.maxsz+=POPUP_SYSINFO_OFS;
        TextOutF(&td,td.col,L"%d%s x %d%s (%.1f %s) %.3f %s",
                  x,STR(STR_SYSINF_CM),
                  y,STR(STR_SYSINF_CM),
                  sqrt(x*x+y*y)/2.54,STR(STR_SYSINF_INCH),
                  (double)y/x,
                  iswide(x,y)?STR(STR_SYSINF_WIDE):L"");

        td.maxsz-=POPUP_SYSINFO_OFS;
    }

    td.x=p0;
    td.maxsz+=POPUP_SYSINFO_OFS;
    td.y+=td.wy;
    TextOutF(&td,D(POPUP_CMP_BETTER_COLOR),STR(STR_SYSINF_MISC));td.x=p1;
    td.maxsz-=POPUP_SYSINFO_OFS;
    popup_resize((td.maxsz+POPUP_SYSINFO_OFS+p0+p1),td.y+D(POPUP_OFSY));
}

void format_size(WCHAR *buf,long long val,int isspeed)
{
#ifdef USE_TORRENT
    if(val<(1<<10))swprintf(buf,L"%d %s",    (int)val,STR(STR_UPD_BYTES));else
    if(val<(1<<20))swprintf(buf,L"%.03f %s",(double)val/(1<<10),STR(STR_UPD_BYTES+1));else
    if(val<(1<<30))swprintf(buf,L"%.03f %s",(double)val/(1<<20),STR(STR_UPD_BYTES+2));else
    if(val<((long long)1<<40))swprintf(buf,L"%.03f %s",(double)val/(1<<30),STR(STR_UPD_BYTES+3));
#else
    buf[0]=0;
    UNREFERENCED_PARAMETER(val)
#endif
    if(isspeed)wcscat(buf,STR(STR_UPD_SEC));
}

void format_time(WCHAR *buf,long long val)
{
    long long days,hours,mins,secs;

    secs=val/1000;
    mins=secs/60;
    hours=mins/60;
    days=hours/24;

    secs%=60;
    mins%=60;
    hours%=24;

    wcscpy(buf,L"\x221E");
    if(secs) wsprintf(buf,L"%d %s",(int)secs,STR(STR_UPD_TSEC));
    if(mins) wsprintf(buf,L"%d %s %d %s",(int)mins,STR(STR_UPD_TMIN),(int)secs,STR(STR_UPD_TSEC));
    if(hours)wsprintf(buf,L"%d %s %d %s",(int)hours,STR(STR_UPD_THOUR),(int)mins,STR(STR_UPD_TMIN));
    if(days) wsprintf(buf,L"%d %s %d %s",(int)days,STR(STR_UPD_TDAY),(int)hours,STR(STR_UPD_THOUR));
}

void popup_download(HDC hdcMem)
{
#ifdef USE_TORRENT
    textdata_t td;
    torrent_status_t t;
    int p0=D(POPUP_OFSX),p1=D(POPUP_OFSX)+10;
    int per=0;
    WCHAR num1[BUFLEN],num2[BUFLEN];


    td.col=D(POPUP_TEXT_COLOR);
    td.y=D(POPUP_OFSY);
    td.wy=D(POPUP_WY);
    td.hdcMem=hdcMem;
    td.maxsz=0;
    td.x=p0;

    //update_getstatus(&t);
    t=torrentstatus;

    format_size(num1,t.downloaded,0);
    format_size(num2,t.downloadsize,0);
    if(t.downloadsize)per=t.downloaded*100/t.downloadsize;
    TextOutSF(&td,STR(STR_DWN_DOWNLOADED),STR(STR_DWN_DOWNLOADED_F),num1,num2,per);
    format_size(num1,t.uploaded,0);
    TextOutSF(&td,STR(STR_DWN_UPLOADED),num1);
    format_time(num1,t.elapsed);
    TextOutSF(&td,STR(STR_DWN_ELAPSED),num1);
    format_time(num1,t.remaining);
    TextOutSF(&td,STR(STR_DWN_REMAINING),num1);

    td.y+=td.wy;
    if(t.status)
        TextOutSF(&td,STR(STR_DWN_STATUS),L"%s",t.status);
    if(*t.error)
    {
        td.col=D(POPUP_CMP_INVALID_COLOR);
        TextOutSF(&td,STR(STR_DWN_ERROR),L"%s",t.error);
        td.col=D(POPUP_TEXT_COLOR);
    }
    format_size(num1,t.downloadspeed,1);
    TextOutSF(&td,STR(STR_DWN_DOWNLOADSPEED),num1);
    format_size(num1,t.uploadspeed,1);
    TextOutSF(&td,STR(STR_DWN_UPLOADSPEED),num1);

    td.y+=td.wy;
    TextOutSF(&td,STR(STR_DWN_SEEDS),STR(STR_DWN_SEEDS_F),t.seedsconnected,t.seedstotal);
    TextOutSF(&td,STR(STR_DWN_PEERS),STR(STR_DWN_SEEDS_F),t.peersconnected,t.peerstotal);
    format_size(num1,t.wasted,0);
    format_size(num2,t.wastedhashfailes,0);
    TextOutSF(&td,STR(STR_DWN_WASTED),STR(STR_DWN_WASTED_F),num1,num2);

//    TextOutSF(&td,L"Paused",L"%d,%d",t.sessionpaused,t.torrentpaused);
    popup_resize((td.maxsz+POPUP_SYSINFO_OFS+p0+p1),td.y+D(POPUP_OFSY));
#else
    UNREFERENCED_PARAMETER(hdcMem)
#endif
}
//}
