
#include "CY001Drv.h"
#include "newdelete.h"

// ��������
REGISTER_DRV_CLASS(CY001Drv)

VOID CY001Drv::PnpSurpriseRemove()
{
	CompleteSyncRequest(SURPRISE_REMOVE, 0);
}

// 
// �������С�
// Ϊ����ʾ�����Ǵ����˶������͵Ķ��С�
NTSTATUS CY001Drv::CreateWDFQueues()
{
	NTSTATUS status = STATUS_SUCCESS;

	WDF_IO_QUEUE_CONFIG ioQConfig;
	KDBG(DPFLTR_INFO_LEVEL, "CY001Drv:[CreateWDFQueues]");

	// �������Queue����
	// ע�����1. ����WdfIoQueueCreateʱ���һ������Device��Ĭ��ΪQueue����ĸ����󣬸�������ά���Ӷ�����������ڡ�
	//				���򲻱�ά��Queue������ֻ�贴�����������١�
	//			 2. �����������ĸ������ڲ���WDF_OBJECT_ATTRIBUTES�����á�
	//			 3. Queue���������У����С����С��ֶ����֡�
	//			 4. ִ��WdfDeviceConfigureRequestDispatching����ĳһ���͵������Ŷӵ���Queue��

	// ����Ĭ�ϲ������У�����DeviceIOControl���
	WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&ioQConfig, WdfIoQueueDispatchParallel);
	ioQConfig.EvtIoDeviceControl = DeviceIoControlParallel_sta;
	status = WdfIoQueueCreate(m_hDevice, &ioQConfig, WDF_NO_OBJECT_ATTRIBUTES, &m_hIoCtlEntryQueue);
	if(!NT_SUCCESS(status))
	{
		KDBG(DPFLTR_INFO_LEVEL, "WdfIoQueueCreate failed with status 0x%08x\n", status);
		return status;
	}

	// �ڶ������У����л����У�����Ĭ�϶���ת����������Ҫ���д�������
	WDF_IO_QUEUE_CONFIG_INIT(&ioQConfig, WdfIoQueueDispatchSequential);
	ioQConfig.EvtIoDeviceControl = DeviceIoControlSerial_sta;
	status = WdfIoQueueCreate(m_hDevice, &ioQConfig, WDF_NO_OBJECT_ATTRIBUTES, &m_hIoCtlSerialQueue);
	if(!NT_SUCCESS(status))
	{
		KDBG(DPFLTR_INFO_LEVEL, "WdfIoQueueCreate failed with status 0x%08x\n", status);
		return status;
	}

	// ���������У����л����У���д������
	WDF_IO_QUEUE_CONFIG_INIT(&ioQConfig, WdfIoQueueDispatchSequential);
	ioQConfig.EvtIoWrite = BulkWrite_sta;
	ioQConfig.EvtIoRead  = BulkRead_sta;
	status = WdfIoQueueCreate(m_hDevice, &ioQConfig, WDF_NO_OBJECT_ATTRIBUTES,	&m_hIoRWQueue);
	if(!NT_SUCCESS(status))
	{
		KDBG(DPFLTR_INFO_LEVEL, "WdfIoQueueCreate failed with status 0x%08x\n", status);
		return status;
	}

	// ���õ��������У�ֻ���ܶ���д��������
	status  = WdfDeviceConfigureRequestDispatching(m_hDevice, m_hIoRWQueue, WdfRequestTypeWrite);
	if(!NT_SUCCESS(status))
	{
		KDBG(DPFLTR_INFO_LEVEL, "WdfDeviceConfigureRequestDispatching failed with status 0x%08x\n", status);
		return status;
	}
	status  = WdfDeviceConfigureRequestDispatching(m_hDevice, m_hIoRWQueue, WdfRequestTypeRead);
	if(!NT_SUCCESS(status))
	{
		KDBG(DPFLTR_INFO_LEVEL, "WdfDeviceConfigureRequestDispatching failed with status 0x%08x\n", status);
		return status;
	}

	// ���ĸ����У��ֶ����С���������б���Ӧ�ó�������������ͬ��������
	// ����������Ϣ֪ͨӦ�ó���ʱ�������ֶ���������ȡһ���������֮��
	WDF_IO_QUEUE_CONFIG_INIT(&ioQConfig, WdfIoQueueDispatchManual);
	status = WdfIoQueueCreate(m_hDevice, &ioQConfig, WDF_NO_OBJECT_ATTRIBUTES, &m_hAppSyncManualQueue);
	if(!NT_SUCCESS(status))
	{
		KDBG(DPFLTR_INFO_LEVEL, "WdfIoQueueCreate for manual queue failed with status 0x%08x\n", status);
		return status;
	}

	// ��������У��ֶ����С�����������ڴ����ж����ݣ��������յ��ж�����ʱ���ʹӶ�������ȡһ���������֮��
	// ��ꡢ���̵��ж��豸������������ʽ��
	WDF_IO_QUEUE_CONFIG_INIT(&ioQConfig, WdfIoQueueDispatchManual);
	status = WdfIoQueueCreate(m_hDevice, &ioQConfig, WDF_NO_OBJECT_ATTRIBUTES, &m_hInterruptManualQueue);
	if(!NT_SUCCESS(status))
	{
		KDBG(DPFLTR_INFO_LEVEL, "WdfIoQueueCreate for manual queue failed with status 0x%08x\n", status);
		return status;
	}

	return status;
}

