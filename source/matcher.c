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
#include "device.h"

//{ Global variables

/*
Invalid:
    me          Windows ME
    ntx64       ntAMD64
    ntai64      ntIA64
    ntarm       ARM
    ntx64.6.0   ntAMD64
    nt.7        future
*/
const char *nts[NUM_DECS]=
{
    "nt.5",  "ntx86.5",  "ntamd64.5"  ,"ntia64.5",   // 2000
    "nt.5.0","ntx86.5.0","ntamd64.5.0","ntia64.5.0", // 2000
    "nt.5.1","ntx86.5.1","ntamd64.5.1","ntia64.5.1", // XP
    "nt.5.2","ntx86.5.2","ntamd64.5.2","ntia64.5.2", // Server 2003
    "nt.6",  "ntx86.6",  "ntamd64.6"  ,"ntia64.6",   // Vista
    "nt.6.0","ntx86.6.0","ntamd64.6.0","ntia64.6.0", // Vista
    "nt.6.1","ntx86.6.1","ntamd64.6.1","ntia64.6.1", // 7
    "nt.6.2","ntx86.6.2","ntamd64.6.2","ntia64.6.2", // 8
    "nt.6.3","ntx86.6.3","ntamd64.6.3","ntia64.6.3", // 9
    "nt",    "ntx86",    "ntamd64",    "ntia64",
    "nt..",  "ntx86..",  "ntamd64..",  "ntia64..",
};

const int nts_version[NUM_DECS]=
{
    50,    50,    50,    50, // 2000
    50,    50,    50,    50, // 2000
    51,    51,    51,    51, // XP
    52,    52,    52,    52, // Server 2003
    60,    60,    60,    60, // Vista
    60,    60,    60,    60, // Vista
    61,    61,    61,    61, // 7
    62,    62,    62,    62, // 8
    63,    63,    63,    63, // 9
     0,     0,     0,     0,
     0,     0,     0,     0,
};

const int nts_arch[NUM_DECS]=
{
    0,  1,  2,  3, // 2000
    0,  1,  2,  3, // 2000
    0,  1,  2,  3, // XP
    0,  1,  2,  3, // Serve
    0,  1,  2,  3, // Vista
    0,  1,  2,  3, // Vista
    0,  1,  2,  3, // 7
    0,  1,  2,  3, // 8
    0,  1,  2,  3, // 9
    0,  1,  2,  3,
    0,  1,  2,  3,
};

const int nts_score[NUM_DECS]=
{
    50,   150,   150,   150, // 2000
    50,   150,   150,   150, // 2000
    51,   151,   151,   151, // XP
    52,   152,   152,   152, // Server 2003
    60,   160,   160,   160, // Vista
    60,   160,   160,   160, // Vista
    61,   161,   161,   161, // 7
    62,   162,   162,   162, // 8
    63,   163,   163,   163, // 9
    10,   100,   100,   100,
    10,   100,   100,   100,
};

const markers_t markers[NUM_MARKERS]=
{
    {"5x86",    5, 1, 0},
    {"6x86",    6, 0, 0},
    {"7x86",    6, 1, 0},
    {"8x86",    6, 2, 0},
    {"81x86",   6, 3, 0},

    {"5x64",    5, 2, 1},
    {"6x64",    6, 0, 1},
    {"7x64",    6, 1, 1},
    {"8x64",    6, 2, 1},
    {"81x64",   6, 3, 1},

    {"winall",  0, 0, 0},
    {"allxp",   5, 0,-1},
    {"allnt",   4, 0,-1},
    {"allx86", -1,-1, 0},
    {"allx64", -1,-1, 1},
    {"all8x86", 6, 2, 0},
    {"all8x64", 6, 2, 1},
    {"ntx86",  -1,-1, 0},
    {"ntx64",  -1,-1, 1},
    {"x86\\",  -1,-1, 0},
    {"x64\\",  -1,-1, 1},
};

char marker[BUFLEN];

