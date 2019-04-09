#include "stdafx.h"
#include <newdev.h>
#include <setupapi.h>
#include <cfgmgr32.h>
#include <regstr.h>
#include <strsafe.h>
#include "setup.h"

#pragma comment(lib, "newdev.lib")
#pragma comment(lib, "setupapi.lib")

void GetHIDList(int nInfIndex);

// �������
void ULOG(TCHAR *msg)
{
	OutputDebugString(_T("UsbKitApp:"));
	OutputDebugString(msg);
	OutputDebugString(_T("\r\n"));
}

#ifdef DEBUG
// ����ε�Log����
void UDBG(TCHAR *msg, ...)
{
	TCHAR strMsg[1024];

	va_list argp;
	va_start(argp, msg);
	_vstprintf_s(strMsg, 1024, msg, argp);
	va_end(argp);
	OutputDebugString(_T("UsbKitApp:"));
	OutputDebugString(strMsg);
	OutputDebugString(_T("\r\n"));
}
#else
void UDBG(TCHAR* msg, ...)
{

}
#endif

PINF_INFO g_InfList[MAX_INF_COUNT];
int g_nInfMaxCount = 0;
int g_nInfCount = 0;	// INF�ļ���

PHID_INFO g_HIDInfo[MAX_HID_COUNT];
int g_nHIDMaxCount = 0;
int g_nHIDCount = 0;	// HID����	

HID_DEVICES g_HIDDevices;

void FreeDevices()
{
	for(int i = 0; i < g_HIDDevices.nMaxCount; i++)
	{
		delete g_HIDDevices.asDeviceInstanceID[i];
	}

	ZeroMemory(&g_HIDDevices, sizeof(g_HIDDevices));
}

void FreeAllInfFiles()
{
	for(int i = 0; i < g_nHIDMaxCount; i++)
		delete g_HIDInfo[i];

	g_nHIDMaxCount = g_nHIDCount = 0;

	for(int i = 0; i < g_nInfMaxCount; i++)
		delete g_InfList[i];

	g_nInfMaxCount = g_nInfCount = 0;
}

int FindAllInfFiles(char* csDir = NULL)
{
	WIN32_FIND_DATA FindFileData;
	char dir[MAX_PATH];
	char path[MAX_PATH];
	HANDLE hFind = INVALID_HANDLE_VALUE;

	if(csDir == NULL)
	{
		// �ڵ�ǰ·������������inf�ļ�
		GetCurrentDirectory(MAX_PATH, dir);
		StringCchCopy(path, MAX_PATH, dir);
	}
	else
	{
		if(!GetFullPathName(csDir, MAX_PATH, dir, NULL))
			GetCurrentDirectory(MAX_PATH, dir);

		StringCchCopy(path, MAX_PATH, dir);
	}

	StringCchCat(path, MAX_PATH, "\\*.inf");

	UDBG(path);

	__try{

		g_nInfCount = 0;
		g_nHIDCount = 0;

		// ����
		hFind = FindFirstFile(path, &FindFileData);

		if (hFind == INVALID_HANDLE_VALUE) 
		{
			UDBG("û�з���inf�ļ�");
			__leave;
		} 

		do{
			if(g_nInfCount >= g_nInfMaxCount)
			{
				g_InfList[g_nInfCount] = new INF_INFO;
				g_nInfMaxCount = g_nInfCount + 1;
			}

			ZeroMemory(g_InfList[g_nInfCount], sizeof(INF_INFO));

			StringCchCopy(path, MAX_PATH, dir);
			StringCchCat(path, MAX_PATH, "\\");
			StringCchCat(path, MAX_PATH, FindFileData.cFileName);
			StringCchCopy(g_InfList[g_nInfCount]->asInfName, MAX_PATH, FindFileData.cFileName);
			StringCchCopy(g_InfList[g_nInfCount]->asFullPath, MAX_PATH, path);

			// ����ÿ��inf�ļ��е�����HID
			UDBG("�ҵ��ļ���%s", FindFileData.cFileName);

			GetHIDList(g_nInfCount);
			g_nInfCount ++;

		}while (FindNextFile(hFind, &FindFileData) != 0 && g_nInfMaxCount < MAX_INF_COUNT) ;

	}
	__finally{
		if(hFind != INVALID_HANDLE_VALUE)
		{
			FindClose(hFind);
		}
	}

	return g_nHIDCount;
}

