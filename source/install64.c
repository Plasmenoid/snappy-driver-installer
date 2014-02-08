#if defined(UNICODE) && !defined(_UNICODE)
	#define _UNICODE
#elif defined(_UNICODE) && !defined(UNICODE)
	#define UNICODE
#endif

#include <windows.h>
#include <newdev.h>
#include <stdio.h>

void print_error(int r,const WCHAR *s)
{
    WCHAR buf[4096];
    buf[0]=0;
    FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS,NULL,r,0,(LPWSTR)&buf,4000,NULL);
    printf("ERROR with %ws:[%d]'%ws'\n",s,r,buf);
}

int WINAPI WinMain(HINSTANCE hThisInstance,HINSTANCE hPrevInstance,LPSTR lpszArgument,int nCmdShow)
{
	WCHAR **argv;
    int argc;
    int ret=0,needreboot=0,lr;

	printf("Start\n");
	printf("Command: '%ws'\n",GetCommandLineW());
    argv=CommandLineToArgvW(GetCommandLineW(),&argc);

	if(argc==3)
	{
		printf("Install64.exe '%ws' '%ws'\n",argv[1],argv[2]);
		ret=UpdateDriverForPlugAndPlayDevices(0,argv[1],argv[2],INSTALLFLAG_FORCE|INSTALLFLAG_NONINTERACTIVE,&needreboot);
	}
	else
		printf("argc=%d\n",argc);
	lr=GetLastError();
	printf("Finished %d,%d,%d(%X)\n",ret,needreboot,lr,lr);
	if(!ret)
	{
		ret=lr;
		print_error(lr,L"");
	}

	LocalFree(argv);
	if(!ret)return lr;
	return ret+(needreboot?0x80000000:0);
}
