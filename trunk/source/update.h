/*update_stop();
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
}torrent_status_t;

// Dialog
BOOL CALLBACK UpdateProcedure(HWND hwnd,UINT Message,WPARAM wParam,LPARAM lParam);
int getver(const char *ptr);
int getcurver(const char *ptr);
void updatelang(HWND hwnd);
void populatelist(HWND hList);

// Update
void update_start();
void update_stop();
void updatestatus(HWND hList);
void updatepriorities(HWND hList);
void update_getstatus(torrent_status_t *t);
