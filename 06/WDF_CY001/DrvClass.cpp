
extern "C"{
#include "public.h"
};

#include "DrvClass.h"

// ��ӿ��豸���ó�ʼ��Alterֵ
UCHAR MultiInterfaceSettings[MAX_INTERFACES] = {1, 1, 1, 1, 1, 1, 1, 1};

// USB���߽ӿ�GUID
// {B1A96A13-3DE0-4574-9B01-C08FEAB318D6}
static GUID USB_BUS_INTERFACE_USBDI_GUID1 =
{ 0xb1a96a13, 0x3de0, 0x4574, {0x9b, 0x1, 0xc0, 0x8f, 0xea, 0xb3, 0x18, 0xd6}};

// �á�GUID Generator����������һ��Ψһ��GUID��
static	GUID DeviceInterface = 
{0xdb713b3f, 0xea3f, 0x4d74, {0x89, 0x46, 0x0, 0x64, 0xdb, 0xa4, 0xfc, 0x6a}};

DrvClass* DrvClass::GetDrvClassFromDriver(WDFDRIVER Driver)
{
	PDRIVER_CONTEXT pContext = GetDriverContext(Driver);
	return (DrvClass*)pContext->par1;
}

DrvClass* DrvClass::GetDrvClassFromDevice(WDFDEVICE Device)
{
	PDEVICE_CONTEXT pContext = GetDeviceContext(Device);
	return (DrvClass*)pContext->par1;
}

NTSTATUS DrvClass::PnpAdd_sta(IN WDFDRIVER Driver, IN PWDFDEVICE_INIT DeviceInit)
{
	PDRIVER_CONTEXT pContext = GetDriverContext(Driver);
	DrvClass* pThis = (DrvClass*)pContext->par1;
	return pThis->PnpAdd(DeviceInit);
}

NTSTATUS DrvClass::PnpPrepareHardware_sta( 
					IN WDFDEVICE Device, 
					IN WDFCMRESLIST ResourceList, 
					IN WDFCMRESLIST ResourceListTranslated 
					)
{
	DrvClass* pThis = DrvClass::GetDrvClassFromDevice(Device);
	ASSERT(pThis);
	return pThis->PnpPrepareHardware(ResourceList, ResourceListTranslated);
}

NTSTATUS DrvClass::PnpReleaseHardware_sta(IN WDFDEVICE Device, 
										IN WDFCMRESLIST ResourceList)
{
	DrvClass* pThis = DrvClass::GetDrvClassFromDevice(Device);
	ASSERT(pThis);
	return pThis->PnpReleaseHardware(ResourceList);
}

VOID 
DrvClass::PnpSurpriseRemove_sta( IN WDFDEVICE  Device)
{
	DrvClass* pThis = DrvClass::GetDrvClassFromDevice(Device);
	ASSERT(pThis);
	pThis->PnpSurpriseRemove();

}

VOID DrvClass::PnpSurpriseRemove()
{
}

NTSTATUS 
DrvClass::PwrD0Entry_sta(
			   IN WDFDEVICE  Device, 
			   IN WDF_POWER_DEVICE_STATE  PreviousState
			   )
{
	DrvClass* pThis = DrvClass::GetDrvClassFromDevice(Device);
	ASSERT(pThis);
	return pThis->PwrD0Entry(PreviousState);
}

NTSTATUS 
DrvClass::PwrD0Exit_sta(
						 IN WDFDEVICE  Device, 
						 IN WDF_POWER_DEVICE_STATE  PreviousState
						 )
{
	DrvClass* pThis = DrvClass::GetDrvClassFromDevice(Device);
	ASSERT(pThis);
	return pThis->PwrD0Exit(PreviousState);
}

