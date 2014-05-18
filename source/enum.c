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
int isLaptop;

const char *deviceststus_str[]=
{
    "Device is not present",
    "Device is disabled",
    "The device has the following problem: %d",
    "The driver reported a problem with the device",
    "Driver is running",
    "Device is currently stopped"
};

#ifdef STORE_PROPS
const dev devtbl[NUM_PROPS]=
{
/**/{SPDRP_DEVICEDESC                   ,"Devicedesc"},
/**/{SPDRP_HARDWAREID                   ,"HardwareID"},
/**/{SPDRP_COMPATIBLEIDS                ,"CompatibleIDs"},
//   SPDRP_UNUSED0           3
    {SPDRP_SERVICE                      ,"Service"},
//   SPDRP_UNUSED1           5
//   SPDRP_UNUSED2           6
    {SPDRP_CLASS                        ,"Class"},
    {SPDRP_CLASSGUID                    ,"ClassGUID"},
/**/{SPDRP_DRIVER                       ,"Driver"},
    {SPDRP_CONFIGFLAGS                  ,"ConfigFlags"},
/**/{SPDRP_MFG                          ,"Mfg"},
/**/{SPDRP_FRIENDLYNAME                 ,"FriendlyName"},
    {SPDRP_LOCATION_INFORMATION         ,"LocationInformation"},
    {SPDRP_PHYSICAL_DEVICE_OBJECT_NAME  ,"PhysicalDeviceObjectName"},
    {SPDRP_CAPABILITIES                 ,"Capabilities"},
    {SPDRP_UI_NUMBER                    ,"UINumber"},
//   SPDRP_UPPERFILTERS      17
//   SPDRP_LOWERFILTERS      18
    {SPDRP_BUSTYPEGUID                  ,"BusTypeGUID"},
    {SPDRP_LEGACYBUSTYPE                ,"LegacyBusType"},
    {SPDRP_BUSNUMBER                    ,"BusNumber"},
    {SPDRP_ENUMERATOR_NAME              ,"EnumeratorName"},
//   SPDRP_SECURITY          23
//   SPDRP_SECURITY_SDS      24
//   SPDRP_DEVTYPE           25
//   SPDRP_EXCLUSIVE         26
//   SPDRP_CHARACTERISTICS   27
    {SPDRP_ADDRESS                      ,"Address"},
//                           29
    {SPDRP_UI_NUMBER_DESC_FORMAT        ,"UINumberDescFormat"},
};
#endif // STORE_PROPS
//}

//{ Print
void print_guid(GUID *g)
{
    WCHAR buffer[BUFLEN];
    /*log("%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X",g->Data1,g->Data2,g->Data3,
        (int)(g->Data4[0]),(int)(g->Data4[1]),
        (int)(g->Data4[2]),(int)(g->Data4[3]),(int)(g->Data4[4]),
        (int)(g->Data4[5]),(int)(g->Data4[6]),(int)(g->Data4[7]));*/

    memset(buffer,0,sizeof(buffer));
    if(!SetupDiGetClassDescription(g,buffer,BUFLEN,0))
    {
        //int lr=GetLastError();
        //print_error(lr,L"print_guid()");
    }
    log("%ws\n",buffer);
}

int print_status(device_t *device)
{
    int isPhantom=0;

    if(device->ret!=CR_SUCCESS)
    {
        if((device->ret==CR_NO_SUCH_DEVINST)||(device->ret==CR_NO_SUCH_VALUE))isPhantom=1;
    }

    if(isPhantom)
        return 0;
    else
    {
        if((device->status&DN_HAS_PROBLEM)&&device->problem==CM_PROB_DISABLED)
            return 1;
        else
        {
            if(device->status&DN_HAS_PROBLEM)
                return 2;
            else if(device->status&DN_PRIVATE_PROBLEM)
                return 3;
            else if(device->status&DN_STARTED)
                return 4;
            else
                return 5;
        }
    }
}
//}

//{ Device/driver
void read_device_property(HDEVINFO hDevInfo,SP_DEVINFO_DATA *DeviceInfoData,state_t *state,int id,ofst *val)
{
    DWORD buffersize=0;
    int lr;
    DWORD DataT=0;
    PBYTE p;
    BYTE buf[BUFLEN];

    memset(buf,0,BUFLEN);

    if(!SetupDiGetDeviceRegistryProperty(hDevInfo,DeviceInfoData,id,&DataT,0,0,&buffersize))
    {
        lr=GetLastError();
        if(lr==ERROR_INVALID_DATA)return;
        if(lr!=ERROR_INSUFFICIENT_BUFFER)
        {
            log("Property %d\n",id);
            print_error(lr,L"read_device_property()");
            return;
        }
    }

    if(DataT==REG_DWORD)
    {
        p=(PBYTE)val;
    }
    else
    {
        *val=heap_alloc(&state->text_handle,buffersize);
        p=(PBYTE)(state->text+*val);
    }
    memset(p,0,buffersize);
    if(!SetupDiGetDeviceRegistryProperty(hDevInfo,DeviceInfoData,id,&DataT,(PBYTE)buf,buffersize,&buffersize))
    {
        lr=GetLastError();
        log("Property %d\n",id);
        print_error(lr,L"read_device_property()");
        return;
    }
    memcpy(p,buf,buffersize);
}

