#ifndef CLI_H_INCLUDED
#define CLI_H_INCLUDED

typedef struct _CommandLineParam_t
{
    BOOL ShowHelp; //показывать подсказку по параметрам командной строки
    //
    BOOL SaveInstalledHWD; //Сохранять установленные HWID в файл
    WCHAR SaveInstalledFileName[BUFLEN]; //Имя файла для сохранения HWID
    //
    BOOL HWIDInstalled; //Проверка что драйвер был установлен в предыдущем сеансе работы
    WCHAR HWIDSTR[BUFLEN]; //идентификатор драйвера для проверки
    //WCHAR HWIDFileName;
} CommandLineParam_t;

 CommandLineParam_t CLIParam;

void init_CLIParam();

//параметры командной строки
#define SAVE_INSTALLED_ID_DEF   L"-save-installed-id"
#define HWIDINSTALLED_DEF       L"-HWIDInstalled:"

void SaveHWID(WCHAR *hwid);
void ExpandPath(WCHAR * Apath);
void Parse_save_installed_id_swith(const WCHAR *ParamStr);
void RUN_CLI(CommandLineParam_t ACLIParam);
void Parse_HWID_installed_swith(const WCHAR *ParamStr);
#endif // CLI_H_INCLUDED