void GetHIDList(int nInfIndex)
{
	INFCONTEXT infcont;	
	INFCONTEXT infcont_dev;

	HINF hInf = INVALID_HANDLE_VALUE;
	DWORD config_flags, problem, status;
	BOOL reboot;
	char inf_path[MAX_PATH];

	char direct[MAX_PATH];
	char manu[MAX_PATH];
	char manu_os[MAX_PATH];
	char hid[MAX_PATH];
	char tmp_id[MAX_PATH];

	char os[8][32];

	__try
	{
		// ȡINF·��
		UDBG(g_InfList[nInfIndex]->asFullPath);
		if(!GetFullPathName(g_InfList[nInfIndex]->asFullPath, MAX_PATH, inf_path, NULL))
		{
			UDBG(".inf�ļ� %s δ�ҵ�", g_InfList[nInfIndex]->asInfName);
			__leave;
		}

		UDBG("INF �ļ�·����%s", inf_path);

		// ��INF�ļ�
		hInf = SetupOpenInfFile(inf_path, NULL, INF_STYLE_WIN4, NULL);

		if(hInf == INVALID_HANDLE_VALUE)
		{
			UDBG("���ļ�ʧ�ܣ�%s", inf_path);
			__leave;
		}

		if(!SetupFindFirstLine(hInf, "version", NULL, &infcont))
		{
			UDBG("�Ҳ���[Version]�򣬲��ǺϷ���Inf�ļ�");
			__leave;
		}

		//
		// ��ȡVersion��Ϣ
		//

		do 
		{
			if(!SetupGetStringField(&infcont, 0, direct, sizeof(direct), NULL))
				continue;

			strlwr(direct);
			if(0 == strcmp(direct, "class"))
			{
				if(SetupGetStringField(&infcont, 1, direct, sizeof(direct), NULL))
				{
					StringCchCopy(g_InfList[nInfIndex]->asClassName, MAX_PATH, direct);
				}
			}
			else if(0 == strcmp(direct, "driverver"))
			{
				if(SetupGetStringField(&infcont, 1, direct, sizeof(direct), NULL))
				{
					StringCchCopy(g_InfList[nInfIndex]->asVersion, MAX_PATH, direct);

					if(SetupGetStringField(&infcont, 2, direct, sizeof(direct), NULL))
					{
						StringCchCat(g_InfList[nInfIndex]->asVersion, MAX_PATH, ", ");
						StringCchCat(g_InfList[nInfIndex]->asVersion, MAX_PATH, direct);
					}
				}
			}
			else if(0 == strcmp(direct, "provider"))
			{
				if(SetupGetStringField(&infcont, 1, direct, sizeof(direct), NULL))
				{
					StringCchCopy(g_InfList[nInfIndex]->asProvider, MAX_PATH, direct);
				}
			}
			else if(strstr(direct, "catalogfile"))
			{
				if(SetupGetStringField(&infcont, 1, direct, sizeof(direct), NULL))
				{
					StringCchCopy(g_InfList[nInfIndex]->asDSign, MAX_PATH, direct);
				}
			}
		} while (SetupFindNextLine(&infcont, &infcont));

		UDBG("INF ��Ϣ��");
		UDBG("%s", g_InfList[nInfIndex]->asClassName);
		UDBG("%s", g_InfList[nInfIndex]->asProvider);
		UDBG("%s", g_InfList[nInfIndex]->asDSign);
		UDBG("%s", g_InfList[nInfIndex]->asVersion);

		if(g_nHIDMaxCount >= MAX_HID_COUNT)
			__leave;

		//
		// �����豸������
		//

		if(!SetupFindFirstLine(hInf, "Manufacturer", NULL, &infcont))
		{
			UDBG("û�п����豸������");
			__leave;
		}

		SYSTEM_INFO siSysInfo;
		GetSystemInfo(&siSysInfo); 	 

		do{
			int i = 0;
			int j = 0;

			// һ��������
			if(!SetupGetStringField(&infcont, 1, manu, sizeof(manu), NULL))
			{
				continue;
			}

			UDBG(manu);

			for(; i < 4; i++)
			{
				if(!SetupGetStringField(&infcont, i+2, os[i], sizeof(os[0]), NULL))
					break;
				else
					UDBG("OS: %s", os[i]);

				strlwr(os[i]);
			}

			// Ѱ����ѵ�ƽ̨ƥ��
			if(siSysInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64) // X64
			{
				for(; j < i; j++)
				{
					if(strcmp(os[j], "ntamd64"))
					{
						StringCchCopy(manu_os, sizeof(manu_os), manu);
						StringCchCat(manu_os, sizeof(manu_os), ".");
						StringCchCat(manu_os, sizeof(manu_os), os[j]);

						// ����ҵ�[XXX.ntamd64],������������ȥ��os��׺
						if(SetupFindFirstLine(hInf, manu_os, NULL, &infcont_dev))
							StringCchCopy(manu, sizeof(manu), manu_os);

						break;
					}
				}
			}
			else if(siSysInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_IA64)
			{
				for(; j < i; j++)
				{
					if(0 == strstr(os[j], "ntia64"))
					{
						StringCchCopy(manu_os, sizeof(manu_os), manu);
						StringCchCat(manu_os, sizeof(manu_os), ".");
						StringCchCat(manu_os, sizeof(manu_os), os[j]);

						// ����ҵ�[XXX.ntamd64],������������ȥ��os��׺
						if(SetupFindFirstLine(hInf, manu_os, NULL, &infcont_dev))
							StringCchCopy(manu, sizeof(manu), manu_os);

						break;
					}
				}
			}
			else if(siSysInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL)
			{
				for(; j < i; j++)
				{
					if(0 == strstr(os[j], "ntx86"))
					{
						StringCchCopy(manu_os, sizeof(manu_os), manu);
						StringCchCat(manu_os, sizeof(manu_os), ".");
						StringCchCat(manu_os, sizeof(manu_os), os[j]);
						UDBG(manu_os);

						// ����ҵ�[XXX.ntamd64],������������ȥ��os��׺
						if(SetupFindFirstLine(hInf, manu_os, NULL, &infcont_dev))
							StringCchCopy(manu, sizeof(manu), manu_os);

						break;
					}
				}
			}

			// ����������ϵİ汾û���ҵ����Ϳ���.nt��û��
			if(NULL == strstr(manu, ".ntx86") && 
				NULL == strstr(manu, ".ntia64") && 
				NULL == strstr(manu, ".ntamd64"))
			{
				for(j = 0; j < i; j++)
				{
					if(0 == strstr(os[j], ".nt"))
					{
						StringCchCopy(manu_os, sizeof(manu_os), manu);
						StringCchCat(manu_os, sizeof(manu_os), ".");
						StringCchCat(manu_os, sizeof(manu_os), os[j]);

						// ����ҵ�[XXX.ntamd64],������������ȥ��os��׺
						if(SetupFindFirstLine(hInf, manu_os, NULL, &infcont_dev))
							StringCchCopy(manu, sizeof(manu), manu_os);

						break;
					}
				}
			}

			UDBG("�豸������%s", manu);
			if(!SetupFindFirstLine(hInf, manu, NULL, &infcont_dev))
			{
				UDBG("�Ҳ����豸ģ��");
				continue;
			}

			do{
				if(!SetupGetStringField(&infcont_dev, 2, hid, sizeof(hid), NULL))
					continue;

				// ����HID��¼
				//

				strlwr(hid);
				UDBG("HID: %s", hid);

				if(g_nHIDMaxCount <= g_nHIDCount)
				{
					g_HIDInfo[g_nHIDCount] = new HID_INFO;
					g_nHIDMaxCount = g_nHIDCount + 1;
				}

				ZeroMemory(g_HIDInfo[g_nHIDCount], sizeof(HID_INFO));
				StringCchCopy(g_HIDInfo[g_nHIDCount]->asHID, MAX_PATH, hid);
				g_HIDInfo[g_nHIDCount]->nInfNum = nInfIndex;
				g_nHIDCount++;

			}while(SetupFindNextLine(&infcont_dev, &infcont_dev) && g_nHIDMaxCount < MAX_HID_COUNT);

			// ��ȡ��һ���豸ID
		}while(SetupFindNextLine(&infcont, &infcont) && g_nHIDMaxCount < MAX_HID_COUNT);
	}
	__finally
	{
		if(hInf != INVALID_HANDLE_VALUE)
			SetupCloseInfFile(hInf);
	}
}

