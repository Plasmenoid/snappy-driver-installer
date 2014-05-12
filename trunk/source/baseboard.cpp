#include <stdio.h>
#include <comdef.h>
#include <Wbemidl.h>

//for some reason CLSID_WbemLocator isn't declared in libwbemuuid.a (although it probably should be).
const GUID CLSID_WbemLocator={0x4590F811,0x1D3A,0x11D0,{ 0x89,0x1F,0x00,0xAA,0x00,0x4B,0x2E,0x24}};

int init=0;
extern "C" int getbaseboard(WCHAR *manuf,WCHAR *model,WCHAR *product,WCHAR *cs_manuf,WCHAR *cs_model,int *type)
{
    IWbemLocator *pLoc=0;
    IWbemServices *pSvc=0;
    IEnumWbemClassObject *pEnumerator=NULL;
    int hres;

    *manuf=*model=*product=0;
    hres=CoInitializeEx(0,COINIT_MULTITHREADED);
    if(FAILED(hres))
    {
        printf("FAILED to initialize COM library. Error code = 0x%X\n",hres);
        return 0;
    }

    if(!init)
    {
        hres=CoInitializeSecurity(NULL,-1,NULL,NULL,RPC_C_AUTHN_LEVEL_DEFAULT,
                                RPC_C_IMP_LEVEL_IMPERSONATE,NULL,EOAC_NONE,NULL);
        if(FAILED(hres))
        {
            printf("FAILED to initialize security. Error code = 0x%X\n",hres);
            CoUninitialize();
            return 0;
        }
    }

    hres=CoCreateInstance(CLSID_WbemLocator,0,CLSCTX_INPROC_SERVER,IID_IWbemLocator,(LPVOID *)&pLoc);
    if(FAILED(hres))
    {
        printf("FAILED to create IWbemLocator object. Error code = 0x%X\n",hres);
        CoUninitialize();
        return 0;
    }

    hres=pLoc->ConnectServer(_bstr_t(L"ROOT\\CIMV2"),NULL,NULL,0,NULL,0,0,&pSvc);
    if(FAILED(hres))
    {
        printf("FAILED to connect to root\\cimv2. Error code = 0x%X\n",hres);
        pLoc->Release();
        CoUninitialize();
        return 0;
    }

    printf("Connected to ROOT\\CIMV2 WMI namespace\n");

    hres=CoSetProxyBlanket(pSvc,RPC_C_AUTHN_WINNT,RPC_C_AUTHZ_NONE,NULL,
       RPC_C_AUTHN_LEVEL_CALL,RPC_C_IMP_LEVEL_IMPERSONATE,NULL,EOAC_NONE);
    if(FAILED(hres))
    {
        printf("FAILED to set proxy blanket. Error code = 0x%X\n",hres);
        pSvc->Release();
        pLoc->Release();
        CoUninitialize();
        return 0;
    }

    hres=pSvc->ExecQuery(
        _bstr_t(L"WQL"),
        _bstr_t(L"SELECT * FROM Win32_BaseBoard"),
        WBEM_FLAG_FORWARD_ONLY|WBEM_FLAG_RETURN_IMMEDIATELY,NULL,&pEnumerator);
    if(FAILED(hres))
    {
        printf("FAILED to query for Win32_BaseBoard. Error code = 0x%X\n",hres);
        pSvc->Release();
        pLoc->Release();
        CoUninitialize();
        return 0;
    }
    else
    {
        IWbemClassObject *pclsObj;
        ULONG uReturn=0;

        while(pEnumerator)
        {
            hres=pEnumerator->Next(WBEM_INFINITE,1,&pclsObj,&uReturn);
            if(0==uReturn)break;

            VARIANT vtProp1,vtProp2,vtProp3;

            hres=pclsObj->Get(L"Manufacturer",0,&vtProp1,0,0);
            if(vtProp1.bstrVal)wcscpy(manuf,vtProp1.bstrVal);

            hres=pclsObj->Get(L"Model",0,&vtProp2,0,0);
            if(vtProp2.bstrVal)wcscpy(model,vtProp2.bstrVal);

            hres=pclsObj->Get(L"Product",0,&vtProp3,0,0);
            if(vtProp3.bstrVal)wcscpy(product,vtProp3.bstrVal);
        }
    }

    hres=pSvc->ExecQuery(
        _bstr_t(L"WQL"),
        _bstr_t(L"SELECT * FROM Win32_ComputerSystem"),
        WBEM_FLAG_FORWARD_ONLY|WBEM_FLAG_RETURN_IMMEDIATELY,NULL,&pEnumerator);
    if(FAILED(hres))
    {
        printf("FAILED to query for Win32_ComputerSystem. Error code = 0x%X\n",hres);
        pSvc->Release();
        pLoc->Release();
        CoUninitialize();
        return 0;
    }
    else
    {
        IWbemClassObject *pclsObj;
        ULONG uReturn=0;

        while(pEnumerator)
        {
            hres=pEnumerator->Next(WBEM_INFINITE,1,&pclsObj,&uReturn);
            if(0==uReturn)break;

            VARIANT vtProp1,vtProp2;

            hres=pclsObj->Get(L"Manufacturer",0,&vtProp1,0,0);
            if(vtProp1.bstrVal)wcscpy(cs_manuf,vtProp1.bstrVal);

            hres=pclsObj->Get(L"Model",0,&vtProp2,0,0);
            if(vtProp2.bstrVal)wcscpy(cs_model,vtProp2.bstrVal);
        }
    }

    hres=pSvc->ExecQuery(
        _bstr_t(L"WQL"),
        _bstr_t(L"SELECT * FROM Win32_SystemEnclosure"),
        WBEM_FLAG_FORWARD_ONLY|WBEM_FLAG_RETURN_IMMEDIATELY,NULL,&pEnumerator);
    if(FAILED(hres))
    {
        printf("FAILED to query for Win32_SystemEnclosure. Error code = 0x%X\n",hres);
        pSvc->Release();
        pLoc->Release();
        CoUninitialize();
        return 0;
    }
    else
    {
        IWbemClassObject *pclsObj;
        ULONG uReturn=0;

        while(pEnumerator)
        {
            hres=pEnumerator->Next(WBEM_INFINITE,1,&pclsObj,&uReturn);
            if(0==uReturn)break;

            VARIANT vtProp;
            hres=pclsObj->Get(L"ChassisTypes",0,&vtProp,0,0);// Uint16
            if(!FAILED(hres))
            {
                if((vtProp.vt==VT_NULL)||(vtProp.vt==VT_EMPTY))
                    *type=0;
                else
                    if((vtProp.vt&VT_ARRAY))
                    {
                        long lLower,lUpper;
                        UINT32 Element=NULL;
                        SAFEARRAY *pSafeArray=vtProp.parray;
                        SafeArrayGetLBound(pSafeArray,1,&lLower);
                        SafeArrayGetUBound(pSafeArray,1,&lUpper);

                        for(long i=lLower;i<=lUpper;i++)
                        {
                            hres=SafeArrayGetElement(pSafeArray,&i,&Element);
                            *type=Element;
                        }
                        SafeArrayDestroy(pSafeArray);
                    }
                    VariantClear(&vtProp);
                    pclsObj->Release();
                    pclsObj=NULL;
            }
        }
    }

    init=1;

    pSvc->Release();
    pLoc->Release();
    CoUninitialize();
    return 1;
}
/*
int main(int argc, char **argv)
{
    WCHAR model[4096];
    WCHAR manuf[4096];
    WCHAR product[4096];

    getbaseboard(manuf,model,product);
    printf("Manufacturer: '%S'\n",manuf);
    printf("Model: '%S'\n",model);
    printf("Product: '%S'\n",product);
}
*/