WCHAR *Filter_1[]={L"Acer",L"acer",L"emachines",L"packard",L"bell",L"gateway",L"aspire",0};
WCHAR *Filter_2[]={L"Apple",L"apple",0};
WCHAR *Filter_3[]={L"Asus",L"asus",0};
WCHAR *Filter_4[]={L"OEM",L"clevo",L"eurocom",L"sager",L"iru",L"viewsonic",L"viewbook",0};
WCHAR *Filter_5[]={L"Dell",L"dell",L"alienware",L"arima",L"jetway",L"gericom",0};
WCHAR *Filter_6[]={L"Fujitsu",L"fujitsu",L"sieme",0};
WCHAR *Filter_7[]={L"OEM",L"ecs",L"elitegroup",L"roverbook",L"rover",L"shuttle",0};
WCHAR *Filter_8[]={L"HP",L"hp",L"hewle",L"compaq",0};
WCHAR *Filter_9[]={L"OEM",L"intel",L"wistron",0};
WCHAR *Filter_10[]={L"Lenovo",L"lenovo",L"compal",L"ibm",0};
WCHAR *Filter_11[]={L"LG",L"lg",0};
WCHAR *Filter_12[]={L"OEM",L"mitac",L"mtc",L"depo",L"getac",0};
WCHAR *Filter_13[]={L"MSI",L"msi",L"micro-star",0};
WCHAR *Filter_14[]={L"Panasonic",L"panasonic",L"matsushita",0};
WCHAR *Filter_15[]={L"OEM",L"quanta",L"prolink",L"nec",L"k-systems",L"benq",L"vizio",0};
WCHAR *Filter_16[]={L"OEM",L"pegatron",L"medion",0};
WCHAR *Filter_17[]={L"Samsung",L"samsung",0};
WCHAR *Filter_18[]={L"Gigabyte",L"gigabyte",0};
WCHAR *Filter_19[]={L"Sony",L"sony",L"vaio",0};
WCHAR *Filter_20[]={L"Toshiba",L"toshiba",0};
WCHAR *Filter_21[]={L"OEM",L"twinhead",L"durabook",0};

WCHAR **filter_list[]=
{
    Filter_1, Filter_2, Filter_3, Filter_4, Filter_5, Filter_6, Filter_7,
    Filter_8, Filter_9, Filter_10,Filter_11,Filter_12,Filter_13,Filter_14,
    Filter_15,Filter_16,Filter_17,Filter_18,Filter_19,Filter_20,Filter_21
};
//}

//{ Calc
void genmarker(state_t *state)
{
    int i,j;
    WCHAR *str;

    *marker=0;

    str=state_getmanuf(state);
    if(!str)return;

    //log_con("Manuf '%ws'\n",str);
    for(i=0;i<21;i++)
    {
        //log_con("%d: '%ws'\n",i+1,filter_list[i][0]);
        for(j=1;filter_list[i][j];j++)
        {
            if(StrStrI(str,filter_list[i][j]))
            {
                sprintf(marker,"%ws_nb",filter_list[i][0]);
                log_con("Matched marker: '%s'(%ws)\n",marker,filter_list[i][j]);
            }
        }
    }
    if(!*marker)sprintf(marker,"OEM_nb");
    log_con("Marker: '%s'\n",marker);
}

int calc_identifierscore(int dev_pos,int dev_ishw,int inf_pos)
{
    if(dev_ishw&&inf_pos==0)    // device hardware ID and a hardware ID in an INF
        return 0x0000+dev_pos;

    if(dev_ishw)                // device hardware ID and a compatible ID in an INF
        return 0x1000+dev_pos+0x100*inf_pos;

    if(inf_pos==0)              // device compatible ID and a hardware ID in an INF
        return 0x2000+dev_pos;

                                // device compatible ID and a compatible ID in an INF
        return 0x3000+dev_pos+0x100*inf_pos;
}

int calc_catalogfile(hwidmatch_t *hwidmatch)
{
    int r=0,i;

    for(i=CatalogFile;i<=CatalogFile_ntamd64;i++)
        if(*getdrp_drvfield(hwidmatch,i))r+=1<<i;

    //if(!isvalidcat(hwidmatch,state))r=0;
    //r=0;
    return r;
}

int calc_signature(int catalogfile,state_t *state,int isnt)
{
    if(state->architecture)
    {
        if(catalogfile&(1<<CatalogFile|1<<CatalogFile_nt|1<<CatalogFile_ntamd64|1<<CatalogFile_ntia64))
            return 0;
    }
    else
    {
        if(catalogfile&(1<<CatalogFile|1<<CatalogFile_nt|1<<CatalogFile_ntx86))
            return 0;
    }
    if(isnt)return 0x8000;
    return 0xC000;
}

unsigned calc_score(int catalogfile,int feature,int rank,state_t *state,int isnt)
{
    if(flags&FLAG_NOFEATURESCORE)feature=0;
    if(state->platform.dwMajorVersion>=6)
        return (calc_signature(catalogfile,state,isnt)<<16)+(feature<<16)+rank;
    else
        return calc_signature(catalogfile,state,isnt)+rank;
}

unsigned calc_score_h(driver_t *driver,state_t *state)
{
    int feature=driver->feature;
    if(flags&FLAG_NOFEATURESCORE)feature=0;
    return calc_score(driver->catalogfile,feature,driver->identifierscore,
        state,StrStrI((WCHAR *)(state->text+driver->InfSectionExt),L".nt")?1:0);
}

