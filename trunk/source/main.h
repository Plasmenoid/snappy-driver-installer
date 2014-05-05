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

#define BUFLEN              4096

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
#include <locale.h>

#include "svnrev.h"
#include "resources.h"

#include "common.h"
#include "indexing.h"
#include "guicon.h"
#include "enum.h"
#include "matcher.h"
#include "manager.h"
#include "theme.h"
#include "install.h"
#include "draw.h"
#include "cli.h"

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
#define APPTITLE            L"Snappy Driver Installer v0.1"
#define VER_MARKER          "SDW"
#define VER_STATE           0x101
#define VER_INDEX           0x203

// Mode
#define STATEMODE_LOAD      2
#define STATEMODE_EXIT      3

// Popup window
#define FLOATING_NONE       0
#define FLOATING_TOOLTIP    1
#define FLOATING_SYSINFO    2
#define FLOATING_CMPDRIVER  3
#define FLOATING_DRIVERLST  4
#define FLOATING_ABOUT      5

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
#define ASPECT       16

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

#define ID_EMU_32          27
#define ID_EMU_64          28
#define ID_DEVICEMNG       29
#define ID_DIS_INSTALL     30
#define ID_DIS_RESTPNT     31

#define ID_WIN_2000        32
#define ID_WIN_XP          33
#define ID_WIN_VISTA       34
#define ID_WIN_7           35
#define ID_WIN_8           36
#define ID_WIN_81          37

#define ID_RESTPNT         38
#define ID_REBOOT          39

#define ID_HWID_CLIP      100
#define ID_HWID_WEB       200

#define MODE_NONE           0
#define MODE_INSTALLING     1
#define MODE_STOPPING       2
#define MODE_SCANNING       3

#define UF_DRPS          0x01
#define UF_DEVICES       0x02
//}

//{ Global variables

// Manager
extern manager_t *manager_g;
extern int volatile installmode;
extern int driverpackpath;
extern CRITICAL_SECTION sync;

// Window
extern HINSTANCE ghInst;
extern int main1x_c,main1y_c;
extern int mainx_c,mainy_c;
extern HFONT hFont;
extern HWND hPopup,hMain,hField;
extern panel_t panels[NUM_PANELS];

// Window helpers
extern int floating_type;
extern int floating_itembar;
extern int floating_x,floating_y;
extern int horiz_sh;
extern int ret_global;

// Settings
extern WCHAR drpext_dir[BUFLEN];
extern WCHAR data_dir  [BUFLEN];
extern WCHAR state_file[BUFLEN];
extern WCHAR finish_rb [BUFLEN];
extern int filters;
extern int flags;
extern int statemode;
extern int expertmode;
extern int virtual_os_version;
extern int virtual_arch_type;
//}

//{ Structs
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
int  settings_load(WCHAR *filename);
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
void redrawmainwnd();
void lang_refresh();
void theme_refresh();
void setscrollrange(int y);
int  getscrollpos();
void setscrollpos(int pos);

// Helpers
void get_resource(int id,void **data,int *size);
const WCHAR *get_winverstr(manager_t *manager);
void mkdir_r(const WCHAR *path);
void snapshot();
void extractto();

// Panel
void panel_setfilters(panel_t *panel);
int  panels_hitscan(int hx,int hy,int *ii);
int  panel_hitscan(panel_t *panel,int x,int y);
void panel_draw_inv(panel_t *panel);
void panel_draw(HDC hdc,panel_t *panel);

// GUI
void gui(int nCmd);
void checktimer(WCHAR *str,long long t,int uMsg);
LRESULT CALLBACK WndProcCommon(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
LRESULT CALLBACK WndProc(HWND,UINT,WPARAM,LPARAM);
LRESULT CALLBACK WindowGraphProcedure(HWND,UINT,WPARAM,LPARAM);
LRESULT CALLBACK PopupProcedure(HWND hwnd,UINT message,WPARAM wParam,LPARAM lParam);
BOOL CALLBACK LicenseProcedure(HWND hwnd,UINT Message,WPARAM wParam,LPARAM lParam);

//new
void set_rstpnt(int checked);
void drvdir();
WCHAR *getHWIDby(int id,int num);
void escapeAmpUrl(WCHAR *buf,WCHAR *source);
void escapeAmp(WCHAR *buf,WCHAR *source);
void contextmenu2(int x,int y);
void contextmenu(int x,int y);