void DrvClass::InitPnpPwrEvents(WDF_PNPPOWER_EVENT_CALLBACKS* pPnpPowerCallbacks)
{
	pPnpPowerCallbacks->EvtDevicePrepareHardware	= PnpPrepareHardware_sta; // ��ʼ��
	pPnpPowerCallbacks->EvtDeviceReleaseHardware	= PnpReleaseHardware_sta; // ֹͣ
	pPnpPowerCallbacks->EvtDeviceSurpriseRemoval	= PnpSurpriseRemove_sta;  // �쳣�Ƴ�
	pPnpPowerCallbacks->EvtDeviceD0Entry	= PwrD0Entry_sta; // ����D0��Դ״̬������״̬����������β��롢���߻���
	pPnpPowerCallbacks->EvtDeviceD0Exit		= PwrD0Exit_sta;  // �뿪D0��Դ״̬������״̬�����������߻��豸�Ƴ�
}

void DrvClass::InitPnpCap(WDF_DEVICE_PNP_CAPABILITIES* pPnpCapabilities)
{
	pPnpCapabilities->Removable = WdfTrue;
	pPnpCapabilities->SurpriseRemovalOK = WdfTrue;
}

//
// �����൱��WDM�е�AddDevice������
// ����PNP�������׵���屻���õĻص���������Ҫ�����Ǵ����豸����
// �豸������ϵͳ���������ʽ�������ⲿ��ĳ������Ӳ���豸��
//NTSTATUS PnpAdd(IN WDFDRIVER  Driver, IN PWDFDEVICE_INIT  DeviceInit)
NTSTATUS DrvClass::PnpAdd(IN PWDFDEVICE_INIT DeviceInit)
{
	NTSTATUS status;
	WDFDEVICE device;
	int nInstance = 0;
	PDEVICE_CONTEXT pContext = NULL;
	UNICODE_STRING DeviceName;
	UNICODE_STRING DosDeviceName;
	UNICODE_STRING RefString;
	WDFSTRING	   SymbolName;
	WDF_OBJECT_ATTRIBUTES attributes;
	WDF_PNPPOWER_EVENT_CALLBACKS pnpPowerCallbacks;
	WDF_DEVICE_PNP_CAPABILITIES pnpCapabilities;

	WCHAR wcsDeviceName[] = L"\\Device\\CY001-0";		// �豸��
	WCHAR wcsDosDeviceName[] = L"\\DosDevices\\CY001-0";// ����������
	WCHAR wcsRefString[] = L"CY001-0";					// ����������
	int	  nLen = wcslen(wcsDeviceName);

	KDBG(DPFLTR_INFO_LEVEL, "[PnpAdd]");

	RtlInitUnicodeString(&DeviceName, wcsDeviceName) ;
	RtlInitUnicodeString(&DosDeviceName, wcsDosDeviceName);
	RtlInitUnicodeString(&RefString, wcsRefString);

	WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&pnpPowerCallbacks);
	InitPnpPwrEvents(&pnpPowerCallbacks);
	WdfDeviceInitSetPnpPowerEventCallbacks(DeviceInit, &pnpPowerCallbacks);

	// ����д����Ļ��巽ʽ
	// Ĭ��ΪBuffered��ʽ���������ַ�ʽ��Direct��Neither��
	WdfDeviceInitSetIoType(DeviceInit, WdfDeviceIoBuffered);

	// �趨�豸�����鳤��
	// ���ڲ������sizeof(DEVICE_CONTEXT)��ṹ�峤��
	WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, DEVICE_CONTEXT);

	// ����֧�����8��ʵ���������������������PC��ʱ�����������Ǹ��貢��֧�֡�
	// ��ͬ���豸���󣬸��������Ƶ�β����0-7����𣬲�����β�������豸ID��
	// ����Ĵ����߼�Ϊ��ǰ�豸����Ѱ��һ������ID��
	for(nInstance = 0; nInstance < MAX_INSTANCE_NUMBER; nInstance++)
	{
		// �޸��豸ID
		wcsDeviceName[nLen-1] += nInstance;

		// ������������������ʧ�ܣ�ʧ�ܵ�ԭ�������������Ч�������Ѵ��ڵ�
		WdfDeviceInitAssignName(DeviceInit, &DeviceName);

		// ����WDF�豸
		// ���ܳɹ�����������ɹ���
		status = WdfDeviceCreate(&DeviceInit, &attributes, &device);  
		if(!NT_SUCCESS(status))
		{
			if(status == STATUS_OBJECT_NAME_COLLISION)// ���ֳ�ͻ
			{
				KDBG(DPFLTR_ERROR_LEVEL, "Already exist: %wZ", &DeviceName);
			}
			else
			{
				KDBG(DPFLTR_ERROR_LEVEL, "WdfDeviceCreate failed with status 0x%08x!!!", status);
				return status;
			}
		}else
		{
			KdPrint(("Device name: %wZ", &DeviceName));
			break;
		}
	}

	// ��ʧ�ܣ����������ӵĿ���������̫�ࣻ
	// ����ʹ��WinOBJ�鿴ϵͳ�е��豸���ơ�
	if (!NT_SUCCESS(status)) 
		return status;

	m_hDevice = device;// �����豸������

	// �����������ӣ�Ӧ�ó�����ݷ������Ӳ鿴��ʹ���ں��豸��
	// ���˴������������⣬���ڸ��õķ�����ʹ��WdfDeviceCreateDeviceInterface�����豸�ӿڡ�
	// �豸�ӿ��ܱ�֤���ֲ����ͻ���������пɶ��ԣ����������Բ��÷���������ʽ��
	nLen = wcslen(wcsDosDeviceName);
	wcsDosDeviceName[nLen-1] += nInstance;

	status = WdfDeviceCreateSymbolicLink(device, &DosDeviceName);
	if(!NT_SUCCESS(status))
	{
		KDBG(DPFLTR_INFO_LEVEL, "Failed to create symbol link: 0x%08X", status);
		return status;
	}

	// ��ʼ��������
	// GetDeviceContext��һ������ָ�룬�ɺ�WDF_DECLARE_CONTEXT_TYPE_WITH_NAME���塣
	// �ο�pulibc.h�е�˵����
	pContext = GetDeviceContext(device);
	RtlZeroMemory(pContext, sizeof(DEVICE_CONTEXT));
	pContext->par1 = this; // ͨ��thisָ����Եõ�һ��

	// PNP���ԡ������豸�쳣�γ���ϵͳ���ᵯ�������
	WDF_DEVICE_PNP_CAPABILITIES_INIT(&pnpCapabilities);
	InitPnpCap(&pnpCapabilities);
	WdfDeviceSetPnpCapabilities(device, &pnpCapabilities);

	status = CreateWDFQueues();
	if(!NT_SUCCESS(status))
		return status;

	return status;
}