BOOL Reboot()
{
	HANDLE Token = NULL;
	TOKEN_PRIVILEGES NewPrivileges;
	LUID Luid;

	__try
	{
		// ʹ��ShutDown��Ȩ
		if(!OpenProcessToken(GetCurrentProcess(),TOKEN_ADJUST_PRIVILEGES,&Token))
			__leave;

		if(!LookupPrivilegeValue(NULL,SE_SHUTDOWN_NAME,&Luid))
			__leave;

		NewPrivileges.PrivilegeCount = 1;
		NewPrivileges.Privileges[0].Luid = Luid;
		NewPrivileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

		AdjustTokenPrivileges(Token, FALSE, &NewPrivileges, 0, NULL, NULL);
	}
	__finally
	{
		if(Token) CloseHandle(Token);
	}

	//��������
	return InitiateSystemShutdownEx(NULL, NULL, 0, FALSE, TRUE, REASON_PLANNED_FLAG | REASON_HWINSTALL);
}

// ɾ���豸��Ӧ���������򣬵���SetupDiCallClassInstaller��������ִ��DIF_REMOVE����
// ����Devs��DevInfo����Ψһ��ʶĿ���豸
bool RemoveDevice(__in HDEVINFO Devs, __in PSP_DEVINFO_DATA DevInfo)
{
	UDBG(_T("[RemoveDriverForSpecifiedDevice]"));

	SP_REMOVEDEVICE_PARAMS rmdParams;
	SP_DEVINSTALL_PARAMS devParams;

	rmdParams.ClassInstallHeader.cbSize = sizeof(SP_CLASSINSTALL_HEADER);
	rmdParams.ClassInstallHeader.InstallFunction = DIF_REMOVE;
	rmdParams.Scope = DI_REMOVEDEVICE_GLOBAL;
	rmdParams.HwProfile = 0;

	if(SetupDiSetClassInstallParams(Devs, DevInfo, &rmdParams.ClassInstallHeader, sizeof(rmdParams)) &&
		SetupDiCallClassInstaller(DIF_REMOVE, Devs, DevInfo)) 
	{
		UDBG(_T("ж�������ɹ�"));

		// ����Ƿ���Ҫ��������
		devParams.cbSize = sizeof(devParams);
		if(SetupDiGetDeviceInstallParams(Devs, DevInfo, &devParams) && 
			(devParams.Flags & (DI_NEEDRESTART|DI_NEEDREBOOT))) 
		{
			g_HIDDevices.bNeedReboot = true;
		}

		return true;
	}

	return false;
}

