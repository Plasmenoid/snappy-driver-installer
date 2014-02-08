// Common/CRC.cpp

#include "StdAfx.h"

#include "../../C/7zCrc.h"

__declspec(dllexport) struct CCRCTableInit { CCRCTableInit() { CrcGenerateTable(); } } g_CRCTableInit;

extern "C"  void registercrc()
{
	CCRCTableInit *a;
}
