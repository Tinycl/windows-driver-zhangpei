
#include <DriverSpecs.h>
__user_code  

#include <windows.h>
#include <setupapi.h>
#include <stdio.h>
#include "resource.h"
#include <dontuse.h>


#if DBG
#define DbgOut(Text) OutputDebugString(TEXT("ClassInstaller: " Text "\n"))
#else
#define DbgOut(Text) 
#endif 

INT_PTR CY001PageProc(__in HWND   hDlg,
               __in UINT   uMessage,
               __in WPARAM wParam,
               __in LPARAM lParam);

DWORD PropPageCY001(
    __in  HDEVINFO            DeviceInfoSet,
    __in  PSP_DEVINFO_DATA    DeviceInfoData OPTIONAL
);

HMODULE ModuleInstance;

BOOL WINAPI DllMain(
    HINSTANCE DllInstance,
    DWORD Reason,
    PVOID Reserved)
{

    UNREFERENCED_PARAMETER( Reserved );

    switch(Reason) 
    {
        case DLL_PROCESS_ATTACH: 
        {

            ModuleInstance = DllInstance;
            DisableThreadLibraryCalls(DllInstance);
            InitCommonControls();
            break;
        }

        case DLL_PROCESS_DETACH: 
        {
            ModuleInstance = NULL;
            break;
        }

        default: 
            break;
    }

    return TRUE;
}

void DbgPrintDIFCode(DI_FUNCTION InstallFunction)
{
    switch (InstallFunction)
    {
        case DIF_INSTALLDEVICE: 
            DbgOut("DIF_INSTALLDEVICE");
            break;
            
        case DIF_ADDPROPERTYPAGE_ADVANCED:
            DbgOut("DIF_ADDPROPERTYPAGE_ADVANCED");
        	break;
        	
        case DIF_POWERMESSAGEWAKE:
            DbgOut("DIF_POWERMESSAGEWAKE");
            break;

        case DIF_PROPERTYCHANGE:
            DbgOut("DIF_PROPERTYCHANGE");
            break;
            
        case DIF_REMOVE: 
             DbgOut("DIF_REMOVE");
             break;
             
        case DIF_NEWDEVICEWIZARD_FINISHINSTALL:
            DbgOut("DIF_NEWDEVICEWIZARD_FINISHINSTALL");
            break;
            
        case DIF_SELECTDEVICE:
            DbgOut("DIF_SELECTDEVICE");
            break;
            
        case DIF_DESTROYPRIVATEDATA:
            DbgOut("DIF_DESTROYPRIVATEDATA");
            break;
        case DIF_INSTALLDEVICEFILES:
            DbgOut("DIF_INSTALLDEVICEFILES");
            break;
            
        case DIF_ALLOW_INSTALL:
            DbgOut("DIF_ALLOW_INSTALL");
            break;
        case DIF_SELECTBESTCOMPATDRV:
            DbgOut("DIF_SELECTBESTCOMPATDRV");
            break;

        case DIF_INSTALLINTERFACES:
            DbgOut("DIF_INSTALLINTERFACES");
            break;
            
        case DIF_REGISTER_COINSTALLERS:
            DbgOut("DIF_REGISTER_COINSTALLERS");
            break;
        default:
            DbgOut("DIF_UNKNOWN");
            break;
    }
}

DWORD CALLBACK CY001ClassInstaller(
    __in  DI_FUNCTION         InstallFunction,
    __in  HDEVINFO            DeviceInfoSet,
    __in  PSP_DEVINFO_DATA    DeviceInfoData OPTIONAL
    )
{
    switch (InstallFunction)
    {    
    	// �����µ�����ҳ���ҽ���������д��Hello CY001������    
        case DIF_ADDPROPERTYPAGE_ADVANCED:
            DbgOut("DIF_ADDPROPERTYPAGE_ADVANCED");
            return PropPageCY001(DeviceInfoSet, DeviceInfoData);      
        
        default:
            DbgPrintDIFCode(InstallFunction);
            break;
    }   
    return ERROR_DI_DO_DEFAULT;    
}

// �����Զ��������ҳ
DWORD PropPageCY001(
    __in  HDEVINFO            DeviceInfoSet,
    __in  PSP_DEVINFO_DATA    DeviceInfoData OPTIONAL
)
{
    HPROPSHEETPAGE  pageHandle;
    PROPSHEETPAGE   page;
    SP_ADDPROPERTYPAGE_DATA AddPropertyPageData = {0};

    if (DeviceInfoData==NULL) {
        return ERROR_DI_DO_DEFAULT;
    }

    AddPropertyPageData.ClassInstallHeader.cbSize = 
         sizeof(SP_CLASSINSTALL_HEADER);

    // ���Ա�����ϸ���壬�������లװ�����ṹ���С�
    // �����������Ҫ�ֳ�������
    // 1. ȡ�ṹ��
    // 2. ������ҳ�棬���޸Ľṹ��
    // 3. ��ṹ��
    if (SetupDiGetClassInstallParams(DeviceInfoSet, DeviceInfoData,
         (PSP_CLASSINSTALL_HEADER)&AddPropertyPageData,
         sizeof(SP_ADDPROPERTYPAGE_DATA), NULL )) 
    {
        // ����һ�����ҳ�������ܳ���
        if(AddPropertyPageData.NumDynamicPages >= MAX_INSTALLWIZARD_DYNAPAGES)
        {
            return NO_ERROR;
        }
        
        // ����һ����ҳ��
        memset(&page, 0, sizeof(PROPSHEETPAGE));

        page.dwSize = sizeof(PROPSHEETPAGE);
        page.dwFlags = 0;
        page.hInstance = ModuleInstance;
        page.pszTemplate = MAKEINTRESOURCE(DLG_TOASTER_PORTSETTINGS);
        page.pfnDlgProc = CY001PageProc;
        page.pfnCallback = NULL;
        page.lParam = 0;

        pageHandle = CreatePropertySheetPage(&page);
        if(!pageHandle)
        {
            return NO_ERROR;
        }

        // ����µ�����ҳ�����������ø����豸
        AddPropertyPageData.DynamicPages[
            AddPropertyPageData.NumDynamicPages++]=pageHandle;

        SetupDiSetClassInstallParams(DeviceInfoSet,
                    DeviceInfoData,
                    (PSP_CLASSINSTALL_HEADER)&AddPropertyPageData,
                    sizeof(SP_ADDPROPERTYPAGE_DATA));
    }
    
    return NO_ERROR;
} 

// Pageҳ�����Ϣ������
INT_PTR CY001PageProc(__in HWND   hDlg,
                   __in UINT   uMessage,
                   __in WPARAM wParam,
                   __in LPARAM lParam)
{
	UNREFERENCED_PARAMETER(wParam);
	UNREFERENCED_PARAMETER(lParam);
	UNREFERENCED_PARAMETER(hDlg);
	
    switch(uMessage) 
    {
    case WM_COMMAND:
    case WM_CONTEXTMENU:
    case WM_HELP:
    case WM_NOTIFY:
    case WM_INITDIALOG:
    	return TRUE;

    default: 
        return FALSE;
    }
} 