void RemoveDevices()
{
	HDEVINFO dev_info;
	SP_DEVINFO_DATA dev_info_data;
	int nDevIndex;
	char id[MAX_PATH];

	g_HIDDevices.bNeedReboot = false;

	// Ѱ�������豸������FlagֵΪDIGCF_ALLCLASSES��
	dev_info = SetupDiGetClassDevs(NULL, NULL, NULL, DIGCF_ALLCLASSES);

	// class����Ч
	if(dev_info == INVALID_HANDLE_VALUE)
	{
		UDBG("��Ч��HDEVINFO���");
		return;
	}

	nDevIndex = 0;

	// ö���豸
	dev_info_data.cbSize = sizeof(dev_info_data);
	while(SetupDiEnumDeviceInfo(dev_info, nDevIndex, &dev_info_data))
	{
		if(CR_SUCCESS != CM_Get_Device_ID(dev_info_data.DevInst, id, MAX_PATH, 0))
			continue;

		strlwr(id);

		for(int i = 0; i < g_HIDDevices.nCount; i++)
		{
			if(g_HIDDevices.bWillBeDelete[i] && 
				0 == strcmp(id, g_HIDDevices.asDeviceInstanceID[i]))
			{
				UDBG(id);
				g_HIDDevices.bSuccess[i] = RemoveDevice(dev_info, &dev_info_data);
			}
		}

		nDevIndex++;
	}

	SetupDiDestroyDeviceInfoList(dev_info);
}

