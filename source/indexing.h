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

#define STR_LN 4096
#define ofst int
//#define MERGE_FINDER

typedef struct _driverpack_t driverpack_t;
typedef struct _data_inffile_t data_inffile_t;
typedef struct _data_manufacturer_t data_manufacturer_t;
typedef struct _data_desc_t data_desc_t;
typedef struct _data_HWID_t data_HWID_t;

typedef struct _collection_t collection_t;

enum
{
    ClassGuid,
    Class,
    Provider,
    CatalogFile,
    CatalogFile_nt,
    CatalogFile_ntx86,
    CatalogFile_ntia64,
    CatalogFile_ntamd64,
    DriverVer,
    DriverPackageDisplayName,
    DriverPackageType,
    NUM_VER_NAMES
};

//{ Misc
#define LSTCNT 1000
typedef struct _inflist_t
{
    driverpack_t *drp;
    WCHAR pathinf[4096];
    WCHAR inffile[4096];
    char *adr;
    int len;

    HANDLE dataready;
    HANDLE slotvacant;
    int type;
}inflist_t;

typedef struct _filedata_t
{
    int size;
    unsigned crc;
    WCHAR *str;
}filedata_t;

typedef struct _sect_data_t
{
    int ofs,len;
}sect_data_t;

typedef struct _tbl_t
{
    const char *s;
    int sz;
}tbl_t;

typedef struct _parse_info_t
{
    char *start;
    char *strend;
    char *sb;
    char *se;

    heap_t strings;
    char *text;

    driverpack_t *pack;
}parse_info_t;

typedef struct _version_t
{
    int d,m,y;
    int v1,v2,v3,v4;
}version_t;
//}

#define DRIVERPACK_TYPE_PENDING_SAVE    0
#define DRIVERPACK_TYPE_INDEXED         1
#define DRIVERPACK_TYPE_UPDATE          2
#define DRIVERPACK_TYPE_EMPTY           3

//{ Indexing strucures
typedef struct _driverpack_t
{
    ofst drppath;
    ofst drpfilename;
    int type;

    collection_t *col;

    hashtable_t section_list;
    hashtable_t string_list;
    hashtable_t indexes;
    hashtable_t cat_list;

    data_inffile_t *inffile;
    heap_t inffile_handle;

    data_manufacturer_t *manufacturer_list;
    heap_t manufacturer_handle;

    data_desc_t *desc_list;
    heap_t desc_list_handle;

    data_HWID_t *HWID_list;
    heap_t HWID_list_handle;

    char *text;
    heap_t text_handle;
}driverpack_t;

typedef struct _data_inffile_t // 80
{
    ofst infpath;
    ofst inffilename;
    ofst fields[NUM_VER_NAMES];
    ofst cats[NUM_VER_NAMES];
    version_t version;
    int infsize;
    int infcrc;
}data_inffile_t;

typedef struct _data_manufacturer_t // 16
{
    int inffile_index;

    ofst manufacturer;
    ofst sections;
    int sections_n;
}data_manufacturer_t;

typedef struct _data_desc_t // 12+1
{
    int manufacturer_index;
    int sect_pos;

    ofst desc;
    ofst install;
    ofst install_picked;
    char feature;
}data_desc_t;

typedef struct _data_HWID_t // 8
{
    int desc_index;
    short inf_pos;

    ofst HWID;
}data_HWID_t;

#define COLLECTION_FORCE_REINDEXING  0x00000001
#define COLLECTION_USE_LZMA          0x00000002
#define COLLECTION_PRINT_INDEX       0x00000004
#define FLAG_NOGUI                   0x00000010
//#define FLAG_NOSLOWSYSINFO           0x00000020
#define FLAG_DISABLEINSTALL          0x00000040
#define FLAG_AUTOINSTALL             0x00000080
#define FLAG_FAILSAFE                0x00000100
#define FLAG_AUTOCLOSE               0x00000200
#define FLAG_NORESTOREPOINT          0x00000400
#define FLAG_NOLOGFILE               0x00000800
#define FLAG_NOSNAPSHOT              0x00001000
#define FLAG_NOSTAMP                 0x00002000
#define FLAG_NOFEATURESCORE          0x00004000
#define FLAG_PRESERVECFG             0x00008000