void read_reg_val(HKEY hkey,state_t *state,const WCHAR *key,ofst *val)
{
    DWORD dwType,dwSize=0;
    int lr;

    *val=0;
    lr=RegQueryValueEx(hkey,key,NULL,0,0,&dwSize);
    if(lr==ERROR_FILE_NOT_FOUND)return;
    if(lr!=ERROR_SUCCESS)
    {
        log("Key %ws\n",key);
        print_error(lr,L"RegQueryValueEx()");
        return;
    }

    *val=heap_alloc(&state->text_handle,(int)(dwSize));
    lr=RegQueryValueEx(hkey,key,NULL,&dwType,(unsigned char *)(state->text+*val),&dwSize);
    if(lr!=ERROR_SUCCESS)
    {
        log("Key %ws\n",key);
        print_error(lr,L"read_reg_val()");
    }
}

void device_print(device_t *cur_device,state_t *state)
{
    /*log("Device[%d]\n",i);
    log("  InstanceID:'%ws'\n",state->text+cur_device->InstanceId);
    log("  ClassGuid:");printguid(&cur_device->DeviceInfoData.ClassGuid);
    log("  DevInst: %d\n",cur_device->DeviceInfoData.DevInst);*/

#ifdef STORE_PROPS
    WCHAR *buffer;
    int DataT;
    int buffersize;
    int j;

    for(j=0;j<NUM_PROPS;j++)
    {
        break;
        if(!cur_device->PropertiesSize[j])continue;
        buffer=(WCHAR *)(state->text+cur_device->Properties[j]);
        DataT=cur_device->PropertiesType[j];
        buffersize=cur_device->PropertiesSize[j];
        log("%s:",devtbl[j].name);
        switch(DataT)
        {
            case REG_NONE:
                log("REG_NONE\n",buffer);
                break;
            case REG_SZ:
                log("REG_SZ:\n  %ws\n",buffer);
                break;
            case REG_BINARY:
                log("REG_BINARY:\n  %d'%.*s'\n",buffersize,buffersize,buffer);
                break;
            case REG_DWORD:
                log("REG_DWORD:\n  %d,%08X\n",*buffer,*buffer);
                break;
            case REG_MULTI_SZ:
                p=buffer;
                log("REG_MULTI_SZ\n");
                while(*p)
                {
                    log("  %ws\n",p);
                    p+=lstrlen(p)+1;
                }
                break;
            default:
                log("Def: %d\n",DataT);
        }
    }
#endif

    char *s=state->text;

    log("DeviceInfo\n");
    log("##Name:#########%ws\n",s+cur_device->Devicedesc);
    log("##Status:#######");
    log(deviceststus_str[print_status(cur_device)],cur_device->problem);
    log("\n##Manufacturer:#%ws\n",s+cur_device->Mfg);
    log("##HWID_reg######%ws\n",s+cur_device->Driver);
    log("##Class:########");    print_guid(&cur_device->DeviceInfoData.ClassGuid);
    log("##Location:#####\n");
    log("##ConfigFlags:##%d\n", cur_device->ConfigFlags);
    log("##Capabilities:#%d\n", cur_device->Capabilities);
}

void device_printHWIDS(device_t *cur_device,state_t *state)
{
    WCHAR *p;
    char *s=state->text;

    if(cur_device->HardwareID)
    {
        p=(WCHAR *)(s+cur_device->HardwareID);
        log("HardwareID\n");
        while(*p)
        {
            log("  %ws\n",p);
            p+=lstrlen(p)+1;
        }
    }
    else
    {
        log("NoID\n");
    }

    if(cur_device->CompatibleIDs)
    {
        p=(WCHAR *)(s+cur_device->CompatibleIDs);
        log("CompatibleID\n");
        while(*p)
        {
            log("  %ws\n",p);
            p+=lstrlen(p)+1;
        }
    }
}

void driver_print(driver_t *cur_driver,state_t *state)
{
    char *s=state->text;
    WCHAR buf[BUFLEN];

    str_date(&cur_driver->version,buf);
    log("##Name:#####%ws\n",s+cur_driver->DriverDesc);
    log("##Provider:#%ws\n",s+cur_driver->ProviderName);
    log("##Date:#####%ws\n",buf);
    log("##Version:##%d.%d.%d.%d\n",cur_driver->version.v1,cur_driver->version.v2,cur_driver->version.v3,cur_driver->version.v4);
    log("##HWID:#####%ws\n",s+cur_driver->MatchingDeviceId);
    log("##inf:######%ws%ws,%ws%ws\n",(s+state->windir),s+cur_driver->InfPath,s+cur_driver->InfSection,s+cur_driver->InfSectionExt);
    int score=calc_score_h(cur_driver,state);
    log("##Score:####%08X %04x\n",score,cur_driver->identifierscore);
}
//}

//{ State
void state_init(state_t *state)
{
    memset(state,0,sizeof(state_t));
    heap_init(&state->devices_handle,ID_STATE_DEVICES,(void **)&state->devices_list,0,sizeof(device_t));
    heap_init(&state->drivers_handle,ID_STATE_DRIVERS,(void **)&state->drivers_list,0,sizeof(driver_t));
    heap_init(&state->text_handle,ID_STATE_TEXT,(void **)&state->text,0,1);
    heap_alloc(&state->text_handle,2);
    state->text[0]=0;
    state->text[1]=0;
    state->revision=SVN_REV;

    //log("sizeof(device_t)=%d\nsizeof(driver_t)=%d\n\n",sizeof(device_t),sizeof(driver_t));
}

void state_free(state_t *state)
{
    heap_free(&state->devices_handle);
    heap_free(&state->drivers_handle);
    heap_free(&state->text_handle);
}

