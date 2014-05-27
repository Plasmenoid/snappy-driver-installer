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

#define LOG_VERBOSE_ARGS       0x0001
#define LOG_VERBOSE_SYSINFO    0x0002
#define LOG_VERBOSE_DEVICES    0x0004
#define LOG_VERBOSE_MATCHER    0x0008
#define LOG_VERBOSE_MANAGER    0x0010
#define LOG_VERBOSE_DRP        0x0020
#define LOG_VERBOSE_TIMES      0x0040
#define LOG_VERBOSE_LOG_ERR    0x0080
#define LOG_VERBOSE_LOG_CON    0x0100
#define LOG_VERBOSE_LAGCOUNTER 0x0200
#define LOG_VERBOSE_DEVSYNC    0x0400

extern int log_verbose;
extern int log_console;
extern WCHAR timestamp[4096];
extern long
    time_total,
    time_startup,
    time_indexes,
    time_devicescan,
    time_indexsave,
    time_indexprint,
    time_sysinfo,
    time_matcher,
    time_test;

extern int error_count;

//#define log_index log

#ifndef log_index
#define log_index log_nul
#endif

// Console
void RedirectIOToFiles();
void closeConsole();

// Logging
void log_times();
void gen_timestamp();
void log_start(WCHAR *log_dir);
void log_stop();
void log_file(CHAR const *format,...);
void log_err(CHAR const *format,...);
void log_con(CHAR const *format,...);
void log_nul(CHAR const *format,...);

WCHAR *errno_str();
void print_error(int r,const WCHAR *s);
DWORD RunSilent(WCHAR* file,WCHAR* cmd,int show,int wait);
int canWrite(const WCHAR *path);
void CALLBACK viruscheck(LPTSTR szFile,DWORD action,LPARAM lParam);
void virusmonitor_start();
void virusmonitor_stop();
void CloseHandle_log(HANDLE h,WCHAR *func,WCHAR *obj);
void UnregisterClass_log(LPCTSTR lpClassName,HINSTANCE hInstance,WCHAR *func,WCHAR *obj);
