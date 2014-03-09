#ifndef CLI_H_INCLUDED
#define CLI_H_INCLUDED

typedef struct _CommandLineParam_t
{
    BOOL ShowHelp; //���������� ��������� �� ���������� ��������� ������
    //
    BOOL SaveInstalledHWD; //��������� ������������� HWID � ����
    WCHAR SaveInstalledFileName[BUFLEN]; //��� ����� ��� ���������� HWID
    //
    BOOL HWIDInstalled; //�������� ��� ������� ��� ���������� � ���������� ������ ������
    WCHAR HWIDSTR[BUFLEN]; //������������� �������� ��� ��������
    //WCHAR HWIDFileName;
} CommandLineParam_t;

 CommandLineParam_t CLIParam;

void init_CLIParam();

//��������� ��������� ������
#define SAVE_INSTALLED_ID_DEF   L"-save-installed-id"
#define HWIDINSTALLED_DEF       L"-HWIDInstalled:"
#define GFG_DEF                 L"-cfg:"

void SaveHWID(WCHAR *hwid);
void ExpandPath(WCHAR * Apath);
void Parse_save_installed_id_swith(const WCHAR *ParamStr);
void RUN_CLI(CommandLineParam_t ACLIParam);
void Parse_HWID_installed_swith(const WCHAR *ParamStr);

void LoadCFGFile(const WCHAR *FileName, WCHAR *DestStr);
BOOL isCfgSwithExist(const WCHAR *cmdParams, WCHAR *cfgPath);
#endif // CLI_H_INCLUDED
