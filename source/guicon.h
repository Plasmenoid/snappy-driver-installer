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

#define LOG_VERBOSE_ARGS        1
#define LOG_VERBOSE_SYSINFO     2
#define LOG_VERBOSE_DEVICES     4
#define LOG_VERBOSE_MATCHER     8
#define LOG_VERBOSE_MANAGER    16
#define LOG_VERBOSE_DRP        32
#define LOG_VERBOSE_TIMES      64

extern int log_verbose;
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
void log_start(WCHAR *log_dir);
void log_stop();
void log(CHAR const *format,...);
void log_err(CHAR const *format,...);
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