void state_save(state_t *state,const WCHAR *filename)
{
    FILE *f;
    int sz;
    int version=VER_STATE;
    char *mem,*mem_pack,*p;

    if(flags&FLAG_NOSNAPSHOT)return;
    log("Saving state in '%ws'...",filename);
    if(!canWrite(filename))
    {
        log_err("ERROR in state_save(): Write-protected,'%ws'\n",filename);
        return;
    }
    f=_wfopen(filename,L"wb");
    if(!f)
    {
        log_err("ERROR in state_save(): failed _wfopen(%ws)\n",errno_str());
        return;
    }

    sz=
        sizeof(state_m_t)+
        state->drivers_handle.used+
        state->devices_handle.used+
        state->text_handle.used+
        2*3*sizeof(int);  // 3 heaps

    p=mem=(char *)malloc(sz);

    fwrite(VER_MARKER,3,1,f);
    fwrite(&version,sizeof(int),1,f);

    memcpy(p,state,sizeof(state_m_t));p+=sizeof(state_m_t);
    p=heap_save(&state->devices_handle,p);
    p=heap_save(&state->drivers_handle,p);
    p=heap_save(&state->text_handle,p);

    if(1)
    {
        mem_pack=(char *)malloc(sz);
        sz=encode(mem_pack,sz,mem,sz);
        fwrite(mem_pack,sz,1,f);
        free(mem_pack);
    }
    else fwrite(mem,sz,1,f);

    free(mem);
    fclose(f);
    log("OK\n");
}

int  state_load(state_t *state,const WCHAR *filename)
{
    char buf[4096];
    //WCHAR txt2[256];
    FILE *f;
    int sz;
    int version;
    char *mem,*p,*mem_unpack=0;

    log_con("Loading state from '%ws'...",filename);
    f=_wfopen(filename,L"rb");
    if(!f)
    {
        log_err("FAILED(%ws)\n",errno_str());
        return 0;
    }

    fseek(f,0,SEEK_END);
    sz=ftell(f);
    fseek(f,0,SEEK_SET);

    fread(buf,3,1,f);
    fread(&version,sizeof(int),1,f);
    sz-=3+sizeof(int);

    if(memcmp(buf,VER_MARKER,3))
    {
        log_err("FAILED(invalid snapshot)\n");
        return 0;
    }
    if(version!=VER_STATE)
    {
        log_err("FAILED(invalid version)\n");
        return 0;
    }

    p=mem=(char *)malloc(sz);
    fread(mem,sz,1,f);

    if(1)
    {
        UInt64 sz_unpack;

        Lzma86_GetUnpackSize((Byte *)p,sz,&sz_unpack);
        mem_unpack=(char *)malloc(sz_unpack);
        decode(mem_unpack,sz_unpack,mem,sz);
        p=mem_unpack;
    }

    memcpy(state,p,sizeof(state_m_t));p+=sizeof(state_m_t);
    p=heap_load(&state->devices_handle,p);
    p=heap_load(&state->drivers_handle,p);
    p=heap_load(&state->text_handle,p);

    state_fakeOSversion(state);

    free(mem);
    if(mem_unpack)free(mem_unpack);
    fclose(f);
    log("OK\n");
    return 1;
}

void state_fakeOSversion(state_t *state)
{
    if(virtual_arch_type==32)state->architecture=0;
    if(virtual_arch_type==64)state->architecture=1;
    if(virtual_os_version)
    {
        state->platform.dwMajorVersion=virtual_os_version/10;
        state->platform.dwMinorVersion=virtual_os_version%10;
    }
}

