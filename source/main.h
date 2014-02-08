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

//{ Includes
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commdlg.h>
#include <setupapi.h>
#include <ddk\cfgmgr32.h>
#include <shlwapi.h>
#include <shellapi.h>
#include <shlobj.h>
#include <ddk\newdev.h>
#include <winerror.h>
#include <webp\decode.h>
#include "SRRestorePtAPI.h"

#include <stdio.h>
#include <time.h>
_CRTIMP double __cdecl sqrt (double);   //#include <math.h>
typedef WINBOOL (__cdecl *MYPROC)(PRESTOREPOINTINFOW pRestorePtSpec,PSTATEMGRSTATUS pSMgrStatus);
#include <errno.h>
#include <ctype.h>
#include <signal.h>
#include <process.h>
#include <direct.h>

#include "svnrev.h"
#include "resources.h"

#include "common.h"
#include "indexing.h"
#include "guicon.h"
#include "enum.h"
#include "matcher.h"
#include "manager.h"
#include "theme.h"

#include "7z.h"
#include "7zAlloc.h"
#include "7zCrc.h"
#include "7zFile.h"
#include "7zVersion.h"
#include "LzmaEnc.h"
#include "Lzma86.h"
//}

//{ Defines

// Misc
//#define CONSOLE_MODE
#define APPTITLE            L"Snappy Driver Installer v0.1a"
#define VER_MARKER          "SDW"
#define VER_STATE           0x101
#define VER_INDEX           0x202
#define BUFLEN              4096

// Mode
#define STATEMODE_SAVE      1
#define STATEMODE_LOAD      2
#define STATEMODE_7Z        3

// Popup window
#define FLOATING_NONE    0
#define FLOATING_TOOLTIP 1
#define FLOATING_SYSINFO 2
#define FLOATING_DRIVER  3
#define FLOATING_ABOUT   4

// Extract info
#define INSTALLDRIVERS      1
#define OPENFOLDER          2

// Left panel types
#define TYPE_GROUP          1
#define TYPE_TEXT           2
#define TYPE_CHECKBOX       3
#define TYPE_BUTTON         4
#define TYPE_GROUP_BREAK    5

// Align
#define ALIGN_RIGHT   1
#define ALIGN_BOTTOM  2
#define ALIGN_HCENTER 4
#define ALIGN_VCENTER 8

// fill mode
#define HTILE         1
#define VTILE         2
#define HSTR          4
#define VSTR          8

// Messages
#define WM_BUNDLEREADY      WM_APP+1
#define WM_UPDATELANG       WM_APP+2
#define WM_UPDATETHEME      WM_APP+3

// Left panel IDs
#define ID_SHOW_MISSING     1
#define ID_SHOW_NEWER       2
#define ID_SHOW_CURRENT     3
#define ID_SHOW_OLD         4
#define ID_SHOW_BETTER      5
#define ID_SHOW_WORSE_RANK  6

#define ID_SHOW_NF_MISSING  7
#define ID_SHOW_NF_UNKNOWN  8
#define ID_SHOW_NF_STANDARD 9

#define ID_SHOW_ONE        10
#define ID_SHOW_DUP        11
#define ID_SHOW_INVALID    12

#define ID_INSTALL         13
#define ID_SELECT_ALL      14
#define ID_SELECT_NONE     15
#define ID_EXPERT_MODE     16

#define ID_LANG            17
#define ID_THEME           18

#define ID_OPENLOGS        19
#define ID_SNAPSHOT        20
#define ID_EXTRACT         21
#define ID_DRVDIR          22

#define ID_SCHEDULE        23
#define ID_SHOWALT         24
#define ID_OPENINF         25
#define ID_LOCATEINF       26
#define ID_HWID_CLIP      100
#define ID_HWID_WEB       200

#define PANELITEMS_NUM     37
//}

//{ Global variables
extern int virtual_os_version;
extern int virtual_arch_type;
extern int mainx_c,mainy_c;

extern HFONT hFont;
extern HWND hPopup,hMain,hField;
extern int floating_x,floating_y;
extern int horiz_sh;

