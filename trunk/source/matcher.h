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

#define NUM_DECS 11*4
#define NUM_MARKERS 21

#define STATUS_BETTER       0x001
#define STATUS_SAME         0x002
#define STATUS_WORSE        0x004
#define STATUS_INVALID      0x008

#define STATUS_MISSING      0x010
#define STATUS_NEW          0x020
#define STATUS_CURRENT      0x040
#define STATUS_OLD          0x080

#define STATUS_NF_MISSING   0x100
#define STATUS_NF_UNKNOWN   0x200
#define STATUS_NF_STANDARD  0x400
#define STATUS_DUP          0x800

extern const char *nts[NUM_DECS];

typedef struct _markers_t
{
    char *name;
    int major,minor,arch;
}markers_t;

typedef struct _devicematch_t
{
    device_t *device;
    driver_t *driver;
    int start_matches;
    int num_matches;
    int status;
}devicematch_t;

typedef struct _hwidmatch_t
{
    driverpack_t *drp;
    int HWID_index;

    devicematch_t *devicematch;
    int identifierscore,decorscore,markerscore,altsectscore,status;
    unsigned score;
}hwidmatch_t;

typedef struct _matcher_t
{
    state_t *state;
    collection_t *col;

    devicematch_t *devicematch_list;
    heap_t devicematch_handle;

    hwidmatch_t *hwidmatch_list;
    heap_t hwidmatch_handle;
}matcher_t;

// Calc
void genmarker(state_t *state);
int isMissing(device_t *device,driver_t *driver,state_t *state);
int calc_identifierscore(int dev_pos,int dev_ishw,int inf_pos);
int calc_catalogfile(hwidmatch_t *hwidmatch);
int calc_signature(int catalogfile,state_t *state,int isnt);
unsigned calc_score(int catalogfile,int feature,int rank,state_t *state,int isnt);
unsigned calc_score_h(driver_t *driver,state_t *state);
int calc_secttype(const char *s);
int calc_decorscore(int id,state_t *state);
int calc_markerscore(state_t *state,char *path);
intptr_t isvalid_usb30hub(hwidmatch_t *hwidmatch,state_t *state,WCHAR *str);
int isvalid_ver(hwidmatch_t *hwidmatch,state_t *state);
int calc_notebook(hwidmatch_t *hwidmatch);
int calc_altsectscore(hwidmatch_t *hwidmatch,state_t *state,int curscore);
int calc_status(hwidmatch_t *hwidmatch,state_t *state);

// Misc
void findHWID_in_list(char *s,int list,int str,int *dev_pos);
void getdd(device_t *cur_device,state_t *state,int *ishw,int *dev_pos);
int  cmpunsigned(unsigned a,unsigned b);
int  cmpdate(version_t *t1,version_t *t2);
int  cmpversion(version_t *t1,version_t *t2);
void devicematch_init(devicematch_t *devicematch,device_t *cur_device,driver_t *driver,int items);

// hwidmatch
void minlen(CHAR *s,int *len);
void hwidmatch_init(hwidmatch_t *hwidmatch,driverpack_t *drp,int HWID_index,int dev_pos,int ishw,state_t *state,devicematch_t *devicematch);
void hwidmatch_initbriefly(hwidmatch_t *hwidmatch,driverpack_t *drp,int HWID_index);
void hwidmatch_calclen(hwidmatch_t *hwidmatch,int *limits);
void hwidmatch_print(hwidmatch_t *hwidmatch,int *limits);
int  hwidmatch_cmp(hwidmatch_t *match1,hwidmatch_t *match2);

// Matcher
void matcher_init(matcher_t *matcher,state_t *state,collection_t *col);
void matcher_free(matcher_t *matcher);
void matcher_findHWIDs(matcher_t *matcher,devicematch_t *device_match,char *hwid,int dev_pos,int ishw);
void matcher_populate(matcher_t *matcher);
void matcher_sort(matcher_t *matcher);
void matcher_print(matcher_t *matcher);

//driverpack
WCHAR *getdrp_packpath(hwidmatch_t *hwidmatch);
WCHAR *getdrp_packname(hwidmatch_t *hwidmatch);
int   getdrp_packontorrent(hwidmatch_t *hwidmatch);
//inffile
char *getdrp_infpath(hwidmatch_t *hwidmatch);
char *getdrp_infname(hwidmatch_t *hwidmatch);
char *getdrp_drvfield(hwidmatch_t *hwidmatch,int n);
char *getdrp_drvcat(hwidmatch_t *hwidmatch,int n);
version_t *getdrp_drvversion(hwidmatch_t *hwidmatch);
int   getdrp_infsize(hwidmatch_t *hwidmatch);
int   getdrp_infcrc(hwidmatch_t *hwidmatch);
//manufacturer
char *getdrp_drvmanufacturer(hwidmatch_t *hwidmatch);
void  getdrp_drvsectionAtPos(driverpack_t *drp,char *buf,int pos,int manuf_index);
void  getdrp_drvsection(hwidmatch_t *hwidmatch,char *buf);
//desc
char *getdrp_drvdesc(hwidmatch_t *hwidmatch);
char *getdrp_drvinstall(hwidmatch_t *hwidmatch);
char *getdrp_drvinstallPicked(hwidmatch_t *hwidmatch);
int   getdrp_drvfeature(hwidmatch_t *hwidmatch);
//HWID
short getdrp_drvinfpos(hwidmatch_t *hwidmatch);
char *getdrp_drvHWID(hwidmatch_t *hwidmatch);