void state_print(state_t *state)
{
    int i,x,y;
    device_t *cur_device;
    WCHAR *buf;
    SYSTEM_POWER_STATUS *battery;

    if(log_verbose&LOG_VERBOSE_SYSINFO)
    {
        log("Windows\n");
        log("  Version:     %ws (%d.%d.%d)\n",get_winverstr(manager_g),state->platform.dwMajorVersion,state->platform.dwMinorVersion,state->platform.dwBuildNumber);
        log("  PlatformId:  %d\n",state->platform.dwPlatformId);
        log("  Update:      %ws\n",state->platform.szCSDVersion);
        if(state->platform.dwOSVersionInfoSize == sizeof(OSVERSIONINFOEX))
        {
            log("  ServicePack: %d.%d\n",state->platform.wServicePackMajor,state->platform.wServicePackMinor);
            log("  SuiteMask:   %d\n",state->platform.wSuiteMask);
            log("  ProductType: %d\n",state->platform.wProductType);
        }
        log("\nEnvironment\n");
        log("  windir:      %ws\n",state->text+state->windir);
        log("  temp:        %ws\n",state->text+state->temp);

        log("\nMotherboard\n");
        log("  Product:     %ws\n",state->text+state->product);
        log("  Model:       %ws\n",state->text+state->model);
        log("  Manuf:       %ws\n",state->text+state->manuf);
        log("  cs_Model:    %ws\n",state->text+state->cs_model);
        log("  cs_Manuf:    %ws\n",state->text+state->cs_manuf);
        log("  Chassis:     %d\n",state->ChassisType);

        log("\nBattery\n");
        battery=(SYSTEM_POWER_STATUS *)(state->text+state->battery);
        log("  AC_Status:   ");
        switch(battery->ACLineStatus)
        {
            case 0:log("Offline\n");break;
            case 1:log("Online\n");break;
            default:
            case 255:log("Unknown\n");break;
        }
        i=battery->BatteryFlag;
        log("  Flags:       %d",i);
        if(i&1)log("[high]");
        if(i&2)log("[low]");
        if(i&4)log("[critical]");
        if(i&8)log("[charging]");
        if(i&128)log("[no battery]");
        if(i==255)log("[unknown]");
        log("\n");
        if(battery->BatteryLifePercent!=255)
            log("  Charged:      %d\n",battery->BatteryLifePercent);
        if(battery->BatteryLifeTime!=0xFFFFFFFF)
            log("  LifeTime:     %d mins\n",battery->BatteryLifeTime/60);
        if(battery->BatteryFullLifeTime!=0xFFFFFFFF)
            log("  FullLifeTime: %d mins\n",battery->BatteryFullLifeTime/60);

        buf=(WCHAR *)(state->text+state->monitors);
        log("\nMonitors\n");
        for(i=0;i<buf[0];i++)
        {
            x=buf[1+i*2];
            y=buf[2+i*2];
            log("  %dcmx%dcm (%.1fin)\t%.3f %s\n",x,y,sqrt(x*x+y*y)/2.54,(double)y/x,iswide(x,y)?"wide":"");
        }

        log("\nMisc\n");
        log("  Type:        %s\n",isLaptop?"Laptop":"Desktop");
        log("  Locale:      %X\n",state->locale);
        log("  CPU_Arch:    %s\n",state->architecture?"64-bit":"32-bit");
        log("\n");

    }

    if(log_verbose&LOG_VERBOSE_DEVICES)
    for(i=0;i<state->devices_handle.items;i++)
    {
        cur_device=&state->devices_list[i];

        device_print(cur_device,state);

        log("DriverInfo\n",cur_device->driver_index);
        if(cur_device->driver_index>=0)
            driver_print(&state->drivers_list[cur_device->driver_index],state);
        else
            log("##NoDriver\n");

        device_printHWIDS(cur_device,state);
        log("\n\n");
    }
    //log("Errors: %d\n",error_count);
}

void state_getsysinfo_fast(state_t *state)
{
    SYSTEM_POWER_STATUS *battery;
    DISPLAY_DEVICE DispDev;
    int x,y,i=0;
    WCHAR buf[BUFLEN];

    time_test=GetTickCount();

    heap_reset(&state->text_handle,2);
    // Battery
    state->battery=heap_alloc(&state->text_handle,sizeof(SYSTEM_POWER_STATUS));
    battery=(SYSTEM_POWER_STATUS *)(state->text+state->battery);
    GetSystemPowerStatus(battery);

    // Monitors
    memset(&DispDev,0,sizeof(DispDev));
    DispDev.cb=sizeof(DispDev);
    buf[0]=0;
    while(EnumDisplayDevices(0,i,&DispDev,0))
    {
        GetMonitorSizeFromEDID(DispDev.DeviceName,&x,&y);
        if(x&&y)
        {
            buf[buf[0]*2+1]=x;
            buf[buf[0]*2+2]=y;
            buf[0]++;
        }
        i++;
    }
    state->monitors=heap_memcpy(&state->text_handle,buf,(1+buf[0]*2)*2);

    // Windows version
    state->platform.dwOSVersionInfoSize=sizeof(OSVERSIONINFOEX);
    if(!(GetVersionEx((OSVERSIONINFO*)&state->platform)))
    {
        state->platform.dwOSVersionInfoSize=sizeof(OSVERSIONINFO);
        if(!GetVersionEx((OSVERSIONINFO*)&state->platform))
        print_error(GetLastError(),L"GetVersionEx()");
    }
    state->locale=GetUserDefaultLCID();

    // Environment
    GetEnvironmentVariable(L"windir",buf,BUFLEN);
    wcscat(buf,L"\\inf\\");
    state->windir=heap_memcpy(&state->text_handle,buf,wcslen(buf)*2+2);

    GetEnvironmentVariable(L"TEMP",buf,BUFLEN);
    state->temp=heap_memcpy(&state->text_handle,buf,wcslen(buf)*2+2);

    // 64-bit detection
    state->architecture=0;
    *buf=0;
    GetEnvironmentVariable(L"PROCESSOR_ARCHITECTURE",buf,BUFLEN);
    if(!lstrcmpi(buf,L"AMD64"))state->architecture=1;
    *buf=0;
    GetEnvironmentVariable(L"PROCESSOR_ARCHITEW6432",buf,BUFLEN);
    if(*buf)state->architecture=1;

    state_fakeOSversion(state);
}

WCHAR *state_getproduct(state_t *state)
{
    WCHAR *s=(WCHAR *)(state->text+state->product);

    if(StrStrIW(s,L"Product"))return (WCHAR *)(state->text+state->cs_model);
    return s;
}

WCHAR *state_getmanuf(state_t *state)
{
    WCHAR *s=(WCHAR *)(state->text+state->manuf);

    if(StrStrIW(s,L"Vendor"))return (WCHAR *)(state->text+state->cs_manuf);
    return s;
}

WCHAR *state_getmodel(state_t *state)
{
    WCHAR *s=(WCHAR *)(state->text+state->model);

    if(!*s)return (WCHAR *)(state->text+state->cs_model);
    return s;
}