#define FLAG_EXTRACTONLY             0x00010000
#define FLAG_KEEPUNPACKINDEX         0x00020000
#define FLAG_KEEPTEMPFILES           0x00040000
#define FLAG_SHOWDRPNAMES            0x00080000
#define FLAG_DPINSTMODE              0x00100000
#define FLAG_SHOWCONSOLE             0x00200000
#define FLAG_DELEXTRAINFS            0x00400000

typedef struct _collection_t
{
    WCHAR *driverpack_dir;
    WCHAR *index_bin_dir;
    WCHAR *index_linear_dir;
    int flags;

    driverpack_t *driverpack_list;
    heap_t driverpack_handle;

    inflist_t *inflist;
    int pos_in,pos_out;
}collection_t;
//}

// Parse
void read_whitespace(parse_info_t *parse_info);
int  read_item(parse_info_t *parse_info);
int  read_field(parse_info_t *parse_info);
int  read_number(char **en,char *v1e);
int  read_hex(parse_info_t *parse_info);
int  read_date(parse_info_t *parse_info,version_t *t);
void read_version(parse_info_t *parse_info,version_t *t);

void parse_init(parse_info_t *parse_info,driverpack_t *drp);
void parse_free(parse_info_t *parse_info);
void parse_set(parse_info_t *parse_info,char *inf_base,sect_data_t *lnk);
void parse_getstr(parse_info_t *parse_info,char **vb,char **ve);
void str_sub(parse_info_t *parse_info);

// Misc
int  unicode2ansi(char *s,char *out,int size);
void extracttest();
int  encode(char *dest,int dest_sz,char *src,int src_sz);
int  decode(char *dest,int dest_sz,char *src,int src_sz);
int  checkfolders(WCHAR *folder1,WCHAR *folder2,hashtable_t *filename2path,hashtable_t *path2filename,int sub);
void hash_clearfiles(hashtable_t *t);
WCHAR *finddrp(WCHAR *s);

// Collection
void collection_init(collection_t *col,WCHAR *driverpacks_dir,WCHAR *index_bin_dir,WCHAR *index_linear_dir,int flags);
void collection_free(collection_t *col);
void collection_save(collection_t *col);
void collection_updatedindexes(collection_t *col);
void collection_load(collection_t *col);
void collection_print(collection_t *col);
WCHAR *collection_finddrp(collection_t *col,WCHAR *s);
void collection_scanfolder(collection_t *col,const WCHAR *path);
int  collection_scanfolder_count(collection_t *col,const WCHAR *path);

// Driverpack
void driverpack_init(driverpack_t *drp,WCHAR const *driverpack_path,WCHAR const *driverpack_filename,collection_t *col);
void driverpack_free(driverpack_t *drp);
void driverpack_saveindex(driverpack_t *drp);
int  driverpack_checkindex(driverpack_t *drp);
int  driverpack_loadindex(driverpack_t *drp);
void driverpack_getindexfilename(driverpack_t *drp,WCHAR *dir,const WCHAR *ext,WCHAR *indfile);
void driverpack_print(driverpack_t *drp);
void collection_printstates(collection_t *col);
void driverpack_genhashes(driverpack_t *drp);
unsigned int __stdcall thread_indexinf(void *arg);
void driverpack_indexinf_async(driverpack_t *drp,collection_t *col,WCHAR const *pathinf,WCHAR const *inffile,char *adr,int len);
void driverpack_parsecat(driverpack_t *drp,WCHAR const *pathinf,WCHAR const *inffile,char *adr,int len);
int  driverpack_genindex(driverpack_t *drp);
void driverpack_indexinf_ansi(driverpack_t *drp,WCHAR const *drpdir,WCHAR const *inffile,char *inf_base,int inf_len);
void driverpack_indexinf(driverpack_t *drp,WCHAR const *drpdir,WCHAR const *inffile,char *inf_base,int inf_len);