extern manager_t *manager_g;
extern int volatile installmode;
extern WCHAR state_file[BUFLEN];
extern WCHAR drpext_dir[BUFLEN];
extern int flags;
extern int statemode;
//}

//{ Structs
typedef struct _panelitem_t
{
    int type;
    int str_id;
    int action_id;
    int checked;
}panelitem_t;

typedef struct _canvas_t
{
    int x,y;
    HDC hdcMem,localDC;
    HBITMAP bitmap,oldbitmap;
    HRGN clipping;
    PAINTSTRUCT ps;
    HWND hwnd;
}canvas_t;
extern canvas_t canvasField;

typedef struct _bundle_t
{
    state_t state;
    collection_t collection;
    matcher_t matcher;
}bundle_t;
//}

// Main
void settings_parse(const WCHAR *str,int ind);
void settings_save();
void settings_load();
void SignalHandler(int signum);
void CALLBACK drp_callback(LPTSTR szFile,DWORD action,LPARAM lParam);
//int WINAPI WinMain(HINSTANCE hInst,HINSTANCE hinst,LPSTR pStr,int nCmd);

// Threads
unsigned int __stdcall thread_scandevices(void *arg);
unsigned int __stdcall thread_loadindexes(void *arg);
unsigned int __stdcall thread_loadall(void *arg);

// Bundle
void bundle_init(bundle_t *bundle);
void bundle_prep(bundle_t *bundle);
void bundle_free(bundle_t *bundle);
void bundle_load(bundle_t *bundle);
void bundle_lowprioirity(bundle_t *bundle);

// Windows
HWND CreateWindowM(const WCHAR *type,const WCHAR *name,HWND hwnd,int id);
HWND CreateWindowMF(const WCHAR *type,const WCHAR *name,HWND hwnd,int id,int f);
void setfont();
void redrawfield();
void lang_refresh();
void theme_refresh();
void setscrollrange(int y);
int  getscrollpos();
void setscrollpos(int pos);

// Helpers
void get_resource(int id,void **data,int *size);
WCHAR *get_winverstr(manager_t *manager);
void mkdir_r(const WCHAR *path);
void snapshot();
void extractto();

// Draw
void image_load(img_t *img,BYTE *data,int sz);
void image_loadFile(img_t *img,WCHAR *filename);
void image_loadRes(img_t *img,int id);
void image_draw(HDC dc,img_t *img,int x1,int y1,int x2,int y2,int anchor,int fill);
void box_draw(HDC hdc,int x1,int y1,int x2,int y2,int i);
void drawcheckbox(HDC hdc,int x,int y,int wx,int wy,int checked,int active);
void drawrect(HDC hdc,int x1,int y1,int x2,int y2,int color1,int color2,int w,int r);
void drawrevision(HDC hdcMem,int y);
void drawpopup(int itembar,WCHAR *str,int i,int type,int x,int y,HWND hwnd);

// Canvas
void canvas_init(canvas_t *canvas);
void canvas_free(canvas_t *canvas);
void canvas_begin(canvas_t *canvas,HWND hwnd,int x,int y);
void canvas_end(canvas_t *canvas);

// Panel
int  panel_hitscan(int x,int y);
void panel_draw(HDC hdc);

// GUI
void gui(int nCmd);
LRESULT CALLBACK WndProcCommon(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
LRESULT CALLBACK WndProc(HWND,UINT,WPARAM,LPARAM);
LRESULT CALLBACK WindowGraphProcedure(HWND,UINT,WPARAM,LPARAM);
LRESULT CALLBACK PopupProcedure(HWND hwnd,UINT message,WPARAM wParam,LPARAM lParam);
BOOL CALLBACK LicenseProcedure(HWND hwnd,UINT Message,WPARAM wParam,LPARAM lParam);

//new
void drvdir();
WCHAR *getHWIDby(int id,int num);
void escapeAmpUrl(WCHAR *buf,WCHAR *source);
void escapeAmp(WCHAR *buf,WCHAR *source);
void contextmenu(int x,int y);