void FindAllDevices(const char* asHID)
{
	HDEVINFO dev_info;
	SP_DEVINFO_DATA dev_info_data;

	int nDevIndex;
	char id[MAX_PATH];
	char *p;
	ULONG problem;
	ULONG status;

	char* ashid = strdup(asHID);
	strlwr(ashid);

	UDBG("FindAllDevices");

	g_HIDDevices.nCount = 0;

	// Ѱ�������豸������FlagֵΪDIGCF_ALLCLASSES��
	dev_info = SetupDiGetClassDevs(NULL, NULL, NULL, DIGCF_ALLCLASSES);

	// class����Ч
	if(dev_info == INVALID_HANDLE_VALUE)
	{
		UDBG("��Ч��HDEVINFO���");
		free(ashid);
		return;
	}

	nDevIndex = 0;

	// ö���豸
	dev_info_data.cbSize = sizeof(dev_info_data);
	while(SetupDiEnumDeviceInfo(dev_info, nDevIndex, &dev_info_data))
	{
		if(CR_SUCCESS != CM_Get_Device_ID(dev_info_data.DevInst, id, MAX_PATH, 0))
			continue;

		strlwr(id);
		if(strstr(id, ashid))
		{
			if(g_HIDDevices.nCount >= g_HIDDevices.nMaxCount)
			{
				g_HIDDevices.asDeviceInstanceID[g_HIDDevices.nMaxCount] = new char[MAX_PATH];
				g_HIDDevices.nMaxCount++;
			}

			// Deviceʵ��ID��
			StringCchCopy(g_HIDDevices.asDeviceInstanceID[g_HIDDevices.nCount], MAX_PATH, id);

			// ����״̬
			g_HIDDevices.bLink[g_HIDDevices.nCount] = false;
			if(CM_Get_DevNode_Status(&status,
				&problem,
				dev_info_data.DevInst,
				0) != CR_NO_SUCH_DEVINST)
			{
				g_HIDDevices.bLink[g_HIDDevices.nCount] = true;
			}

			g_HIDDevices.nCount++;
		}

		nDevIndex++;
	}

	SetupDiDestroyDeviceInfoList(dev_info);
	free(ashid);
}

void InstallDriver()
{
	HDEVINFO dev_info;
	SP_DEVINFO_DATA dev_info_data;
	int nDevIndex;
	char id[MAX_PATH];
	char tmp_id[MAX_PATH];
	char *p;
	ULONG problem;
	ULONG status;
	int nInfIndex;

	for(nInfIndex = 0; nInfIndex < g_nInfCount; nInfIndex++)
	{
		if(false == g_InfList[nInfIndex]->bWillBeInstall)
			continue;

		// ����INF�ļ�
		//
		if(FALSE ==	SetupCopyOEMInf(g_InfList[nInfIndex]->asFullPath, NULL, SPOST_PATH, SP_COPY_NOOVERWRITE, 
				g_InfList[nInfIndex]->asDesPath, MAX_PATH, 
				NULL, &g_InfList[nInfIndex]->asDesName))
		{
			if(GetLastError() == ERROR_FILE_EXISTS)
				g_InfList[nInfIndex]->nResultInstall = 1;
			else
				g_InfList[nInfIndex]->nResultInstall = -1;
		}else
		{
			UDBG(g_InfList[nInfIndex]->asDesPath);
			g_InfList[nInfIndex]->nResultInstall = 0;
		}

		for(int i = 0; i < g_nHIDCount; i++)
		{
			if(g_HIDInfo[i]->nInfNum == nInfIndex)
			{
				g_HIDInfo[i]->nNumCone = 0;
				g_HIDInfo[i]->nNumNotCone = 0;
				g_HIDInfo[i]->bNeedUpdate = false;
			}
		}

		// ��ȡ�豸ʵ����
		if(g_InfList[nInfIndex]->nResultInstall == 0 && g_InfList[nInfIndex]->asClassName[0])
		{

			UDBG(g_InfList[nInfIndex]->asClassName);

			// Ѱ�������豸������FlagֵΪDIGCF_ALLCLASSES��
			dev_info = SetupDiGetClassDevs(NULL, NULL, NULL, DIGCF_ALLCLASSES);

			// class����Ч
			if(dev_info == INVALID_HANDLE_VALUE){
				UDBG("class ����Ч");
				continue;
			}

			nDevIndex = 0;

			// ö���豸
			dev_info_data.cbSize = sizeof(dev_info_data);
			while(SetupDiEnumDeviceInfo(dev_info, nDevIndex, &dev_info_data))
			{
				// ȡ�豸ID
				if(SetupDiGetDeviceRegistryProperty(dev_info, &dev_info_data,
					SPDRP_HARDWAREID, 
					NULL,
					(BYTE *)tmp_id,
					sizeof(tmp_id), 
					NULL))
				{
					//��õ���һ���ַ����б��ʶ���Ҫ����
					for(p = tmp_id; *p; p += (strlen(p) + 1))
					{
						strlwr(p);
						for(int x = 0; x < g_nHIDCount; x++)
						{
							if(g_HIDInfo[x]->nInfNum != nInfIndex)
								continue;

							if(strstr(p, g_HIDInfo[x]->asHID)){
								g_HIDInfo[x]->bNeedUpdate = true;// ���豸�Ѵ��ڣ���Ҫ���¡�
								if(CM_Get_DevNode_Status(&status,
									&problem,
									dev_info_data.DevInst,
									0) == CR_NO_SUCH_DEVINST)
								{
									g_HIDInfo[x]->nNumNotCone++;
								}
								else
									g_HIDInfo[x]->nNumCone++;
							}
						}
					}
				}

				nDevIndex++;
			}

			SetupDiDestroyDeviceInfoList(dev_info);
		}
	}	
}

