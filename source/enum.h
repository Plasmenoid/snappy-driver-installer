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

//{ Global variables
extern int isLaptop;
//}

//{ enum structures

#define DISPLAY_DEVICE_ACTIVE         1
#define DISPLAY_DEVICE_ATTACHED       2

//#define STORE_PROPS
#ifdef STORE_PROPS
enum
{
/**/PROP_DEVICEDESC=0,
/**/PROP_HARDWAREID,
/**/PROP_COMPATIBLEIDS,
//   PROP_UNUSED0,           3
    PROP_SERVICE,
//   PROP_UNUSED1,           5
//   PROP_UNUSED2,           6
    PROP_CLASS,
    PROP_CLASSGUID,
/**/PROP_DRIVER,
    PROP_CONFIGFLAGS,
/**/PROP_MFG,
/**/PROP_FRIENDLYNAME,
    PROP_LOCATION_INFORMATION,
    PROP_PHYSICAL_DEVICE_OBJECT_NAME,
    PROP_CAPABILITIES,
    PROP_UI_NUMBER,
//   PROP_UPPERFILTERS,      17
//   PROP_LOWERFILTERS,      18
    PROP_BUSTYPEGUID,
    PROP_LEGACYBUSTYPE,
    PROP_BUSNUMBER,
    PROP_ENUMERATOR_NAME,
//   PROP_SECURITY,          23
//   PROP_SECURITY_SDS,      24
//   PROP_DEVTYPE,           25
//   PROP_EXCLUSIVE,         26
//   PROP_CHARACTERISTICS,   27
    PROP_ADDRESS,
//                           29
    PROP_UI_NUMBER_DESC_FORMAT,
    NUM_PROPS
};
#endif // STORE_PROPS

typedef struct _infdata_t
{
    int catalogfile;
    int feature;
    int inf_pos;
}infdata_t;

typedef struct _device_t
{
    int driver_index;

    ofst Devicedesc;
    ofst HardwareID;
    ofst CompatibleIDs;
    ofst Driver;
    ofst Mfg;
    ofst FriendlyName;
    int Capabilities;
    int ConfigFlags;

    ofst InstanceId;
    ULONG status,problem;
    int ret;

#ifdef STORE_PROPS
    ofst Properties[NUM_PROPS];
    int  PropertiesSize[NUM_PROPS];
    int  PropertiesType[NUM_PROPS];
#endif

    SP_DEVINFO_DATA DeviceInfoData;     // ClassGuid,DevInst
}device_t;

typedef struct _driver_t driver_t;
typedef struct _driver_t
{
    ofst DriverDesc;
    ofst ProviderName;
    ofst DriverDate;
    ofst DriverVersion;
    ofst MatchingDeviceId;
    ofst InfPath;
    ofst InfSection;
    ofst InfSectionExt;
    version_t version;

    int catalogfile;
    int feature;
    int identifierscore;

    //SP_DRVINSTALL_PARAMS drvparams;
    //SP_DRVINFO_DETAIL_DATA drvdet;
}driver_t;

typedef struct _state_m_t
{
    OSVERSIONINFOEX platform;
    int locale;
    int architecture;

    ofst manuf;
    ofst model;
    ofst product;
    ofst monitors;
    ofst battery;

    ofst windir;
    ofst temp;

    driverpack_t windirinf;
}state_m_t;

typedef struct _state_t
{
    OSVERSIONINFOEX platform;
    int locale;
    int architecture;

    ofst manuf;
    ofst model;
    ofst product;
    ofst monitors;
    ofst battery;

    ofst windir;
    ofst temp;

    driverpack_t windirinf;

/*    ofst *profile_list;
    heap_t profile_handle;
    int profile_current;*/

    device_t *devices_list;
    heap_t drivers_handle;

    driver_t *drivers_list;
    heap_t devices_handle;

    char *text;
    heap_t text_handle;
}state_t;
//}

extern const char *deviceststus_str[];
void print_guid(GUID *g);
int print_status(device_t *device);
void print_appinfo();

void read_device_property(HDEVINFO hDevInfo,SP_DEVINFO_DATA *DeviceInfoData,state_t *state,int id,ofst *val);
void read_reg_val(HKEY hkey,state_t *state,const WCHAR *key,ofst *val);

void device_print(device_t *cur_device,state_t *state);
void device_printHWIDS(device_t *cur_device,state_t *state);
void driver_print(driver_t *cur_driver,state_t *state);

void state_init(state_t *state);
void state_free(state_t *state);
void state_save(state_t *state,const WCHAR *filename);
int  state_load(state_t *state,const WCHAR *filename);
void state_fakeOSversion(state_t *state);
void log_add(CHAR const *format,...);
void state_print(state_t *state);
void state_getsysinfo_fast(state_t *state);
void state_getsysinfo_slow(state_t *state);
void state_scandevices(state_t *state);

int GetMonitorDevice(WCHAR* adapterName,DISPLAY_DEVICE *ddMon);
int GetMonitorSizeFromEDID(WCHAR* adapterName,int *Width,int *Height);
int iswide(int x,int y);
void isnotebook_a(state_t *state);