void state_getsysinfo_slow(state_t *state)
{
    WCHAR manuf[BUFLEN];
    WCHAR model[BUFLEN];
    WCHAR product[BUFLEN];
    WCHAR cs_manuf[BUFLEN];
    WCHAR cs_model[BUFLEN];

    time_sysinfo=GetTickCount();

    getbaseboard(manuf,model,product,cs_manuf,cs_model,&state->ChassisType);
    state->manuf=heap_memcpy(&state->text_handle,manuf,wcslen(manuf)*2+2);
    state->product=heap_memcpy(&state->text_handle,product,wcslen(product)*2+2);
    state->model=heap_memcpy(&state->text_handle,model,wcslen(model)*2+2);
    state->cs_manuf=heap_memcpy(&state->text_handle,cs_manuf,wcslen(cs_manuf)*2+2);
    state->cs_model=heap_memcpy(&state->text_handle,cs_model,wcslen(cs_model)*2+2);

    time_sysinfo=GetTickCount()-time_sysinfo;
}

int opencatfile(state_t *state,driver_t *cur_driver)
{
    WCHAR filename[BUFLEN];
    CHAR bufa[BUFLEN];

    wcscpy(filename,(WCHAR *)(state->text+state->windir));
    wsprintf(filename+wcslen(filename)-4,
             L"system32\\CatRoot\\{F750E6C3-38EE-11D1-85E5-00C04FC295EE}\\%ws",
             (WCHAR *)(state->text+cur_driver->InfPath));

    wcscpy(filename+wcslen(filename)-3,L"cat");
    {
        FILE *f;
        char *buft;
        int len;

        *bufa=0;
        f=_wfopen(filename,L"rb");
        if(f)
        {
            //log_con("Open '%ws'\n",filename);
            fseek(f,0,SEEK_END);
            len=ftell(f);
            fseek(f,0,SEEK_SET);
            buft=(char *)malloc(len);
            fread(buft,len,1,f);
            fclose(f);
            {
                unsigned bufal=0;
                char *p=buft;

                while(p+11<buft+len)
                {
                    if(*p=='O'&&!memcmp(p,L"OSAttr",11))
                    {
                        if(!*bufa||bufal<wcslen((WCHAR *)(p+19)))
                        {
                            sprintf(bufa,"%ws",p+19);
                            bufal=strlen(bufa);
                        }
                    }
                    p++;
                }
                //if(*bufa)log_con("[%s]\n",bufa);
            }
            free(buft);
        }
    }

    return *bufa?heap_strcpy(&state->text_handle,bufa):0;
}