int calc_secttype(const char *s)
{
    char buf[4096];
    char *p=buf,*s1;
    int i;

    while(1)
    {
        s1=strchr(s,'.');
        if(s1)
        {
            s=s1;
            s++;
            if(!memcmp(s,"nt",2))break;
        }else break;

    }

    strcpy(buf,s);

    if((p=strchr(p,'.')))
        if((p=strchr(p+1,'.')))
            if((p=strchr(p+1,'.')))*p=0;
    for(i=0;i<NUM_DECS;i++)if(!strcmpi(buf,nts[i]))return i;
//log_file("'%s'\n",buf);
    return -1;
}

int calc_decorscore(int id,state_t *state)
{
    int major=state->platform.dwMajorVersion,
        minor=state->platform.dwMinorVersion,
        arch=state->architecture+1;

    if(id<0)
    {
        return 1;
    }
    if(nts_version[id]&&major*10+minor<nts_version[id])return 0;
    if(nts_arch[id]&&arch!=nts_arch[id])return 0;
    return nts_score[id];
}

int calc_markerscore(state_t *state,char *path)
{
    char buf[BUFLEN];
    int majver=state->platform.dwMajorVersion,
        minver=state->platform.dwMinorVersion,
        arch=state->architecture,
        curmaj=-1,curmin=-1,curarch=-1;
    int i;
    //int winall=0;
    int score=0;

    strcpy(buf,path);
    strtolower(buf,strlen(buf));

    for(i=0;i<NUM_MARKERS;i++)
    {
        if(strstr(buf,markers[i].name))
        {
            score=1;
            if(markers[i].major==0){/*winall=1;*/continue;}
            if(markers[i].major>curmaj)curmaj=markers[i].major;
            if(markers[i].minor>curmin)curmin=markers[i].minor;
            if(markers[i].arch>curarch)curarch=markers[i].arch;
        }
    }

    if(curmaj>=0&&curmin>=0&&majver>=curmaj&&minver>=curmin)score+=2+8;
    if(curmaj<0&&score)score+=2;
    if(curarch>=0&&curarch==arch)score+=4+16;
    if(curarch<0&&score)score+=4;
    return score;
}

intptr_t isvalid_usb30hub(hwidmatch_t *hwidmatch,state_t *state,WCHAR *str)
{
    //log_con("Intel USB3.0 HUB '%ws'\n",state->text+hwidmatch->devicematch->device->HardwareID);
    return (intptr_t)StrStrI((WCHAR *)(state->text+hwidmatch->devicematch->device->HardwareID),str);
}

int isvalid_ver(hwidmatch_t *hwidmatch,state_t *state)
{
    version_t *v;
    int major=state->platform.dwMajorVersion,
        minor=state->platform.dwMinorVersion;

    v=getdrp_drvversion(hwidmatch);
    switch(v->v1)
    {
        case 5:if(major!=5)return 0;break;              // XP
        case 6:if(major!=6||minor!=0)return 0;break;    // Vista
        case 7:if(major!=6||minor!=1)return 0;break;    // 7
        case 8:if(major!=6||minor <2)return 0;break;    // 8 and 8.1
        default:break;
    }
    return 1;
}

int calc_notebook(hwidmatch_t *hwidmatch)
{
    if(StrStrIA(getdrp_infpath(hwidmatch),"_nb\\")||
       StrStrIA(getdrp_infpath(hwidmatch),"Touchpad_Mouse\\"))
    {
        if(!isLaptop)return 0;
        if(!*marker)return 0;
        if(!StrStrIA(getdrp_infpath(hwidmatch),marker))return 0;
    }
    return 1;
}

int calc_altsectscore(hwidmatch_t *hwidmatch,state_t *state,int curscore)
{
    char buf[BUFLEN];
    int pos;
    int desc_index,manufacturer_index;
    driverpack_t *drp;

    drp=hwidmatch->drp;
    desc_index=drp->HWID_list[hwidmatch->HWID_index].desc_index;
    manufacturer_index=drp->desc_list[desc_index].manufacturer_index;

    for(pos=0;pos<hwidmatch->drp->manufacturer_list[manufacturer_index].sections_n;pos++)
    {
        getdrp_drvsectionAtPos(hwidmatch->drp,buf,pos,manufacturer_index);
        if(calc_decorscore(calc_secttype(buf),state)>curscore)return 0;
    }

    if(!calc_notebook(hwidmatch))return 0;

    if(StrStrIA(getdrp_infpath(hwidmatch),"intel_2nd\\"))
        if(!isvalid_usb30hub(hwidmatch,state,L"IUSB3\\ROOT_HUB30&VID_8086&PID_1E31"))return 0;

    if(StrStrIA(getdrp_infpath(hwidmatch),"intel_4th\\"))
        if(!isvalid_usb30hub(hwidmatch,state,L"IUSB3\\ROOT_HUB30&VID_8086&PID_8C31"))return 0;

    if(StrStrIA(getdrp_infpath(hwidmatch),"matchver\\")||
       StrStrIA(getdrp_infpath(hwidmatch),"L\\Realtek\\")||
       StrStrIA(getdrp_infpath(hwidmatch),"L\\R\\"))
        if(!isvalid_ver(hwidmatch,state))return 0;

    if(StrStrIA(getdrp_infpath(hwidmatch),"matchmarker\\"))
        if((calc_markerscore(state,getdrp_infpath(hwidmatch))&7)!=7)return 0;

    //log_file("Sc:%d\n\n",curscore);
    return isvalidcat(hwidmatch,state)?2:1;
}