//��������Power�ص�����WDM�е�PnpSetPower���ơ�
NTSTATUS CY001Drv::PwrD0Entry(IN WDF_POWER_DEVICE_STATE  PreviousState)
{
	KDBG(DPFLTR_INFO_LEVEL, "CY001Drv:[PwrD0Entry] from %s", PowerName(PreviousState));

	if(PreviousState == PowerDeviceD2)
	{
		SetDigitron(byPre7Seg);
		SetLEDs(byPreLEDs);
	}

	CompleteSyncRequest(ENTERD0, PreviousState);

	return STATUS_SUCCESS;
}

// �뿪D0״̬
NTSTATUS CY001Drv::PwrD0Exit(IN WDF_POWER_DEVICE_STATE  TargetState)
{
	KDBG(DPFLTR_INFO_LEVEL, "CY001Drv:[PwrD0Exit] to %s", PowerName(TargetState));

	if(TargetState == PowerDeviceD2)
	{		
		GetDigitron(&byPre7Seg);
		GetLEDs(&byPreLEDs);
	}

	CompleteSyncRequest(EXITD0, TargetState);

	// ֹͣ�ж϶�����
	InterruptReadStop();
	ClearSyncQueue();
	return STATUS_SUCCESS;
}

//
// �豸���úú󣬽ӿڡ��ܵ����Ѵ����ˡ�
NTSTATUS CY001Drv::GetUsbPipes()
{
	KDBG(DPFLTR_INFO_LEVEL, "CY001Drv:[GetUsbPipes]");

	m_hUsbCtlPipe = NULL;
	m_hUsbIntOutPipe = NULL;
	m_hUsbIntInPipe = NULL;
	m_hUsbBulkInPipe = NULL;
	m_hUsbBulkOutPipe = NULL;

	BYTE index = 0;
	WDFUSBPIPE pipe = NULL;
	WDF_USB_PIPE_INFORMATION pipeInfo;

	WDF_USB_PIPE_INFORMATION_INIT(&pipeInfo);

	while(TRUE)
	{
		pipe = WdfUsbInterfaceGetConfiguredPipe(m_hUsbInterface, index, &pipeInfo);
		if(NULL == pipe)break;

		// Dump �ܵ���Ϣ
		KDBG(DPFLTR_INFO_LEVEL, "Type:%s\r\nEndpointAddress:0x%x\r\nMaxPacketSize:%d\r\nAlternateValue:%d", 
			pipeInfo.PipeType == WdfUsbPipeTypeInterrupt ? "Interrupt" : 
			pipeInfo.PipeType == WdfUsbPipeTypeBulk ? "Bulk" :  
			pipeInfo.PipeType == WdfUsbPipeTypeControl ? "Control" : 
			pipeInfo.PipeType == WdfUsbPipeTypeIsochronous ? "Isochronous" : "Invalid!!",
			pipeInfo.EndpointAddress, 
			pipeInfo.MaximumPacketSize,
			pipeInfo.SettingIndex);

		// ���ùܵ����ԣ����԰����ȼ��
		// ��������ã���ôÿ�ζԹܵ�����д������ʱ�����뻺�����ĳ��ȱ�����
		// pipeInfo.MaximumPacketSize������������������ʧ�ܡ�
		// ����ṩ����������飬�ɱ������������߻�ȡ�����벻����������Ϣ�������Ǵ˴����ԡ�
		WdfUsbTargetPipeSetNoMaximumPacketSizeCheck(pipe);

		if(WdfUsbPipeTypeControl == pipeInfo.PipeType)
		{
			m_hUsbCtlPipe = pipe;
		}
		else if(WdfUsbPipeTypeInterrupt == pipeInfo.PipeType)
		{ 
			if(TRUE == WdfUsbTargetPipeIsInEndpoint(pipe))
				m_hUsbIntInPipe = pipe;
			else
				m_hUsbIntOutPipe = pipe;
		}
		else if(WdfUsbPipeTypeBulk == pipeInfo.PipeType)
		{
			if(TRUE == WdfUsbTargetPipeIsInEndpoint(pipe))
			{
				m_hUsbBulkInPipe = pipe;
			}
			else if(TRUE == WdfUsbTargetPipeIsOutEndpoint(pipe))
			{
				m_hUsbBulkOutPipe = pipe;
			}
		}

		index++;
	}

	// ͨ���ܵ��жϹ̼��汾
	if((NULL == m_hUsbIntOutPipe)  ||
		(NULL == m_hUsbIntInPipe)  ||
		(NULL == m_hUsbBulkInPipe) ||
		(NULL == m_hUsbBulkOutPipe))
	{
		KDBG(DPFLTR_ERROR_LEVEL, "Not our CY001 firmware!!!");
	}

	return STATUS_SUCCESS;
}