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
#define DRIVERPACK_TYPE_DIR             2

//{ Indexing strucures
typedef struct _driverpack_t
{
    ofst drppath;
    ofst drpfilename;
    int type;

    collection_t *col;
    FILE *fi;

    hashtable_t section_list;
    hashtable_t dup_list;
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

#define COLLECTION_FORCE_REINDEXING     1
#define COLLECTION_USE_LZMA             2
#define COLLECTION_PRINT_INDEX          4
#define COLLECTION_PRINT_LINEAR_INDEX   8
#define FLAG_NOGUI                     16
#define FLAG_NOSLOWSYSINFO             32
#define FLAG_DISABLEINSTALL            64
#define FLAG_AUTOINSTALL              128
#define FLAG_FAILSAFE                 256

typedef struct _collection_t
{
    WCHAR *driverpack_dir;
    WCHAR *index_bin_dir;
    WCHAR *index_linear_dir;
    int flags;

    driverpack_t *driverpack_list;
    heap_t driverpack_handle;
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

// Collection
void collection_init(collection_t *col,WCHAR *driverpacks_dir,WCHAR *index_bin_dir,WCHAR *index_linear_dir,int flags);
void collection_free(collection_t *col);
void collection_save(collection_t *col);
void collection_load(collection_t *col);
void collection_print(collection_t *col);
void collection_scanfolder(collection_t *col,const WCHAR *path);
int  collection_scanfolder_count(collection_t *col,const WCHAR *path);

// Driverpack
void driverpack_init(driverpack_t *drp,WCHAR const *driverpack_path,WCHAR const *driverpack_filename,collection_t *col);
void driverpack_free(driverpack_t *drp);
void driverpack_saveindex(driverpack_t *drp);
int  driverpack_loadindex(driverpack_t *drp);
void driverpack_getindexfilename(driverpack_t *drp,WCHAR *dir,const WCHAR *ext,WCHAR *indfile);
void driverpack_print(driverpack_t *drp);
void collection_printstates(collection_t *col);
void driverpack_genhashes(driverpack_t *drp);
unsigned int __stdcall thread_indexinf(void *arg);
void driverpack_indexinf_async(driverpack_t *drp,WCHAR const *pathinf,WCHAR const *inffile,char *adr,int len);
void driverpack_parsecat(driverpack_t *drp,WCHAR const *pathinf,WCHAR const *inffile,char *adr,int len);
int  driverpack_genindex(driverpack_t *drp);
void driverpack_indexinf1(driverpack_t *drp,WCHAR const *drpdir,WCHAR const *inffile,char *inf_base,int inf_len);
void driverpack_indexinf(driverpack_t *drp,WCHAR const *drpdir,WCHAR const *inffile,char *inf_base,int inf_len);
