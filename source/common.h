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

#define HASH_FLAG_KEYS_ARE_POINTERS 1
#define HASH_FLAG_STR_TO_LOWER 2
#define HASH_FLAG_STRS_ARE_INTS 4

extern int trap_mode;
typedef struct _hashtable_t hashtable_t;

enum
{
    HASH_MODE_INTACT,
    HASH_MODE_REPLACE,
    HASH_MODE_ADD
};

enum // heap_t
{
    ID_HASH_ITEMS=0,    // 0
    ID_HASH_STR,        // 1

    ID_STATE_DEVICES,   // 2
    ID_STATE_DRIVERS,   // 3
    ID_STATE_TEXT,      // 4

    ID_DRIVERPACK_inffile,      // 5
    ID_DRIVERPACK_manufacturer, // 6
    ID_DRIVERPACK_desc_list,    // 7
    ID_DRIVERPACK_HWID_list,    // 8
    ID_DRIVERPACK_text,         // 9
    ID_DRIVERPACK,          // 10

    ID_PARSE,          // 11
    ID_MATCHER,          // 12
    ID_MANAGER,         // 13
    NUM_HEAPS
};

enum // hash_t
{
    ID_MEMCPYZ_DUP=0,// 0

    ID_STRINGS,     // 1
    ID_SECTIONS,    // 2
    ID_INDEXES,     // 3
    ID_DUP_LIST,    // 4
    ID_INF_LIST,    // 5
    ID_CAT_LIST,    // 6
    ID_FILES,       // 7
    NUM_HASHES
};

//{ Structs
typedef struct _heap_t
{
    int id;
    void **membck;
    void *base;
    int used;
    int allocated;
    int itemsize;
    int items;

    hashtable_t *dup;
}heap_t;

typedef struct _find_t
{
    int findnext,findstrlen;
    const char *findstr;
}find_t;

typedef struct hashitem_type
{
    int string1;
    int key;
    int next;
    int strlen;
}hashitem_t;

typedef struct _hashtable_t
{
    int id;
    int flags;
    find_t finddata;
    int size;

    hashitem_t *items;
    heap_t items_handle;
    char *strs;
    heap_t strs_handle;

    int search_all,search_miss;
    int cmp_all,cmp_miss;
    int max_deep,used_1;
}hashtable_t;
//}

// Heap
void heap_refresh(heap_t *t);
void heap_expand(heap_t *t,int sz);
void heap_init(heap_t *t,int id,void **mem,int initsize,int itemsize);
void heap_free(heap_t *t);
void heap_reset(heap_t *t,int sz);

int  heap_alloc(heap_t *t,int sz);
int  heap_allocitem_i(heap_t *t);
void *heap_allocitem_ptr(heap_t *t);
void heap_freelastitem(heap_t *t);
int  heap_memcpy(heap_t *t,const void *mem,int sz);
int  heap_strcpy(heap_t *t,const char *s);
int  heap_memcpyz(heap_t *t,const void *mem,int sz);
int  heap_memcpyz_dup(heap_t *t,const void *mem,int sz);
int  heap_strtolowerz(heap_t *t,const char *s,int sz);
char *heap_save(heap_t *t,char *p);
char *heap_load(heap_t *t,char *p);

// Strings
void strsub(WCHAR *str,const WCHAR *pattern,const WCHAR *rep);
void strtoupper(char *s,int len);
void strtolower(char *s,int len);
char *strtolower_alloc(const char *s);
char *memcpy_alloc(const char *s,int sz);

// 7-zip
void registerall();
extern int  Extract7z(WCHAR *str);
extern void registercrc();
extern void register7z();
extern void registerBCJ();
extern void registerBCJ2();
extern void registerBranch();
extern void registerCopy();
extern void registerLZMA();
extern void registerLZMA2();
extern void registerPPMD();
extern void registerDelta();
extern void registerByteSwap();

// Hash
unsigned hash_getcode(const char *s,int sz);
inline char *getstr(hashtable_t *t,hashitem_t *cur);
void hash_init(hashtable_t *t,int id,int size,int flags);
void hash_clear(hashtable_t *t,int zero);
void hash_free(hashtable_t *t);
void hash_stats(hashtable_t *t);
char *hash_save(hashtable_t *t,char *p);
char *hash_load(hashtable_t *t,char *p);
void hash_add(hashtable_t *t,const char *s,int s_sz,int key,int mode);
int  hash_find(hashtable_t *t,char *s,int sz,int *isfound);
int  hash_find_str(hashtable_t *t, char *s);
int  hash_findnext_rec(hashtable_t *t,const char *s);
int  hash_findnext_b(hashtable_t *t,int *isfound);
int  hash_findnext(hashtable_t *t);