int isMissing(device_t *device,driver_t *driver,state_t *state)
{
    if(device->problem==CM_PROB_DISABLED)return 0;
    if(driver)
    {
        if(!_wcsicmp((WCHAR*)(state->text+driver->MatchingDeviceId),L"PCI\\CC_0300"))return 1;
        if(device->problem&&device->HardwareID)return 1;
    }else
    {
        if(device->problem&&device->HardwareID)return 1;
    }
    return 0;
}

int calc_status(hwidmatch_t *hwidmatch,state_t *state)
{
    int r=0,res;
    int score;
    driver_t *cur_driver=hwidmatch->devicematch->driver;

    if(isMissing(hwidmatch->devicematch->device,cur_driver,state))return STATUS_MISSING;

    if(hwidmatch->devicematch->driver)
    {
        if(getdrp_drvversion(hwidmatch))
        {
            res=cmpdate(&hwidmatch->devicematch->driver->version,getdrp_drvversion(hwidmatch));
            if(res<0)r+=STATUS_NEW;else
            if(res>0)r+=STATUS_OLD;else
                r+=STATUS_CURRENT;
        }

        score=calc_score_h(cur_driver,state);
        res=cmpunsigned(score,hwidmatch->score);
        if(res>0)r+=STATUS_BETTER;else
        if(res<0)r+=STATUS_WORSE;else
            r+=STATUS_SAME;
    }
    else
        r+=STATUS_BETTER;

    if(!hwidmatch->altsectscore)r+=STATUS_INVALID;
    return r;
}
//}

//{ Misc
void findHWID_in_list(char *s,int list,int str,int *dev_pos)
{
    *dev_pos=0;
    WCHAR *p=(WCHAR *)(s+list);
    while(*p)
    {
        if(!wcsicmp(p,(WCHAR *)(s+str)))return;
        p+=lstrlen(p)+1;
        (*dev_pos)++;
    }
    *dev_pos=-1;
}

void getdd(device_t *cur_device,state_t *state,int *ishw,int *dev_pos)
{
    driver_t *cur_driver=&state->drivers_list[cur_device->driver_index];

    *ishw=1;
    findHWID_in_list(state->text,cur_device->HardwareID,cur_driver->MatchingDeviceId,dev_pos);
    if(*dev_pos<0&&cur_device->CompatibleIDs)
    {
        *ishw=0;
        findHWID_in_list(state->text,cur_device->CompatibleIDs,cur_driver->MatchingDeviceId,dev_pos);
    }
}

int cmpunsigned(unsigned a,unsigned b)
{
    if(a>b)return 1;
    if(a<b)return -1;
    return 0;
}

int cmpdate(version_t *t1,version_t *t2)
{
    int res;

    res=t1->y-t2->y;
    if(res)return res;

    res=t1->m-t2->m;
    if(res)return res;

    res=t1->d-t2->d;
    if(res)return res;

    return 0;
}

int cmpversion(version_t *t1,version_t *t2)
{
    int res;

    res=t1->v1-t2->v1;
    if(res)return res;

    res=t1->v2-t2->v2;
    if(res)return res;

    res=t1->v3-t2->v3;
    if(res)return res;

    res=t1->v4-t2->v4;
    if(res)return res;

    return 0;

}

void devicematch_init(devicematch_t *devicematch,device_t *cur_device,driver_t *driver,int items)
{
    devicematch->device=cur_device;
    devicematch->driver=driver;
    devicematch->start_matches=items;
    devicematch->num_matches=0;
}
//}