// ������������
int UpdateDriver()
{
	HDEVINFO dev_info;
	SP_DEVINFO_DATA dev_info_data;
	INFCONTEXT infcont;
	HINF hInf = NULL;
	DWORD config_flags, problem, status;
	BOOL reboot;
	char inf_path[MAX_PATH];
	char id[MAX_PATH];
	char tmp_id[MAX_PATH];
	char *p;
	int dev_index;

	dev_info_data.cbSize = sizeof(SP_DEVINFO_DATA);

	__try
	{
		for(int i = 0; i < g_nHIDCount; i++)
		{
			if(g_HIDInfo[i]->bWillBeUpdate)
			{
				if(g_HIDInfo[i]->nNumCone)
				{
					// Ϊ��ǰ���ӵ�ϵͳ�������豸��������
					g_HIDInfo[i]->bSuccess = UpdateDriverForPlugAndPlayDevices(NULL, g_HIDInfo[i]->asHID, 
											g_InfList[g_HIDInfo[i]->nInfNum]->asDesPath, 
											INSTALLFLAG_FORCE, 
											&reboot);
				}

				if(g_HIDInfo[i]->nNumNotCone)
				{
					dev_index = 0;

					// Ѱ�������豸������FlagֵΪDIGCF_ALLCLASSES��
					dev_info = SetupDiGetClassDevs(NULL, NULL, NULL, DIGCF_ALLCLASSES);

					if(dev_info == INVALID_HANDLE_VALUE)
						continue;

					// ö���豸
					while(SetupDiEnumDeviceInfo(dev_info, dev_index, &dev_info_data))
					{
						// ��ȡ�豸ID���ж��Ƿ��ǵ�ǰ�����豸
						if(SetupDiGetDeviceRegistryProperty(dev_info, &dev_info_data,
							SPDRP_HARDWAREID, NULL,
							(BYTE *)tmp_id,
							sizeof(tmp_id), 
							NULL))
						{
							//��õ���һ���ַ����б��ʶ���Ҫ����
							for(p = tmp_id; *p; p += (strlen(p) + 1))
							{
								strlwr(p);

								if(strstr(p, id))
								{
									// �жϴ��豸�ǲ��ǵ�ǰδ������ϵͳ
									if(CM_Get_DevNode_Status(&status,
										&problem,
										dev_info_data.DevInst,
										0) == CR_NO_SUCH_DEVINST)
									{
										// ȡ��ǰ����ֵ
										if(SetupDiGetDeviceRegistryProperty(dev_info,
											&dev_info_data,
											SPDRP_CONFIGFLAGS,
											NULL,
											(BYTE *)&config_flags,
											sizeof(config_flags),
											NULL))
										{
											// ��CONFIGFLAG_REINSTALL��
											config_flags |= CONFIGFLAG_REINSTALL;

											// ���µ�����ֵд��
											SetupDiSetDeviceRegistryProperty(dev_info,
												&dev_info_data,
												SPDRP_CONFIGFLAGS,
												(BYTE *)&config_flags,
												sizeof(config_flags));
										}
									}
									
									// �ҵ�һ�����ɣ�������
									break;
								}
							}
						}

						dev_index++;
					}

					SetupDiDestroyDeviceInfoList(dev_info);
				}
			}
		}		

	}
	__finally
	{
	}

	return 0;
}
