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

typedef struct _torrent_status_t
{
    long long downloaded,downloadsize;
    long long uploaded;
    int elapsed,remaining;

    WCHAR *status;
    WCHAR error[4096];
    int uploadspeed,downloadspeed;
    int seedstotal,seedsconnected;
    int peerstotal,peersconnected;
    int wasted,wastedhashfailes;

    int sessionpaused,torrentpaused;
}torrent_status_t;

extern volatile int downloadmangar_exitflag;
extern HANDLE downloadmangar_event;
extern HANDLE thandle_download;
extern torrent_status_t torrentstatus;
extern int finisheddownloading,finishedupdating;

// Dialog
void upddlg_updatelang();
void upddlg_setcheckboxes(HWND hList);
void upddlg_setpriorities(HWND hList);
void upddlg_setpriorities_driverpack(const WCHAR *name,int pri);
void upddlg_calctotalsize(HWND hList);
LRESULT CALLBACK NewButtonProc(HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
BOOL CALLBACK UpdateProcedure(HWND hwnd,UINT Message,WPARAM wParam,LPARAM lParam);
int  getnewver(const char *ptr);
int  getcurver(const char *ptr);
int  upddlg_populatelist(HWND hList);

// Update
void update_start();
void update_stop();
void update_resume();
void update_getstatus(torrent_status_t *t);
void delolddrp(const char *ptr);
void update_movefiles();
unsigned int __stdcall thread_download(void *arg);