//{ hwidmatch
void hwidmatch_init(hwidmatch_t *hwidmatch,driverpack_t *drp,int HWID_index,int dev_pos,int ishw,state_t *state,devicematch_t *devicematch)
{
    char buf[4096];

    hwidmatch->drp=drp;
    hwidmatch->HWID_index=HWID_index;
    hwidmatch->devicematch=devicematch;

    getdrp_drvsection(hwidmatch,buf);

    hwidmatch->identifierscore=calc_identifierscore(dev_pos,ishw,drp->HWID_list[HWID_index].inf_pos);
    hwidmatch->decorscore=calc_decorscore(calc_secttype(buf),state);
    hwidmatch->markerscore=calc_markerscore(state,getdrp_infpath(hwidmatch));
    hwidmatch->altsectscore=calc_altsectscore(hwidmatch,state,hwidmatch->decorscore);
    hwidmatch->score=calc_score(calc_catalogfile(hwidmatch),getdrp_drvfeature(hwidmatch),
        hwidmatch->identifierscore,state,strstr(getdrp_drvinstallPicked(hwidmatch),".nt")?1:0);
    hwidmatch->status=calc_status(hwidmatch,state);
}

void hwidmatch_initbriefly(hwidmatch_t *hwidmatch,driverpack_t *drp,int HWID_index)
{
    hwidmatch->drp=drp;
    hwidmatch->HWID_index=HWID_index;
}
/*
0 section
1 driverpack
2 inffile
3 manuf
4 version
5 hwid
6 desc
*/

void minlen(CHAR *s,int *len)
{
    int l=strlen(s);
    if(*len<l)*len=l;
}

void hwidmatch_calclen(hwidmatch_t *hwidmatch,int *limits)
{
    char buf[4096];
    version_t *v;

    getdrp_drvsection(hwidmatch,buf);
    minlen(buf,&limits[0]);
    sprintf(buf,"%ws%ws",getdrp_packpath(hwidmatch),getdrp_packname(hwidmatch));
    minlen(buf,&limits[1]);
    sprintf(buf,"%s%s",getdrp_infpath(hwidmatch),getdrp_infname(hwidmatch));
    minlen(buf,&limits[2]);
    minlen(getdrp_drvmanufacturer(hwidmatch),&limits[3]);
    v=getdrp_drvversion(hwidmatch);
    sprintf(buf,"%d.%d.%d.%d",v->v1,v->v2,v->v3,v->v4);
    minlen(buf,&limits[4]);
    minlen(getdrp_drvHWID(hwidmatch),&limits[5]);
    minlen(getdrp_drvdesc(hwidmatch),&limits[6]);
}

void hwidmatch_print(hwidmatch_t *hwidmatch,int *limits)
{
    CHAR buf[4096];
    version_t *v;

    v=getdrp_drvversion(hwidmatch);
    log_file("  %d |",               hwidmatch->altsectscore);
    log_file(" %08X |",              hwidmatch->score);
    log_file(" %2d.%02d.%4d |",      v->d,v->m,v->y);
    log_file(" %3d |",               hwidmatch->decorscore);
    log_file(" %d |",                hwidmatch->markerscore);
    log_file(" %3X |",               hwidmatch->status);
                                getdrp_drvsection(hwidmatch,buf);
    log_file(" %-*s |",limits[0],buf);

    sprintf(buf,"%ws\\%ws",       getdrp_packpath(hwidmatch),getdrp_packname(hwidmatch));
    log_file(" %-*s |",limits[1],buf);
    log_file(" %8X% |",              getdrp_infcrc(hwidmatch));
    sprintf(buf,"%s%s",         getdrp_infpath(hwidmatch),getdrp_infname(hwidmatch));
    log_file(" %-*s |",limits[2],buf);
    log_file(" %-*s |",limits[3],    getdrp_drvmanufacturer(hwidmatch));
    sprintf(buf,"%d.%d.%d.%d",  v->v1,v->v2,v->v3,v->v4);
    log_file(" %*s |",limits[4],buf);
    log_file(" %-*s |",limits[5],    getdrp_drvHWID(hwidmatch));
    log_file(" %-*s",limits[6],      getdrp_drvdesc(hwidmatch));
    log_file("\n");
}

int hwidmatch_cmp(hwidmatch_t *match1,hwidmatch_t *match2)
{
    int res;

    res=match1->altsectscore-match2->altsectscore;
    if(res)return res;

    res=cmpunsigned(match1->score,match2->score);
    if(res)return -res;

    res=cmpdate(getdrp_drvversion(match1),getdrp_drvversion(match2));
    if(res)return res;

    res=match1->decorscore-match2->decorscore;
    if(res)return res;

    res=match1->markerscore-match2->markerscore;
    if(res)return res;

    res=(match1->status&~STATUS_DUP)-(match2->status&~STATUS_DUP);
    if(res)return res;

    return 0;
}
//}

//{ Matcher
void matcher_init(matcher_t *matcher,state_t *state,collection_t *col)
{
    matcher->state=state;
    matcher->col=col;

    heap_init(&matcher->devicematch_handle,ID_MATCHER,(void **)&matcher->devicematch_list,0,sizeof(devicematch_t));
    heap_init(&matcher->hwidmatch_handle,ID_MATCHER,(void **)&matcher->hwidmatch_list,0,sizeof(hwidmatch_t));
}