// 
// �������С�
NTSTATUS DrvClass::CreateWDFQueues()
{
	NTSTATUS status = STATUS_SUCCESS;

	WDF_IO_QUEUE_CONFIG ioQConfig;
	KDBG(DPFLTR_INFO_LEVEL, "[CreateWDFQueues]");

	// ����Ĭ�ϲ������У�����DeviceIOControl���
	WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&ioQConfig, WdfIoQueueDispatchParallel);
	ioQConfig.EvtIoDefault  = EventDefault_sta;
	status = WdfIoQueueCreate(m_hDevice, &ioQConfig, WDF_NO_OBJECT_ATTRIBUTES, &m_hDefQueue);
	if(!NT_SUCCESS(status))
	{
		KDBG(DPFLTR_INFO_LEVEL, "WdfIoQueueCreate failed with status 0x%08x\n", status);
	}

	return status;
}

VOID DrvClass::EventDefault_sta(IN WDFQUEUE  Queue, IN WDFREQUEST  Request)
{
	DrvClass* pThis = GetDrvClassFromDevice(WdfIoQueueGetDevice(Queue));
	pThis->EventDefault(Queue, Request);
}

VOID DrvClass::EventDefault(IN WDFQUEUE  Queue, IN WDFREQUEST  Request)
{
	// Ĭ�ϵĴ���ʽ�ǣ�����������е��������
	WdfRequestComplete(Request, STATUS_SUCCESS);
}