void state_scandevices(state_t *state)
{
    HDEVINFO hDevInfo;
    HKEY   hkey;
    SP_DEVINFO_DATA *DeviceInfoData;
    WCHAR buf[BUFLEN];
    DWORD buffersize;
    driver_t *cur_driver;
    collection_t collection;
    driverpack_t unpacked_drp;
    hashtable_t inf_list;
    infdata_t *infdata;
    unsigned i;
    int r;
    int lr;

    time_devicescan=GetTickCount();
    collection_init(&collection,(WCHAR *)(state->text+state->windir),L"",L"",0);
    driverpack_init(&unpacked_drp,L"",L"windir.7z",&collection);
    hash_init(&inf_list,ID_INF_LIST,200,HASH_FLAG_KEYS_ARE_POINTERS);
    heap_reset(&state->devices_handle,0);

/*
    DWORD HwProfileList[4096];
    DWORD HwProfileListSize=4096;
    DWORD RequiredSize;
    DWORD CurrentlyActiveIndex;
    log("Profiles\n");
    SetupDiGetHwProfileList(HwProfileList,HwProfileListSize,&RequiredSize,&CurrentlyActiveIndex);
    for(i=0;i<RequiredSize;i++)
    {
        SetupDiGetHwProfileFriendlyName(i,buf,4096,0);
        log("  %d:%ws%s\n",RequiredSize,buf,(CurrentlyActiveIndex==i)?"(current)":"");
    }
    log("\n");
*/
    hDevInfo=SetupDiGetClassDevs(0,0,0,DIGCF_PRESENT|DIGCF_ALLCLASSES);
    if(hDevInfo==INVALID_HANDLE_VALUE)
    {
        print_error(GetLastError(),L"SetupDiGetClassDevs()");
        return;
    }

    i=0;
    while(1)
    {
        //log("%d,%d/%d\n",i,state->text_handle.used,state->text_handle.allocated);
        heap_refresh(&state->text_handle);
        device_t *cur_device=(device_t *)heap_allocitem_ptr(&state->devices_handle);
        DeviceInfoData=&cur_device->DeviceInfoData;
        memset(DeviceInfoData,0,sizeof(SP_DEVINFO_DATA));
        DeviceInfoData->cbSize=sizeof(SP_DEVINFO_DATA);

        if(!SetupDiEnumDeviceInfo(hDevInfo,i,DeviceInfoData))
        {
            r=GetLastError();
            if(r==ERROR_NO_MORE_ITEMS)
                heap_freelastitem(&state->devices_handle);
            else
                print_error(r,L"SetupDiEnumDeviceInfo()");
            break;
        }

        SetupDiGetDeviceInstanceId(hDevInfo,DeviceInfoData,0,0,&buffersize);
        cur_device->InstanceId=heap_alloc(&state->text_handle,buffersize);
        SetupDiGetDeviceInstanceId(hDevInfo,DeviceInfoData,(WCHAR *)(state->text+cur_device->InstanceId),4096,0);

/*
        SP_DEVINSTALL_PARAMS  devdt;
        devdt.cbSize=sizeof(SP_DEVINSTALL_PARAMS);
        lr=SetupDiGetDeviceInstallParams(hDevInfo,DeviceInfoData,&devdt);
        log("%dDriverPath '%s'\n",lr,devdt.DriverPath);*/

        read_device_property(hDevInfo,DeviceInfoData,state,SPDRP_DEVICEDESC,    &cur_device->Devicedesc);
        read_device_property(hDevInfo,DeviceInfoData,state,SPDRP_HARDWAREID,    &cur_device->HardwareID);
        read_device_property(hDevInfo,DeviceInfoData,state,SPDRP_COMPATIBLEIDS, &cur_device->CompatibleIDs);
        read_device_property(hDevInfo,DeviceInfoData,state,SPDRP_DRIVER,        &cur_device->Driver);
        read_device_property(hDevInfo,DeviceInfoData,state,SPDRP_MFG,           &cur_device->Mfg);
        //if(i!=102&&i!=103&&i!=104&&i!=105&&i!=106&&i!=107&&i!=108&&i!=109&&i!=110&&
            /*i!=111&&i!=112&&i!=113&&i!=114&&i!=115&&*/
        //    i!=116&&i!=117&&i!=118&&i!=119&&i!=120&&i!=121&&i!=122)
        read_device_property(hDevInfo,DeviceInfoData,state,SPDRP_FRIENDLYNAME,  &cur_device->FriendlyName);
        read_device_property(hDevInfo,DeviceInfoData,state,SPDRP_CAPABILITIES,  &cur_device->Capabilities);
        read_device_property(hDevInfo,DeviceInfoData,state,SPDRP_CONFIGFLAGS,   &cur_device->ConfigFlags);

#ifdef STORE_PROPS
        WCHAR *buffer;
        DWORD DataT;
        int j;

        for(j=0;j<NUM_PROPS;j++)
        {
            lr=0;
            buffersize=0;
            buffer=0;
            while(!SetupDiGetDeviceRegistryProperty(hDevInfo,DeviceInfoData,devtbl[j].id,&DataT,(PBYTE)buffer,buffersize,&buffersize))
            {
                lr=GetLastError();
                if(lr==ERROR_INSUFFICIENT_BUFFER)
                {
                    cur_device->Properties[j]=heap_alloc(&state->text_handle,buffersize);
                    buffer=(WCHAR *)(state->text+cur_device->Properties[j]);
                    lr=0;
                }else
                if(lr==ERROR_INVALID_DATA)
                {
                    break;
                }else
                {
                    log("Property (%d)'%s'\n",devtbl[j].id,devtbl[j].name);
                    print_error(lr,L"SetupDiGetDeviceRegistryProperty()");
                    break;
                }
            }
            if(lr==ERROR_INVALID_DATA)continue;

            cur_device->PropertiesSize[j]=buffersize;
            cur_device->PropertiesType[j]=DataT;
        }
#endif

        lr=CM_Get_DevNode_Status(&cur_device->status,&cur_device->problem,cur_device->DeviceInfoData.DevInst,0);
        cur_device->ret=lr;
        if(lr!=CR_SUCCESS)
        {
            log("Error %d with CM_Get_DevNode_Status()\n",lr);
        }

        // Driver
        cur_device->driver_index=-1;
        if(!cur_device->Driver){i++;continue;}
        wsprintf(buf,
             L"SYSTEM\\CurrentControlSet\\Control\\Class\\%s",
             (WCHAR *)(state->text+cur_device->Driver));

        lr=RegOpenKeyEx(HKEY_LOCAL_MACHINE,buf,0,KEY_QUERY_VALUE,&hkey);
        if(lr==ERROR_FILE_NOT_FOUND)
        {

        }
        else if(lr!=ERROR_SUCCESS)
        {
            print_error(lr,L"RegOpenKeyEx()");
        }
        else if(lr==ERROR_SUCCESS)
        {
            int dev_pos,ishw,inf_pos=-1;
            parse_info_t pi;
            WCHAR filename[4096];
            char bufa[4096];

            cur_device->driver_index=heap_allocitem_i(&state->drivers_handle);
            cur_driver=&state->drivers_list[cur_device->driver_index];

            read_reg_val(hkey,state,L"DriverDesc",         &cur_driver->DriverDesc);
            read_reg_val(hkey,state,L"ProviderName",       &cur_driver->ProviderName);
            read_reg_val(hkey,state,L"DriverDate",         &cur_driver->DriverDate);
            read_reg_val(hkey,state,L"DriverVersion",      &cur_driver->DriverVersion);
            read_reg_val(hkey,state,L"MatchingDeviceId",   &cur_driver->MatchingDeviceId);

            read_reg_val(hkey,state,L"InfPath",            &cur_driver->InfPath);
            read_reg_val(hkey,state,L"InfSection",         &cur_driver->InfSection);
            read_reg_val(hkey,state,L"InfSectionExt",      &cur_driver->InfSectionExt);

            getdd(cur_device,state,&ishw,&dev_pos);

            if(cur_driver->InfPath)
            {
                FILE *f;
                char *buft;
                int len;
                int HWID_index;

                wsprintf(filename,L"%s%s",state->text+state->windir,state->text+cur_driver->InfPath);

                int isfound;
                sprintf(bufa,"%ws%ws",filename,state->text+cur_driver->MatchingDeviceId);
                infdata=(infdata_t *)hash_find(&inf_list,bufa,strlen(bufa),&isfound);
                if(flags&FLAG_FAILSAFE)
                {
                    inf_pos=0;
                }
                else
                if(isfound)
                {
                    //log("Skipped '%ws'\n",filename,inf_pos);
                    cur_driver->feature=infdata->feature;
                    cur_driver->catalogfile=infdata->catalogfile;
                    cur_driver->cat=infdata->cat;
                    inf_pos=infdata->inf_pos;
                }else
                {
                    //log("Looking for '%ws'\n",filename);
                    f=_wfopen(filename,L"rb");
                    if(!f)
                    {
                        log_err("ERROR: Not found '%ws'\n",filename);
                        goto sskp;
                    }
                    fseek(f,0,SEEK_END);
                    len=ftell(f);
                    if(len<0)len=0;
                    fseek(f,0,SEEK_SET);
                    buft=(char *)malloc(len);
                    fread(buft,len,1,f);
                    fclose(f);

                    wsprintf(buf,L"%ws",state->text+state->windir);
                    if(len>0)
                    driverpack_indexinf(&unpacked_drp,buf,(WCHAR *)(state->text+cur_driver->InfPath),buft,len);
                    free(buft);

                    char sect[BUFLEN];
                    sprintf(sect,"%ws%ws",state->text+cur_driver->InfSection,state->text+cur_driver->InfSectionExt);
                    sprintf(bufa,"%ws",state->text+cur_driver->MatchingDeviceId);
                    for(HWID_index=0;HWID_index<unpacked_drp.HWID_list_handle.items;HWID_index++)
                    if(!strcmpi(unpacked_drp.text+unpacked_drp.HWID_list[HWID_index].HWID,bufa))
                    {
                        hwidmatch_t hwidmatch;

                        hwidmatch_initbriefly(&hwidmatch,&unpacked_drp,HWID_index);
                        if(StrStrIA(sect,getdrp_drvinstall(&hwidmatch)))
                        {
                            cur_driver->feature=getdrp_drvfeature(&hwidmatch);
                            cur_driver->catalogfile=calc_catalogfile(&hwidmatch);
                            if(inf_pos<0||inf_pos>getdrp_drvinfpos(&hwidmatch))inf_pos=getdrp_drvinfpos(&hwidmatch);
                        }
                    }
                    if(inf_pos==-1)
                        log_err("ERROR: not found '%s'\n",sect);

                    cur_driver->cat=opencatfile(state,cur_driver);

                    //log("Added '%ws',%d\n",filename,inf_pos);
                    infdata=malloc(sizeof(infdata_t));
                    infdata->catalogfile=cur_driver->catalogfile;
                    infdata->feature=cur_driver->feature;
                    infdata->cat=cur_driver->cat;
                    infdata->inf_pos=inf_pos;
                    sprintf(bufa,"%ws%ws",filename,state->text+cur_driver->MatchingDeviceId);
                    hash_add(&inf_list,bufa,strlen(bufa),(int)infdata,HASH_MODE_INTACT);
                }
            }
            sskp:
            cur_driver->identifierscore=calc_identifierscore(dev_pos,ishw,inf_pos);
            //log("%d,%d,%d\n",dev_pos,ishw,inf_pos);

            pi.sb=bufa;
            sprintf(bufa,"%ws",state->text+cur_driver->DriverDate);
            pi.se=bufa+strlen(bufa);
            read_date(&pi,&cur_driver->version);
            sprintf(bufa,"%ws",state->text+cur_driver->DriverVersion);
            pi.se=bufa+strlen(bufa);
            read_version(&pi,&cur_driver->version);
        }
        i++;
    }
/*
    SP_DRVINFO_DATA drvinfo;
    drvinfo.cbSize=sizeof(SP_DRVINFO_DATA);
    SP_DRVINSTALL_PARAMS drvparams;
    drvparams.cbSize=sizeof(SP_DRVINSTALL_PARAMS);
    SetupDiBuildDriverInfoList(hDevInfo,DeviceInfoData,SPDIT_CLASSDRIVER);
    for (j=0;SetupDiEnumDriverInfo(hDevInfo,DeviceInfoData,SPDIT_CLASSDRIVER,j,&drvinfo);j++)
    {
        SYSTEMTIME t;
        SetupDiGetDriverInstallParams(hDevInfo,DeviceInfoData,&drvinfo,&drvparams);
        SP_DRVINFO_DETAIL_DATA drvdet;
        drvdet.cbSize=sizeof(SP_DRVINFO_DETAIL_DATA);
        SetupDiGetDriverInfoDetail(hDevInfo,DeviceInfoData,&drvinfo,&drvdet,4096,0);
    //Error(GetLastError());
        FileTimeToSystemTime(&drvinfo.DriverDate,&t);
    }*/
    //driverpack_print(&unpacked_drp);
    collection_free(&collection);
    hash_free(&inf_list);
    SetupDiDestroyDeviceInfoList(hDevInfo);
    time_devicescan=GetTickCount()-time_devicescan;
}
//}