void matcher_free(matcher_t *matcher)
{
    heap_free(&matcher->devicematch_handle);
    heap_free(&matcher->hwidmatch_handle);
}

void matcher_findHWIDs(matcher_t *matcher,devicematch_t *devicematch,char *hwid,int dev_pos,int ishw)
{
    driverpack_t *drp;
    hwidmatch_t *hwidmatch;
    int i;
    int code;
    int sz;
    int isfound;

    sz=strlen(hwid);
    strtoupper(hwid,sz);
    code=hash_getcode(hwid,sz);

    for(i=0;i<matcher->col->driverpack_handle.items;i++)
    {
        drp=&matcher->col->driverpack_list[i];
        int val=hash_find(&drp->indexes,(char *)&code,4,&isfound);
        while(isfound)
        {
            hwidmatch=(hwidmatch_t *)heap_allocitem_ptr(&matcher->hwidmatch_handle);
            hwidmatch_init(hwidmatch,drp,val,dev_pos,ishw,matcher->state,devicematch);
            devicematch->num_matches++;
            val=hash_findnext_b(&drp->indexes,&isfound);
        }
    }
}

void matcher_populate(matcher_t *matcher)
{
    devicematch_t *devicematch;
    state_t *state=matcher->state;
    driver_t *cur_driver;
    device_t *cur_device;
    WCHAR *p;
    char *s=state->text;
    char buf[4096];
    int dev_pos;
    int i;

    time_matcher=GetTickCount();
    cur_device=state->devices_list;
    heap_reset(&matcher->devicematch_handle,0);
    heap_reset(&matcher->hwidmatch_handle,0);

    genmarker(state);
    for(i=0;i<state->devices_handle.items;i++,cur_device++)
    {
        cur_driver=0;
        if(cur_device->driver_index>=0)cur_driver=&matcher->state->drivers_list[cur_device->driver_index];
        devicematch=(devicematch_t *)heap_allocitem_ptr(&matcher->devicematch_handle);
        devicematch_init(devicematch,cur_device,cur_driver,matcher->hwidmatch_handle.items);
        if(cur_device->HardwareID)
        {
            p=(WCHAR *)(s+cur_device->HardwareID);
            dev_pos=0;
            while(*p)
            {
                sprintf(buf,"%ws",p);
                matcher_findHWIDs(matcher,devicematch,buf,dev_pos,1);
                p+=lstrlen(p)+1;
                dev_pos++;
            }
        }

        if(cur_device->CompatibleIDs)
        {
            p=(WCHAR *)(s+cur_device->CompatibleIDs);
            dev_pos=0;
            while(*p)
            {
                sprintf(buf,"%ws",p);
                matcher_findHWIDs(matcher,devicematch,buf,dev_pos,0);
                p+=lstrlen(p)+1;
                dev_pos++;
            }
        }
        if(!devicematch->num_matches)
        {
            heap_allocitem_ptr(&matcher->hwidmatch_handle);

            if(isMissing(cur_device,cur_driver,state))devicematch->status=STATUS_NF_MISSING;else
            if(devicematch->driver)
            {
                if(!wcscmp((WCHAR *)(state->text+devicematch->driver->ProviderName),L"Microsoft"))
                    devicematch->status=STATUS_NF_STANDARD;
                else
                {
                    if(*state->text+devicematch->driver->MatchingDeviceId)
                        devicematch->status=STATUS_NF_UNKNOWN;
                    else
                        devicematch->status=STATUS_NF_STANDARD;
                }
            }
            else
            {
                if(devicematch->device->problem)
                    devicematch->status=devicematch->device->HardwareID?STATUS_NF_MISSING:STATUS_NF_STANDARD;
                else
                    devicematch->status=STATUS_NF_STANDARD;
            }
        }
    }
}