// �˺���������WDM�е�PNP_MN_START_DEVICE������������PnpAdd֮�󱻵��á�
// ��ʱPNP�������������֮���Ѿ���������Щϵͳ��Դ�������ǰ�豸��
// ����ResourceList��ResourceListTranslated��������Щϵͳ��Դ��
// ��������������ʱ���豸�Ѿ�������D0��Դ״̬��������ɺ��豸����ʽ���빤��״̬��
NTSTATUS DrvClass::PnpPrepareHardware(IN WDFCMRESLIST ResourceList, IN WDFCMRESLIST ResourceListTranslated)
{
	NTSTATUS status;
	PDEVICE_CONTEXT pContext = NULL; 
	ULONG ulNumRes = 0;
	ULONG ulIndex;
	CM_PARTIAL_RESOURCE_DESCRIPTOR*  pResDes = NULL;

	KDBG(DPFLTR_INFO_LEVEL, "[PnpPrepareHardware]");

	pContext = GetDeviceContext(m_hDevice);

	// �����豸
	status = ConfigureUsbDevice();
	if(!NT_SUCCESS(status))
		return status;

	// ��ȡPipe���
	status = GetUsbPipes();
	if(!NT_SUCCESS(status))
		return status;

	// ��ʼ��Դ���ԣ�
	status = InitPowerManagement();

	// ��ȡUSB���������ӿڡ����߽ӿ��а������������ṩ�Ļص�������������Ϣ��
	// ���߽ӿ���ϵͳ����GUID��ʶ��
	status = WdfFdoQueryForInterface(
		m_hDevice,
		&USB_BUS_INTERFACE_USBDI_GUID1,		// ���߽ӿ�ID
		(PINTERFACE)&m_busInterface,		// �������豸��������
		sizeof(USB_BUS_INTERFACE_USBDI_V1),
		1, NULL);

	// ���ýӿں������ж�USB�汾��
	if(NT_SUCCESS(status) && m_busInterface.IsDeviceHighSpeed){
		if(m_busInterface.IsDeviceHighSpeed(m_busInterface.BusContext))
			KDBG(DPFLTR_INFO_LEVEL, "USB 2.0");
		else
			KDBG(DPFLTR_INFO_LEVEL, "USB 1.1");
	}

	// ��ϵͳ��Դ�б����ǲ���ʵ���ԵĲ�����������ӡһЩ��Ϣ��
	// ������ȫ���԰��������Щ���붼ע�͵���
	ulNumRes = WdfCmResourceListGetCount(ResourceList);
	KDBG(DPFLTR_INFO_LEVEL, "ResourceListCount:%d\n", ulNumRes);

	// ���������������µ�ö����Щϵͳ��Դ������ӡ�����ǵ����������Ϣ��
	for(ulIndex = 0; ulIndex < ulNumRes; ulIndex++)
	{
		pResDes = WdfCmResourceListGetDescriptor(ResourceList, ulIndex);		
		if(!pResDes)continue; // ȡ��ʧ�ܣ���������һ��

		switch (pResDes->Type) 
		{
		case CmResourceTypeMemory:
			KDBG(DPFLTR_INFO_LEVEL, "System Resource��CmResourceTypeMemory\n");
			break;

		case CmResourceTypePort:
			KDBG(DPFLTR_INFO_LEVEL, "System Resource��CmResourceTypePort\n");
			break;

		case CmResourceTypeInterrupt:
			KDBG(DPFLTR_INFO_LEVEL, "System Resource��CmResourceTypeInterrupt\n");
			break;

		default:
			KDBG(DPFLTR_INFO_LEVEL, "System Resource��Others %d\n", pResDes->Type);
			break;
		}
	}

	return STATUS_SUCCESS;
}