//{ laptop/desktop detection
int GetMonitorDevice(WCHAR* adapterName,DISPLAY_DEVICE *ddMon)
{
    DWORD devMon = 0;

    while(EnumDisplayDevices(adapterName,devMon,ddMon,0))
    {
        if (ddMon->StateFlags&DISPLAY_DEVICE_ACTIVE&&
            ddMon->StateFlags&DISPLAY_DEVICE_ATTACHED) // for ATI, Windows XP
                break;
        devMon++;
    }
    return *ddMon->DeviceID!=0;
}

int GetMonitorSizeFromEDID(WCHAR* adapterName,int *Width,int *Height)
{
    DISPLAY_DEVICE ddMon;
    ZeroMemory(&ddMon,sizeof(ddMon));
    ddMon.cb=sizeof(ddMon);

    *Width=0;
    *Height=0;
    if(GetMonitorDevice(adapterName,&ddMon))
    {
        WCHAR model[18];
        WCHAR* s=wcschr(ddMon.DeviceID,L'\\')+1;
        size_t len=wcschr(s,L'\\')-s;
        wcsncpy(model,s,len);
        model[len]=0;

        WCHAR *path=wcschr(ddMon.DeviceID,L'\\')+1;
        WCHAR str[MAX_PATH]=L"SYSTEM\\CurrentControlSet\\Enum\\DISPLAY\\";
        wcsncat(str,path,wcschr(path, L'\\')-path);
        path=wcschr(path,L'\\')+1;
        HKEY hKey;
        if(RegOpenKeyEx(HKEY_LOCAL_MACHINE,str,0,KEY_READ,&hKey)==ERROR_SUCCESS)
        {
            DWORD i=0;
            DWORD size=MAX_PATH;
            FILETIME ft;
            while(RegEnumKeyEx(hKey,i,str,&size,NULL,NULL,NULL,&ft)==ERROR_SUCCESS)
            {
                HKEY hKey2;
                if(RegOpenKeyEx(hKey,str,0,KEY_READ,&hKey2)==ERROR_SUCCESS)
                {
                    size=MAX_PATH;
                    if(RegQueryValueEx(hKey2, L"Driver",NULL,NULL,(LPBYTE)&str,&size)==ERROR_SUCCESS)
                    {
                        if(wcscmp(str,path)==0)
                        {
                            HKEY hKey3;
                            if(RegOpenKeyEx(hKey2,L"Device Parameters",0,KEY_READ,&hKey3)==ERROR_SUCCESS)
                            {
                                BYTE EDID[256];
                                size=256;
                                if(RegQueryValueEx(hKey3,L"EDID",NULL,NULL,(LPBYTE)&EDID,&size)==ERROR_SUCCESS)
                                {
                                    DWORD p=8;
                                    WCHAR model2[9];

                                    char byte1=EDID[p];
                                    char byte2=EDID[p+1];
                                    model2[0]=((byte1&0x7C)>>2)+64;
                                    model2[1]=((byte1&3)<<3)+((byte2&0xE0)>>5)+64;
                                    model2[2]=(byte2&0x1F)+64;
                                    wsprintf(model2+3,L"%X%X%X%X",(EDID[p+3]&0xf0)>>4,EDID[p+3]&0xf,(EDID[p+2]&0xf0)>>4,EDID[p+2]&0x0f);
                                    if(wcscmp(model,model2)==0)
                                    {
                                        *Width=EDID[22];
                                        *Height=EDID[21];
                                        return 1;
                                    }
                                }
                                RegCloseKey(hKey3);
                            }
                        }
                    }
                    RegCloseKey(hKey2);
                }
                i++;
            }
            RegCloseKey(hKey);
        }
    }
    return 0;
}