void matcher_sort(matcher_t *matcher)
{
    devicematch_t *devicematch;
    hwidmatch_t *match1,*match2,*bestmatch;
    hwidmatch_t matchtmp;
    char sect1[BUFLEN],sect2[BUFLEN];
    int k,i,j;

    devicematch=matcher->devicematch_list;
    for(k=0;k<matcher->devicematch_handle.items;k++,devicematch++)
    {
        // Sort
        match1=&matcher->hwidmatch_list[devicematch->start_matches];
        for(i=0;i<devicematch->num_matches-1;i++,match1++)
        {
            match2=&matcher->hwidmatch_list[devicematch->start_matches+i+1];
            bestmatch=match1;
            for(j=i+1;j<devicematch->num_matches;j++,match2++)
                if(hwidmatch_cmp(bestmatch,match2)<0)bestmatch=match2;

            if(bestmatch!=match1)
            {
                memcpy(&matchtmp,match1,sizeof(hwidmatch_t));
                memcpy(match1,bestmatch,sizeof(hwidmatch_t));
                memcpy(bestmatch,&matchtmp,sizeof(hwidmatch_t));
            }

        }

        // Mark dups
        match1=&matcher->hwidmatch_list[devicematch->start_matches];
        for(i=0;i<devicematch->num_matches-1;i++,match1++)
        {
            getdrp_drvsection(match1,sect1);

            match2=&matcher->hwidmatch_list[devicematch->start_matches+i+1];
            for(j=i+1;j<devicematch->num_matches;j++,match2++)
            {
                getdrp_drvsection(match2,sect2);

                if(getdrp_infcrc(match1)==getdrp_infcrc(match2)&&
                   !strcmp(getdrp_drvHWID(match1),getdrp_drvHWID(match2))&&
                   !strcmp(sect1,sect2))match2->status|=STATUS_DUP;

            }
        }
    }
    time_matcher=GetTickCount()-time_matcher;
}

void matcher_print(matcher_t *matcher)
{
    devicematch_t *devicematch;
    device_t *cur_device;
    hwidmatch_t *hwidmatch;
    int limits[7];
    int i,j;

    if((log_verbose&LOG_VERBOSE_MATCHER)==0)return;
    log_file("\n{matcher_print[devices=%d,hwids=%d]\n",matcher->devicematch_handle.items,matcher->hwidmatch_handle.items);
    devicematch=matcher->devicematch_list;
    for(i=0;i<matcher->devicematch_handle.items;i++,devicematch++)
    {
        cur_device=devicematch->device;
        device_print(cur_device,matcher->state);
        log_file("DriverInfo\n");
        if(devicematch->driver)
            driver_print(devicematch->driver,matcher->state);
        else
            log_file("##NoDriver\n");

        memset(limits,0,sizeof(limits));
        hwidmatch=&matcher->hwidmatch_list[devicematch->start_matches];
        for(j=0;j<matcher->devicematch_list[i].num_matches;j++,hwidmatch++)
            hwidmatch_calclen(hwidmatch,limits);

        hwidmatch=&matcher->hwidmatch_list[devicematch->start_matches];
        for(j=0;j<matcher->devicematch_list[i].num_matches;j++,hwidmatch++)
            hwidmatch_print(hwidmatch,limits);
        log_file("\n");
    }
    log_file("}matcher_print\n\n");
}
//}

//{ Getters
//driverpack
WCHAR *getdrp_packpath(hwidmatch_t *hwidmatch)
{
    driverpack_t *drp=hwidmatch->drp;
    return (WCHAR*)(drp->text+drp->drppath);
}
WCHAR *getdrp_packname(hwidmatch_t *hwidmatch)
{
    driverpack_t *drp=hwidmatch->drp;
    return (WCHAR*)(drp->text+drp->drpfilename);
}
int getdrp_packontorrent(hwidmatch_t *hwidmatch)
{
    return hwidmatch->drp->type==DRIVERPACK_TYPE_UPDATE;
}