// �˺���������WDM�е�PNP_MN_STOP_DEVICE���������豸�Ƴ�ʱ�����á�
// ��������������ʱ���豸�Դ��ڹ���״̬��
NTSTATUS DrvClass::PnpReleaseHardware(IN WDFCMRESLIST ResourceListTranslated)
{
	KDBG(DPFLTR_INFO_LEVEL, "[PnpReleaseHardware]");

	// ���PnpPrepareHardware����ʧ��,UsbDeviceΪ�գ�
	// ��ʱ��ֱ�ӷ��ؼ��ɡ�
	if (m_hUsbDevice == NULL)
		return STATUS_SUCCESS;

	// ȡ��USB�豸������IO��������������ȡ������Pipe��IO������
	WdfIoTargetStop(WdfUsbTargetDeviceGetIoTarget(m_hUsbDevice), WdfIoTargetCancelSentIo);

	// Deconfiguration���ߡ������á�
	WDF_USB_DEVICE_SELECT_CONFIG_PARAMS  configParams;
	WDF_USB_DEVICE_SELECT_CONFIG_PARAMS_INIT_DECONFIG(&configParams);
	return WdfUsbTargetDeviceSelectConfig(m_hUsbDevice, WDF_NO_OBJECT_ATTRIBUTES, &configParams);
}

// �����豸�����ĵ�Դ������
NTSTATUS DrvClass::InitPowerManagement()
{
	NTSTATUS status = STATUS_SUCCESS;
	WDF_USB_DEVICE_INFORMATION usbInfo;

	KDBG(DPFLTR_INFO_LEVEL, "[InitPowerManagement]");

	// ��ȡ�豸��Ϣ
	WDF_USB_DEVICE_INFORMATION_INIT(&usbInfo);
	WdfUsbTargetDeviceRetrieveInformation(m_hUsbDevice, &usbInfo);

	// USB�豸��Ϣ��������ʽ��������Traits�С�
	KDBG( DPFLTR_INFO_LEVEL, "Device self powered: %s", 
		usbInfo.Traits & WDF_USB_DEVICE_TRAIT_SELF_POWERED ? "TRUE" : "FALSE");
	KDBG( DPFLTR_INFO_LEVEL, "Device remote wake capable: %s",
		usbInfo.Traits & WDF_USB_DEVICE_TRAIT_REMOTE_WAKE_CAPABLE ? "TRUE" : "FALSE");
	KDBG( DPFLTR_INFO_LEVEL, "Device high speed: %s",
		usbInfo.Traits & WDF_USB_DEVICE_TRAIT_AT_HIGH_SPEED ? "TRUE" : "FALSE");

	m_bIsDeviceHighSpeed = usbInfo.Traits & WDF_USB_DEVICE_TRAIT_AT_HIGH_SPEED;

	// �����豸�����ߺ�Զ�̻��ѹ���
	if(usbInfo.Traits & WDF_USB_DEVICE_TRAIT_REMOTE_WAKE_CAPABLE)
	{
		WDF_DEVICE_POWER_POLICY_IDLE_SETTINGS idleSettings;
		WDF_DEVICE_POWER_POLICY_WAKE_SETTINGS wakeSettings;

		// �����豸Ϊ��ʱ���ߡ���ʱ����10S���Զ���������״̬��
		WDF_DEVICE_POWER_POLICY_IDLE_SETTINGS_INIT(&idleSettings, IdleUsbSelectiveSuspend);
		idleSettings.IdleTimeout = 10000;
		status = WdfDeviceAssignS0IdleSettings(m_hDevice, &idleSettings);
		if(!NT_SUCCESS(status))
		{
			KDBG( DPFLTR_ERROR_LEVEL, "WdfDeviceAssignS0IdleSettings failed with status 0x%0.8x!!!", status);
			return status;
		}

		// ����Ϊ��Զ�̻��ѡ������豸�����������Ѿ���PCϵͳ�������ߺ��豸���Խ�ϵͳ���ѣ��������档
		WDF_DEVICE_POWER_POLICY_WAKE_SETTINGS_INIT(&wakeSettings);
		status = WdfDeviceAssignSxWakeSettings(m_hDevice, &wakeSettings);
		if(!NT_SUCCESS(status))
		{
			KDBG( DPFLTR_ERROR_LEVEL, "WdfDeviceAssignSxWakeSettings failed with status 0x%0.8x!!!", status);
			return status;
		}
	}

	return status;
}

