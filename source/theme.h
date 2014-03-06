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

#include "themelist.h"
#include "langlist.h"

// Image
typedef struct _img_t
{
    BYTE *big;
    HBITMAP bitmap;
    HDC dc;
    int sx,sy;
    int index;
    int iscopy;
}img_t;
extern const int boxindex[BOX_NUM];
extern img_t box[BOX_NUM];
extern img_t icon[ICON_NUM];
extern WCHAR themelist[64][250];

// Theme/lang
typedef struct _entry_t
{
    WCHAR *name;
    int val;
    int init;
}entry_t;
extern entry_t language[STR_NM];
#define STR(A) (language[A].val?(WCHAR *)language[A].val:L"")
extern entry_t theme[THEME_NM];
#define D(A) theme[A].val

// Monitor
typedef void (CALLBACK *FileChangeCallback)(LPTSTR,DWORD,LPARAM);
typedef struct _monitor_t
{
	OVERLAPPED ol;
	HANDLE     hDir;
	BYTE       buffer[32*1024];
	LPARAM     lParam;
	DWORD      notifyFilter;
	BOOL       fStop;
	int        subdirs;
	FileChangeCallback callback;
}*monitor_t;

// Image
void box_init(img_t *img,int i);
void box_free(img_t *img);
void icon_init(img_t *img,int i);
void icon_free(img_t *img);

// Vault
void vault_startmonitors();
void vault_stopmonitors();
void vault_init();
void vault_free();
void *vault_loadfile(const WCHAR *filename,int *sz);
int  vault_findvar(hashtable_t *t,WCHAR *str);
int  vault_readvalue(const WCHAR *str);
int  vault_findstr(WCHAR *str);
void vault_parse(WCHAR *data,entry_t *entry,hashtable_t *tbl,WCHAR **origdata);
void vault_loadfromfile(WCHAR *filename,entry_t *entry,hashtable_t *tbl,WCHAR **origdata);
void vault_loadfromres(int id,entry_t *entry,hashtable_t *tbl,int num,WCHAR **origdata);

// Lang/theme
void lang_enum(HWND hwnd,WCHAR *path,int locale);
void theme_enum(HWND hwnd,WCHAR *path);
void lang_set(int i);
void theme_set(int i);
void lang_load(WCHAR *filename);
void theme_load(WCHAR *filename);
void CALLBACK lang_callback(LPTSTR szFile,DWORD action,LPARAM lParam);
void CALLBACK theme_callback(LPTSTR szFile,DWORD action,LPARAM lParam);

// Monitor
monitor_t      monitor_start(LPCTSTR szDirectory,DWORD notifyFilter,int subdirs,FileChangeCallback callback);
int           monitor_refresh(monitor_t pMonitor);
void CALLBACK monitor_callback(DWORD dwErrorCode,DWORD dwNumberOfBytesTransfered,LPOVERLAPPED lpOverlapped);
void          monitor_stop(monitor_t pMonitor);