//inffile
char *getdrp_infpath(hwidmatch_t *hwidmatch)
{
    driverpack_t *drp=hwidmatch->drp;
    int desc_index=drp->HWID_list[hwidmatch->HWID_index].desc_index;
    int manufacturer_index=drp->desc_list[desc_index].manufacturer_index;
    int inffile_index=drp->manufacturer_list[manufacturer_index].inffile_index;
    return drp->text+drp->inffile[inffile_index].infpath;
}
char *getdrp_infname(hwidmatch_t *hwidmatch)
{
    driverpack_t *drp=hwidmatch->drp;
    int desc_index=drp->HWID_list[hwidmatch->HWID_index].desc_index;
    int manufacturer_index=drp->desc_list[desc_index].manufacturer_index;
    int inffile_index=drp->manufacturer_list[manufacturer_index].inffile_index;
    return drp->text+drp->inffile[inffile_index].inffilename;
}
char *getdrp_drvfield(hwidmatch_t *hwidmatch,int n)
{
    driverpack_t *drp=hwidmatch->drp;
    int desc_index=drp->HWID_list[hwidmatch->HWID_index].desc_index;
    int manufacturer_index=drp->desc_list[desc_index].manufacturer_index;
    int inffile_index=drp->manufacturer_list[manufacturer_index].inffile_index;
    if(!drp->inffile[inffile_index].fields[n])return "";
    return drp->text+drp->inffile[inffile_index].fields[n];
}
char *getdrp_drvcat(hwidmatch_t *hwidmatch,int n)
{
    driverpack_t *drp=hwidmatch->drp;
    int desc_index=drp->HWID_list[hwidmatch->HWID_index].desc_index;
    int manufacturer_index=drp->desc_list[desc_index].manufacturer_index;
    int inffile_index=drp->manufacturer_list[manufacturer_index].inffile_index;
    if(!drp->inffile[inffile_index].cats[n])return "";
    return drp->text+drp->inffile[inffile_index].cats[n];
}
version_t *getdrp_drvversion(hwidmatch_t *hwidmatch)
{
    driverpack_t *drp=hwidmatch->drp;
    int desc_index=drp->HWID_list[hwidmatch->HWID_index].desc_index;
    int manufacturer_index=drp->desc_list[desc_index].manufacturer_index;
    int inffile_index=drp->manufacturer_list[manufacturer_index].inffile_index;
    return &drp->inffile[inffile_index].version;
}
int getdrp_infsize(hwidmatch_t *hwidmatch)
{
    driverpack_t *drp=hwidmatch->drp;
    int desc_index=drp->HWID_list[hwidmatch->HWID_index].desc_index;
    int manufacturer_index=drp->desc_list[desc_index].manufacturer_index;
    int inffile_index=drp->manufacturer_list[manufacturer_index].inffile_index;
    return drp->inffile[inffile_index].infsize;
}
int getdrp_infcrc(hwidmatch_t *hwidmatch)
{
    driverpack_t *drp=hwidmatch->drp;
    int desc_index=drp->HWID_list[hwidmatch->HWID_index].desc_index;
    int manufacturer_index=drp->desc_list[desc_index].manufacturer_index;
    int inffile_index=drp->manufacturer_list[manufacturer_index].inffile_index;
    return drp->inffile[inffile_index].infcrc;
}

//manufacturer
char *getdrp_drvmanufacturer(hwidmatch_t *hwidmatch)
{
    driverpack_t *drp=hwidmatch->drp;
    int desc_index=drp->HWID_list[hwidmatch->HWID_index].desc_index;
    int manufacturer_index=drp->desc_list[desc_index].manufacturer_index;
    return drp->text+drp->manufacturer_list[manufacturer_index].manufacturer;
}
void getdrp_drvsectionAtPos(driverpack_t *drp,char *buf,int pos,int manuf_index)
{
    intptr_t rr=(intptr_t)drp->text+drp->manufacturer_list[manuf_index].sections;
    if(pos)
    {
        char *ext=drp->text+((int *)rr)[pos];
        sprintf(buf,"%s.%s",drp->text+((int *)rr)[0],ext);
    }
    else
        sprintf(buf,"%s",drp->text+((int *)rr)[pos]);
}
void getdrp_drvsection(hwidmatch_t *hwidmatch,char *buf)
{
    driverpack_t *drp=hwidmatch->drp;
    int desc_index=drp->HWID_list[hwidmatch->HWID_index].desc_index;
    int manufacturer_index=drp->desc_list[desc_index].manufacturer_index;
    getdrp_drvsectionAtPos(drp,buf,drp->desc_list[desc_index].sect_pos,manufacturer_index);
}

//desc
char *getdrp_drvdesc(hwidmatch_t *hwidmatch)
{
    driverpack_t *drp=hwidmatch->drp;
    int desc_index=drp->HWID_list[hwidmatch->HWID_index].desc_index;
    return drp->text+drp->desc_list[desc_index].desc;
}
char *getdrp_drvinstall(hwidmatch_t *hwidmatch)
{
    driverpack_t *drp=hwidmatch->drp;
    int desc_index=drp->HWID_list[hwidmatch->HWID_index].desc_index;
    return drp->text+drp->desc_list[desc_index].install;
}
char *getdrp_drvinstallPicked(hwidmatch_t *hwidmatch)
{
    driverpack_t *drp=hwidmatch->drp;
    int desc_index=drp->HWID_list[hwidmatch->HWID_index].desc_index;
    return drp->text+drp->desc_list[desc_index].install_picked;
}
int getdrp_drvfeature(hwidmatch_t *hwidmatch)
{
    driverpack_t *drp=hwidmatch->drp;
    int desc_index=drp->HWID_list[hwidmatch->HWID_index].desc_index;
    return drp->desc_list[desc_index].feature&0xFF;
}

//HWID
short getdrp_drvinfpos(hwidmatch_t *hwidmatch)
{
    driverpack_t *drp=hwidmatch->drp;
    return drp->HWID_list[hwidmatch->HWID_index].inf_pos;
}
char *getdrp_drvHWID(hwidmatch_t *hwidmatch)
{
    driverpack_t *drp=hwidmatch->drp;
    return drp->text+drp->HWID_list[hwidmatch->HWID_index].HWID;
}
//}