int iswide(int x,int y)
{
    return ((double)y/x)>1.35?1:0;
}

/*
 1   ***, XX..14, 4:3         ->  desktop
 2   ***, 15..16, 4:3         ->  desktop
 3   ***, 17..XX, 4:3         ->  desktop
 4   ***, XX..14, Widescreen  ->  desktop
 5   ***, 15..16, Widescreen  ->  desktop
 6   ***, 17..XX, Widescreen  ->  desktop
 7   +/-, XX..14, 4:3         ->  assume desktop
 8   +/-, 15..16, 4:3         ->  desktop
 9   +/-, 17..XX, 4:3         ->  desktop
10   +/-, XX..14, Widescreen  ->  assume laptop
11   +/-, 15..18, Widescreen  ->  assume laptop
12   +/-, 18..XX, Widescreen  ->  assume desktop
*/
void isnotebook_a(state_t *state)
{
    int i;
    int x,y;
    int min_v=99,min_x=0,min_y=0;
    int diag;
    int batdev=0;
    WCHAR *buf;
    SYSTEM_POWER_STATUS *battery;
    device_t *cur_device;

    buf=(WCHAR *)(state->text+state->monitors);
    battery=(SYSTEM_POWER_STATUS *)(state->text+state->battery);

    if(state->ChassisType==3)
    {
        isLaptop=0;
        return;
    }
    if(state->ChassisType==10)
    {
        isLaptop=1;
        return;
    }

    for(i=0;i<buf[0];i++)
    {
        x=buf[1+i*2];
        y=buf[2+i*2];
        diag=sqrt(x*x+y*y)/2.54;

        if(diag<min_v||(diag==min_v&&iswide(x,y)))
        {
            min_v=diag;
            min_x=x;
            min_y=y;
        }
    }

    for(i=0;i<state->devices_handle.items;i++)
    {
        cur_device=&state->devices_list[i];
        WCHAR *p;
        char *s=state->text;

        if(cur_device->HardwareID)
        {
            p=(WCHAR *)(s+cur_device->HardwareID);
            while(*p)
            {
                if(StrStrI(p,L"*ACPI0003"))batdev=1;
                p+=lstrlen(p)+1;
            }
        }
    }

    if((battery->BatteryFlag&128)==0||batdev)
    {
        if(!buf[0])
            isLaptop=1;
        else if(iswide(min_x,min_y))
            isLaptop=min_v<=18?1:0;
        else
            isLaptop=0;
    }
    else
       isLaptop=0;
}
//}
