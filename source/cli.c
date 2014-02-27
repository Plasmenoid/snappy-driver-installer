#include "main.h"

const WCHAR INSTALLEDVENFILENAMEDEFPATH[]=L"%temp%\\SDI2\\InstalledID.txt";

HFONT CLIHelp_Font;

void SaveHWID(WCHAR *hwid)
{
    if (CLIParam.SaveInstalledHWD)
    {
        FILE *f=_wfopen(CLIParam.SaveInstalledFileName,L"a+");
        if (!f)
          log_err("Failed to create '%ws'\n",CLIParam.SaveInstalledFileName);
        fwprintf(f, hwid);
        fwprintf(f, L"\n");
        fclose(f);
    }
}

void ExpandPath(WCHAR * Apath)
{
  #define INFO_BUFFER_SIZE 32767
  WCHAR  infoBuf[INFO_BUFFER_SIZE];
  DWORD  bufCharCount;
  memset( infoBuf, 0, sizeof(infoBuf) );
  bufCharCount = ExpandEnvironmentStringsW(Apath, infoBuf, INFO_BUFFER_SIZE );
  wcscpy(Apath,infoBuf);
}

static BOOL CALLBACK ShowHelpProcedure(HWND hwnd,UINT Message,WPARAM wParam,LPARAM lParam)
{
   UNREFERENCED_PARAMETER(lParam);

    HWND hEditBox;
    void *s;
    int sz;

    switch(Message)
    {
    case WM_INITDIALOG:
        SetWindowTextW(hwnd,L"Command Line options help");
        hEditBox=GetDlgItem(hwnd,0);
        SetWindowTextW(hEditBox,L"Command Line options");
        //ShowWindow(hEditBox,SW_HIDE);
        hEditBox=GetDlgItem(hwnd,IDOK);
        ShowWindow(hEditBox,SW_HIDE);
        hEditBox=GetDlgItem(hwnd,IDCANCEL);
        SetWindowTextW(hEditBox,L"Close");


        get_resource(IDR_CLI_HELP,&s,&sz);
        hEditBox=GetDlgItem(hwnd,IDC_EDIT1);

        SendMessage(hEditBox,WM_SETFONT,(WPARAM)CLIHelp_Font,0);


        SetWindowTextA(hEditBox,s);
        SendMessage(hEditBox,EM_SETREADONLY,1,0);

        return TRUE;

    case WM_COMMAND:
        switch(LOWORD(wParam))
        {
        case IDOK:
//            license=1;
            EndDialog(hwnd,IDOK);
            return TRUE;

        case IDCANCEL:
//            license=0;
            EndDialog(hwnd,IDCANCEL);
            return TRUE;

        default:
            break;
        }
        break;

    default:
        break;;
    }
    return FALSE;
}
static void ShowHelp(HINSTANCE AhInst)
{
 CLIHelp_Font=CreateFontA(-12,0,0,0,FW_NORMAL,0,0,0,
                         DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,
                         DEFAULT_QUALITY,FF_DONTCARE,"Consolas");

  DialogBox(AhInst ,MAKEINTRESOURCE(IDD_DIALOG1),0,ShowHelpProcedure);
  DeleteObject(CLIHelp_Font);
}

void init_CLIParam()
{
  memset(&CLIParam, 0, sizeof(CLIParam));
  CLIParam.ShowHelp = FALSE; //показывать подсказку по параметрам командной строки
    //
  CLIParam.SaveInstalledHWD  = FALSE; //Сохранять установленные HWID в файл
  CLIParam.SaveInstalledFileName[0] = 0; //Имя файла для сохранения HWID
    //
  CLIParam.HWIDInstalled= FALSE; //Проверка что драйвер был установлен в предыдущем сеансе работы
}

void Parse_save_installed_id_swith(const WCHAR *ParamStr)
{
  int tmpLen = wcslen(SAVE_INSTALLED_ID_DEF);
  if (wcslen(ParamStr) > tmpLen)
  {
    if (ParamStr[tmpLen]==L':') wcscpy(CLIParam.SaveInstalledFileName,ParamStr+tmpLen+1);  else
    if (ParamStr[tmpLen]==L' ') wcscpy(CLIParam.SaveInstalledFileName, INSTALLEDVENFILENAMEDEFPATH);
    else return; //Это другой идентификатор, нам его обрабатывать не нужно
  }
  else
    wcscpy(CLIParam.SaveInstalledFileName, INSTALLEDVENFILENAMEDEFPATH);
  CLIParam.SaveInstalledHWD = TRUE;
}

void RUN_CLI(CommandLineParam_t ACLIParam)
{
    WCHAR buf[BUFLEN];
    if (ACLIParam.ShowHelp)
    {
        ShowHelp(ghInst);
        flags|=FLAG_AUTOCLOSE|FLAG_NOGUI;
        statemode=STATEMODE_EXIT;
        return;
    } else
    if (CLIParam.SaveInstalledHWD)
    {   //Сохранять установленные HWID в файл
        ExpandPath(CLIParam.SaveInstalledFileName);
        wcscpy(buf, CLIParam.SaveInstalledFileName);
        PathRemoveFileSpec(buf);
        CreateDirectory(buf, NULL);
        DeleteFileW(CLIParam.SaveInstalledFileName);
    }
    else
    if (CLIParam.HWIDInstalled)
    {
      ExpandPath(CLIParam.SaveInstalledFileName);
      FILE *f;
      WCHAR *RStr;
      f=_wfopen(CLIParam.SaveInstalledFileName,L"rt");
      if(!f) log_err("Failed to open '%ws'\n",CLIParam.SaveInstalledFileName);

      while(fgetws(buf,sizeof(buf),f))
	  { log_err("'%ws'\n", buf);
	    RStr = wcsstr(buf, CLIParam.HWIDSTR);
	    if (RStr != NULL)
        {
          ret_global = 1;
          break;
        }
	  };
      fclose(f);
      flags |= FLAG_AUTOCLOSE|FLAG_NOGUI;
      statemode = STATEMODE_EXIT;
    };
    //else;
}

void Parse_HWID_installed_swith(const WCHAR *ParamStr)
{
  int tmpLen = wcslen(HWIDINSTALLED_DEF);
  if (wcslen(ParamStr) < (tmpLen+17)) //-HWIDInstalled:VEN_xxxx&DEV_xxxx
  {
    log_err("invalid parameter %ws\n", ParamStr);
    ret_global = ERROR_BAD_LENGTH;
    statemode=STATEMODE_EXIT;
    return;
  }
  else
  { //парсим параметр
    WCHAR buf[BUFLEN];
    wcscpy(buf, ParamStr+tmpLen);
    WCHAR *chB;

    chB = wcsrchr (buf,'=');
    if (chB== NULL)
      wcscpy(CLIParam.SaveInstalledFileName, INSTALLEDVENFILENAMEDEFPATH);
    else
    {
      tmpLen = chB-buf+1;
      wcscpy(CLIParam.SaveInstalledFileName, buf+tmpLen);
      buf [tmpLen-1] = NULL;
    }
    wcscpy(CLIParam.HWIDSTR, buf);
    CLIParam.HWIDInstalled = TRUE;
  }
}