//��������Power�ص�����WDM�е�PnpSetPower���ơ�
NTSTATUS DrvClass::PwrD0Entry(IN WDF_POWER_DEVICE_STATE  PreviousState)
{
	KDBG(DPFLTR_INFO_LEVEL, "[PwrD0Entry] from %s", PowerName(PreviousState));
	return STATUS_SUCCESS;
}

// �뿪D0״̬
NTSTATUS DrvClass::PwrD0Exit(IN WDF_POWER_DEVICE_STATE  TargetState)
{
	KDBG(DPFLTR_INFO_LEVEL, "[PwrD0Exit] to %s", PowerName(TargetState));
	return STATUS_SUCCESS;
}

// ��ʼ���ṹ��WDF_USB_INTERFACE_SETTING_PAIR��
// �������ö�ӿ��豸��
int  InitSettingPairs(IN WDFUSBDEVICE UsbDevice,				// �豸����
					  OUT PWDF_USB_INTERFACE_SETTING_PAIR Pairs,// �ṹ��ָ�롣
					  IN ULONG NumSettings						// �ӿڸ���
					  )
{
	int i;

	// ���֧��8���ӿڣ��Ѷ�����е���
	if(NumSettings > MAX_INTERFACES)
		NumSettings = MAX_INTERFACES;

	// ���ýӿ�
	for(i = 0; i < NumSettings; i++) {
		Pairs[i].UsbInterface = WdfUsbTargetDeviceGetInterface(UsbDevice, i);// ���ýӿھ��
		Pairs[i].SettingIndex = MultiInterfaceSettings[i];					 // ���ýӿڿ�ѡֵ(Alternate Setting)
	}

	return NumSettings;
}

