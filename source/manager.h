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

// Filters
#define FILTER_SHOW_MISSING     (1<<ID_SHOW_MISSING)
#define FILTER_SHOW_NEWER       (1<<ID_SHOW_NEWER)
#define FILTER_SHOW_CURRENT     (1<<ID_SHOW_CURRENT)
#define FILTER_SHOW_OLD         (1<<ID_SHOW_OLD)
#define FILTER_SHOW_BETTER      (1<<ID_SHOW_BETTER)
#define FILTER_SHOW_WORSE_RANK  (1<<ID_SHOW_WORSE_RANK)

#define FILTER_SHOW_NF_MISSING  (1<<ID_SHOW_NF_MISSING)
#define FILTER_SHOW_NF_UNKNOWN  (1<<ID_SHOW_NF_UNKNOWN)
#define FILTER_SHOW_NF_STANDARD (1<<ID_SHOW_NF_STANDARD)

#define FILTER_SHOW_ONE         (1<<ID_SHOW_ONE)
#define FILTER_SHOW_DUP         (1<<ID_SHOW_DUP)
#define FILTER_SHOW_INVALID     (1<<ID_SHOW_INVALID)

// Itembar slots
#define SLOT_EMPTY          0
#define SLOT_VIRUS_AUTORUN  1
#define SLOT_VIRUS_RECYCLER 2
#define SLOT_VIRUS_HIDDEN   3
#define SLOT_INFO           4
#define SLOT_DPRDIR         5
#define SLOT_SNAPSHOT       6
#define SLOT_INDEXING       7
#define SLOT_EXTRACTING     8
#define SLOT_RESTORE_POINT  9
#define RES_SLOTS          10

#define NUM_STATUS 6
typedef struct _status_t
{
    int filter,status;
}status_t;


typedef struct _itembar_t
{
    devicematch_t *devicematch;
    hwidmatch_t *hwidmatch;

    WCHAR txt1[1024];
    int install_status;
    int val1,val2;
    int percent;

    int isactive;
    int checked;
    int index;

    int oldpos,curpos,tagpos,accel;
}itembar_t;

typedef struct _manager_t
{
    matcher_t *matcher;

    itembar_t *items_list;
    heap_t items_handle;

    long animstart;
}manager_t;

typedef struct _textdata_t
{
    HDC hdcMem;
    int x;
    int y;
    int wy;
    int maxsz;
    int col;
    int i;
    int *limits;
    int mode;
}textdata_t;

//{ Global vars
extern WCHAR extractdir[4096];
//}

// Manager
void manager_init(manager_t *manager,matcher_t *matcher);
void manager_free(manager_t *manager);
void manager_populate(manager_t *manager);
void manager_filter(manager_t *manager,int options);
void manager_print(manager_t *manager);

// User interaction
void manager_hitscan(manager_t *manager,int x,int y, int *i,int *zone);
void manager_install(int flags);
void manager_clear(manager_t *manager);
void manager_testitembars(manager_t *manager);
void manager_toggle(manager_t *manager,int index);
void manager_expand(manager_t *manager,int index);
void manager_selectnone(manager_t *manager);
void manager_selectall(manager_t *manager);

// Helpers
int groupsize(manager_t *manager,int index);
void itembar_init(itembar_t *item,devicematch_t *devicematch,hwidmatch_t *match,int groupindex);
void itembar_settext(manager_t *manager,int i,WCHAR *txt1,int percent);
void itembar_setpos(itembar_t *itembar,int *pos,int *cnt);
int  itembar_cmp(itembar_t *a,itembar_t *b,CHAR *ta,CHAR *tb);
int  isdrivervalid(hwidmatch_t *hwidmatch);
void str_status(WCHAR *buf,itembar_t *itembar);
int box_status(int index);
void str_date(version_t *v,WCHAR *buf);
WCHAR *str_version(version_t *ver);

// Driver list
void manager_setpos(manager_t *manager);
int  manager_animate(manager_t *manager);
int  manager_drawitem(manager_t *manager,HDC hdc,int index,int ofsy,int zone);
int  isbehind(manager_t *manager,int pos,int ofs,int j);
void manager_draw(manager_t *manager,HDC hdc,int ofsy);
void manager_restorepos(manager_t *manager,manager_t *manager_prev);

// Draw
void TextOut_CM(HDC hdcMem,int x,int y,WCHAR *str,int color,int *maxsz,int mode);
void TextOutP(textdata_t *td,WCHAR *format,...);
void TextOutF(textdata_t *td,int col,WCHAR *format,...);
void TextOutSF(textdata_t *td,WCHAR *str,WCHAR *format,...);

// Popup
void popup_resize(int x,int y);
void popup_driverline(hwidmatch_t *hwidmatch,int *limits,HDC hdcMem,int ln,int mode,int index);
void popup_driverlist(manager_t *manager,HDC hdcMem,RECT rect,int i);
int  pickcat(hwidmatch_t *hwidmatch,state_t *state);
int  isvalidcat(hwidmatch_t *hwidmatch,state_t *state);
void popup_drivercmp(manager_t *manager,HDC hdcMem,RECT rect,int i);
void popup_about(HDC hdcMem);
void popup_sysinfo(manager_t *manager,HDC hdcMem);