// �豸����
// ����WDF��ܣ��豸����ѡ��Ĭ��Ϊ1������ڶ������ѡ���Ҫ�л�ѡ��Ļ�����Ƚ��鷳��
// һ�ְ취�ǣ�ʹ�ó�ʼ���꣺WDF_USB_DEVICE_SELECT_CONFIG_PARAMS_INIT_INTERFACES_DESCRIPTORS 
// ʹ������꣬��Ҫ���Ȼ�ȡ����������������ؽӿ���������
// ��һ�ְ취�ǣ�ʹ��WDM�������ȹ���һ������ѡ���URB��Ȼ��Ҫô�Լ�����IRP���͵�����������
// Ҫôʹ��WDF��������WDF_USB_DEVICE_SELECT_CONFIG_PARAMS_INIT_URB��ʼ���ꡣ
// 
NTSTATUS DrvClass::ConfigureUsbDevice()
{
	WDF_USB_DEVICE_SELECT_CONFIG_PARAMS usbConfig;
	WDF_USB_INTERFACE_SELECT_SETTING_PARAMS  interfaceSelectSetting;

	KDBG(DPFLTR_INFO_LEVEL, "[ConfigureUsbDevice]");

	// ����Usb�豸����
	// USB�豸���������ǽ���USB��������㡣�󲿷ֵ�USB�ӿں�����������������еġ�
	// USB�豸���󱻴������������Լ�ά������ܱ�����������Ҳ����������
	NTSTATUS status = WdfUsbTargetDeviceCreate(m_hDevice, WDF_NO_OBJECT_ATTRIBUTES, &m_hUsbDevice);
	if(!NT_SUCCESS(status))
	{
		KDBG(DPFLTR_INFO_LEVEL, "WdfUsbTargetDeviceCreate failed with status 0x%08x\n", status);
		return status;
	}

	// �ӿ�����
	// WDF�ṩ�˶��ֽӿ����õĳ�ʼ���꣬�ֱ���Ե�һ�ӿڡ���ӿڵ�USB�豸��
	// ��ʼ���껹�ṩ���ڶ�����ü�����л���;�������������������ġ�
	// ��ѡ��Ĭ�����õ�����£��豸���ý��ޱȼ򵥣��򵥵��������ĥ���ں˳���Ա����۾���
	// ��ΪWDM�ϰ��еĴ����߼�������ֻҪ�����о͹��ˡ�
	UCHAR numInterfaces = WdfUsbTargetDeviceGetNumInterfaces(m_hUsbDevice);
	if(1 == numInterfaces)
	{
		KDBG(DPFLTR_INFO_LEVEL, "There is one interface.", status);
		WDF_USB_DEVICE_SELECT_CONFIG_PARAMS_INIT_SINGLE_INTERFACE(&usbConfig);
	}
	else// ��ӿ�
	{
		KDBG(DPFLTR_INFO_LEVEL, "There are %d interfaces.", numInterfaces);

		PWDF_USB_INTERFACE_SETTING_PAIR settingPairs = (WDF_USB_INTERFACE_SETTING_PAIR*)
			ExAllocatePoolWithTag(PagedPool,
				sizeof(WDF_USB_INTERFACE_SETTING_PAIR) * numInterfaces, POOLTAG);

		if (settingPairs == NULL)
			return STATUS_INSUFFICIENT_RESOURCES;

		int nNum = InitSettingPairs(m_hUsbDevice, settingPairs, numInterfaces);
		WDF_USB_DEVICE_SELECT_CONFIG_PARAMS_INIT_MULTIPLE_INTERFACES(&usbConfig, nNum, settingPairs);
	}

	status = WdfUsbTargetDeviceSelectConfig(m_hUsbDevice, WDF_NO_OBJECT_ATTRIBUTES, &usbConfig);
	if(!NT_SUCCESS(status))
	{
		KDBG(DPFLTR_INFO_LEVEL, "WdfUsbTargetDeviceSelectConfig failed with status 0x%08x\n", status);
		return status;
	}

	// ����ӿ�
	if(1 == numInterfaces)
	{
		m_hUsbInterface = usbConfig.Types.SingleInterface.ConfiguredUsbInterface;

		// ʹ��SINGLE_INTERFACE�ӿ����ú꣬�ӿڵ�AltSettingֵĬ��Ϊ0��
		// �������д�����ʾ������ֶ��޸�ĳ�ӿڵ�AltSettingֵ���˴�Ϊ1��.
		WDF_USB_INTERFACE_SELECT_SETTING_PARAMS_INIT_SETTING(&interfaceSelectSetting, 1);
		status = WdfUsbInterfaceSelectSetting(m_hUsbInterface, WDF_NO_OBJECT_ATTRIBUTES, &interfaceSelectSetting);
	}
	else
	{
		int i;
		m_hUsbInterface = usbConfig.Types.MultiInterface.Pairs[0].UsbInterface;
		for(i = 0; i < numInterfaces; i++)
			m_hMulInterfaces[i] = usbConfig.Types.MultiInterface.Pairs[i].UsbInterface;
	}

	return status;
}

//
// �豸���úú󣬽ӿڡ��ܵ����Ѵ����ˡ�
NTSTATUS DrvClass::GetUsbPipes()
{
	return STATUS_SUCCESS;
}


char* PowerName(WDF_POWER_DEVICE_STATE PowerState)
{
	char *name;

	switch(PowerState)
	{
	case WdfPowerDeviceInvalid :	
		name = "PowerDeviceUnspecified"; 
		break;
	case WdfPowerDeviceD0:			
		name = "WdfPowerDeviceD0"; 
		break;
	case WdfPowerDeviceD1:			
		name = "WdfPowerDeviceD1"; 
		break;
	case WdfPowerDeviceD2:		
		name = "WdfPowerDeviceD2"; 
		break;
	case WdfPowerDeviceD3:		
		name = "WdfPowerDeviceD3"; 
		break;
	case WdfPowerDeviceD3Final:	
		name = "WdfPowerDeviceD3Final";
		break;
	case WdfPowerDevicePrepareForHibernation:									
		name = "WdfPowerDevicePrepareForHibernation"; 
		break;
	case WdfPowerDeviceMaximum:		
		name = "WdfPowerDeviceMaximum";
		break;
	default:					
		name = "Unknown Power State";
		break;
	}

	return name;
}