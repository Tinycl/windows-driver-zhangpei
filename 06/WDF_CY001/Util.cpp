/*
	��Ȩ��CY001 WDF USB��������Ŀ  2009/9/1
	
	����ӵ�еĴ˷ݴ��뿽�����������ڸ���ѧϰ���κ���ҵ�����������;����ʹ�ã�����������
	�����δ�������벻Ҫ�������������ϴ�������Ŀ�������ѽ����뷢����������ά����

	���ߣ��������		���� 
		  ����������	����
		  AMD			�ĺ���

	���ڣ�2009/9/1

	�ļ���Util.c
	˵����ʵ�ó���
	��ʷ��
	������
*/

#include "CY001Drv.h"

#define CY001_LOAD_REQUEST    0xA0
#define ANCHOR_LOAD_EXTERNAL  0xA3
#define MAX_INTERNAL_ADDRESS  0x4000
#define INTERNAL_RAM(address) ((address <= MAX_INTERNAL_ADDRESS) ? 1 : 0)

#define MCU_Ctl_REG    0x7F92
#define MCU_RESET_REG  0xE600

// �����ж϶�������Ϊ�����ǿ�����һ��������������
// ����Ӧ������Ȥ�ο���ص�WDM���룬Ϊ��ʵ��������ܣ�WDM��ʵ���൱�������������׳���
NTSTATUS CY001Drv::InterruptReadStart()
{
	NTSTATUS status = STATUS_SUCCESS;
	WDF_USB_CONTINUOUS_READER_CONFIG interruptConfig;
	ASSERT(m_hUsbIntInPipe);

	KDBG(DPFLTR_INFO_LEVEL, "[InterruptReadStart]");

	//WDF_IO_TARGET_STATE state;
	WDF_USB_PIPE_INFORMATION pipeInfo;
	WDF_USB_PIPE_INFORMATION_INIT(&pipeInfo);
	WdfUsbTargetPipeGetInformation(m_hUsbIntInPipe, &pipeInfo);

	// ��ȡpipe IOTarget�ĵ�ǰ״̬
	//WdfIoTargetGetState(WdfUsbTargetPipeGetIoTarget(m_hUsbIntInPipe), &state);

	// Ҫ�жϱ�־λ��
	// �ж�Pipeֻ��Ҫ����һ��continue���þͿ����ˡ���������ܵ���ֹ�����������ض������á�
	if(m_bIntPipeConfigured == FALSE)
	{
		WDF_USB_CONTINUOUS_READER_CONFIG_INIT(&interruptConfig, 
			InterruptRead_sta,			// �ص�����ע�ᡣ���յ�һ�ζ������Ϣ�󣬴˺��������á�
			this,	 					// �ص���������
			pipeInfo.MaximumPacketSize	// ���豸��ȡ���ݵĳ���
			);

		status = WdfUsbTargetPipeConfigContinuousReader(m_hUsbIntInPipe, &interruptConfig);

		if(NT_SUCCESS(status))
			m_bIntPipeConfigured = TRUE;
		else
			KDBG(DPFLTR_INFO_LEVEL, "Error! Status: %08x", status);
	}

	// ����Pipe�������ǵ�һ��������Ҳ�����Ǻ���������
	if(NT_SUCCESS(status))
		status = WdfIoTargetStart(WdfUsbTargetPipeGetIoTarget(m_hUsbIntInPipe));
	else
		KDBG(DPFLTR_INFO_LEVEL, "WdfUsbTargetPipeConfigContinuousReader failed with status 0x%08x", status);

	return status;
}

// ֹͣ�ж϶�����
NTSTATUS CY001Drv::InterruptReadStop()
{
	NTSTATUS status = STATUS_SUCCESS;
	WDFREQUEST Request = NULL;

	if(NULL == m_hUsbIntInPipe)
		return STATUS_SUCCESS;

	KDBG(DPFLTR_INFO_LEVEL, "[InterruptReadStop]");

	if(m_hUsbIntInPipe)
		// �������δ��ɵ�IO��������Cancel����
		WdfIoTargetStop(WdfUsbTargetPipeGetIoTarget(m_hUsbIntInPipe), WdfIoTargetCancelSentIo);

	// ������ֶ������е�����δ���Request��
	// ���Queue����δ����״̬���᷵��STATUS_WDF_PAUSED��
	// �������������ᰤ��ȡ����Entry��ֱ������STATUS_NO_MORE_ENTRIES��	
	do{
		status = WdfIoQueueRetrieveNextRequest(m_hInterruptManualQueue, &Request);

		if(NT_SUCCESS(status))
		{
			WdfRequestComplete(Request, STATUS_SUCCESS);
		}
	}while(status != STATUS_NO_MORE_ENTRIES && status != STATUS_WDF_PAUSED);

	return STATUS_SUCCESS;
}

void CY001Drv::ClearSyncQueue()
{
	NTSTATUS status;
	WDFREQUEST Request = NULL;

	KDBG(DPFLTR_INFO_LEVEL, "[ClearSyncQueue]");

	// ���ͬ�������е�����ͬ��Request���˲����߼������溯����ͬ��
	do{
		status = WdfIoQueueRetrieveNextRequest(m_hAppSyncManualQueue, &Request);

		if(NT_SUCCESS(status))
			WdfRequestComplete(Request, STATUS_SUCCESS);

	}while(status != STATUS_NO_MORE_ENTRIES && status != STATUS_WDF_PAUSED);
}

// ��ͬ��������ȡ��һ����ЧRequest��
NTSTATUS CY001Drv::GetOneSyncRequest(WDFREQUEST* pRequest)
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	ASSERT(pRequest);
	*pRequest = NULL;

	if(m_hAppSyncManualQueue)
		status = WdfIoQueueRetrieveNextRequest(m_hAppSyncManualQueue, pRequest);

	return status;
}

// ���һ��ͬ��Request�����������Ϣ������Request��
void CY001Drv::CompleteSyncRequest(DRIVER_SYNC_ORDER_TYPE type, int info)
{
	NTSTATUS status;
	WDFREQUEST Request;
	if(NT_SUCCESS(GetOneSyncRequest(&Request)))
	{
		PDriverSyncPackt pData = NULL;

		if(!NT_SUCCESS(WdfRequestRetrieveOutputBuffer(Request, sizeof(DriverSyncPackt), (void**)&pData, NULL)))
			WdfRequestComplete(Request, STATUS_INVALID_PARAMETER);
		else{

			// ���Output�ṹ����
			pData->type = type;
			pData->info = info;
			WdfRequestCompleteWithInformation(Request, STATUS_SUCCESS, sizeof(DriverSyncPackt));
		}
	}
}

NTSTATUS CY001Drv::SetDigitron(IN UCHAR chSet)
{
	NTSTATUS status = STATUS_SUCCESS;
	WDF_USB_CONTROL_SETUP_PACKET controlPacket;
	WDF_MEMORY_DESCRIPTOR hMemDes;

	KDBG(DPFLTR_INFO_LEVEL, "[SetDigitron] %d", chSet);
	WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&hMemDes, &chSet, sizeof(UCHAR));

	WDF_USB_CONTROL_SETUP_PACKET_INIT_VENDOR(
		&controlPacket,
		BmRequestHostToDevice,
		BmRequestToDevice,
		0xD2, // Vendor����
		0,
		0);

	status = WdfUsbTargetDeviceSendControlTransferSynchronously(
		m_hUsbDevice,
		NULL, NULL,
		&controlPacket,
		&hMemDes,
		NULL);

	return status;
}

NTSTATUS CY001Drv::GetDigitron(OUT UCHAR* pchGet)
{
	NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;
	WDF_USB_CONTROL_SETUP_PACKET controlPacket;
	WDFMEMORY hMem = NULL;
	WDFREQUEST newRequest = NULL;

	ASSERT(pchGet);
	KDBG(DPFLTR_INFO_LEVEL, "[GetDigitron]");

	// �����ڴ�������
	ntStatus = WdfMemoryCreatePreallocated(WDF_NO_OBJECT_ATTRIBUTES, pchGet, sizeof(UCHAR), &hMem);
	if(!NT_SUCCESS(ntStatus))
		return ntStatus;

	// ��ʼ����������
	WDF_USB_CONTROL_SETUP_PACKET_INIT_VENDOR(
		&controlPacket,
		BmRequestDeviceToHost,// input����
		BmRequestToDevice,
		0xD4,// Vendor ����D4
		0, 0);

	// �����µ�WDF REQUEST����
	ntStatus = WdfRequestCreate(NULL, NULL, &newRequest);
	if(!NT_SUCCESS(ntStatus))
		return ntStatus;

	WdfUsbTargetDeviceFormatRequestForControlTransfer(m_hUsbDevice, newRequest, &controlPacket, hMem, NULL);	

	if(NT_SUCCESS(ntStatus))
	{
		// ͬ������
		WDF_REQUEST_SEND_OPTIONS opt;
		WDF_REQUEST_SEND_OPTIONS_INIT(&opt, WDF_REQUEST_SEND_OPTION_SYNCHRONOUS);
		if(WdfRequestSend(newRequest, WdfDeviceGetIoTarget(m_hDevice), &opt))
		{
			WDF_REQUEST_COMPLETION_PARAMS par;
			WDF_REQUEST_COMPLETION_PARAMS_INIT(&par);
			WdfRequestGetCompletionParams(newRequest, &par);

			// �ж϶�ȡ�����ַ����ȡ�
			if(sizeof(UCHAR) != par.Parameters.Usb.Completion->Parameters.DeviceControlTransfer.Length)
				ntStatus = STATUS_UNSUCCESSFUL;
		}else
			ntStatus = STATUS_UNSUCCESSFUL;
	}

	// ͨ��WdfXxxCreate�����Ķ��󣬱���ɾ��
	WdfObjectDelete(newRequest);

	return ntStatus;
}

NTSTATUS CY001Drv::SetLEDs(IN UCHAR chSet)
{
	NTSTATUS status = STATUS_SUCCESS;
	WDF_USB_CONTROL_SETUP_PACKET controlPacket;
	WDF_MEMORY_DESCRIPTOR hMemDes;

	KDBG(DPFLTR_INFO_LEVEL, "[SetLEDs] %c", chSet);
	WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&hMemDes, &chSet, sizeof(UCHAR));

	WDF_USB_CONTROL_SETUP_PACKET_INIT_VENDOR(
		&controlPacket,
		BmRequestHostToDevice,
		BmRequestToDevice,
		0xD1, // Vendor����
		0, 0);

	status = WdfUsbTargetDeviceSendControlTransferSynchronously(
		m_hUsbDevice,
		NULL, NULL,
		&controlPacket,
		&hMemDes,
		NULL);

	return status;
}

NTSTATUS CY001Drv::GetLEDs(OUT UCHAR* pchGet)
{
	NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;
	WDF_USB_CONTROL_SETUP_PACKET controlPacket;
	WDFMEMORY hMem = NULL;
	WDFREQUEST newRequest = NULL;

	KDBG(DPFLTR_INFO_LEVEL, "[GetLEDs]");
	ASSERT(pchGet);

	// �����ڴ�������
	ntStatus = WdfMemoryCreatePreallocated(WDF_NO_OBJECT_ATTRIBUTES, pchGet, sizeof(UCHAR), &hMem);
	if(!NT_SUCCESS(ntStatus))
		return ntStatus;

	// ��ʼ����������
	WDF_USB_CONTROL_SETUP_PACKET_INIT_VENDOR(
		&controlPacket,
		BmRequestDeviceToHost,// input����
		BmRequestToDevice,
		0xD3,// Vendor ����D3
		0, 0);

	// ����WDF REQUEST����
	ntStatus = WdfRequestCreate(NULL, NULL, &newRequest);
	if(!NT_SUCCESS(ntStatus))
		return ntStatus;

	WdfUsbTargetDeviceFormatRequestForControlTransfer(m_hUsbDevice, newRequest, &controlPacket, hMem, NULL);	

	if(NT_SUCCESS(ntStatus))
	{
		// ͬ������
		WDF_REQUEST_SEND_OPTIONS opt;
		WDF_REQUEST_SEND_OPTIONS_INIT(&opt, WDF_REQUEST_SEND_OPTION_SYNCHRONOUS);
		if(WdfRequestSend(newRequest, WdfDeviceGetIoTarget(m_hDevice), &opt))
		{
			WDF_REQUEST_COMPLETION_PARAMS par;
			WDF_REQUEST_COMPLETION_PARAMS_INIT(&par);
			WdfRequestGetCompletionParams(newRequest, &par);

			// �ж϶�ȡ�����ַ����ȡ�
			if(sizeof(UCHAR) != par.Parameters.Usb.Completion->Parameters.DeviceControlTransfer.Length)
				ntStatus = STATUS_UNSUCCESSFUL;
		}else
			ntStatus = STATUS_UNSUCCESSFUL;
	}

	// ͨ��WdfXxxCreate�����Ķ��󣬱���ɾ��
	WdfObjectDelete(newRequest);

	return ntStatus;
}

NTSTATUS CY001Drv::GetStringDes(USHORT shIndex, USHORT shLanID, VOID* pBufferOutput, ULONG OutputBufferLength, ULONG* pulRetLen)
{
	NTSTATUS status;

	USHORT  numCharacters;
	PUSHORT  stringBuf;
	WDFMEMORY  memoryHandle;

	KDBG(DPFLTR_INFO_LEVEL, "[GetStringDes] index:%d", shIndex);
	ASSERT(pulRetLen);
	*pulRetLen = 0;

	// ����String��������һ���䳤�ַ����飬������ȡ���䳤��
	status = WdfUsbTargetDeviceQueryString(
		m_hUsbDevice,
		NULL, NULL, NULL, // ������ַ���
		&numCharacters,
		shIndex,
		shLanID
		);
	if(!NT_SUCCESS(status))
		return status;

	// �ж��������ĳ���
	if(OutputBufferLength < numCharacters){
		status = STATUS_BUFFER_TOO_SMALL;
		return status;
	}

	// �ٴ���ʽ��ȡ��String������
	status = WdfUsbTargetDeviceQueryString(m_hUsbDevice,
		NULL, NULL,
		(PUSHORT)pBufferOutput,// Unicode�ַ���
		&numCharacters,
		shIndex,
		shLanID
		);

	// ��ɲ���
	if(NT_SUCCESS(status)){
		((PUSHORT)pBufferOutput)[numCharacters] = L'\0';// �ֶ����ַ���ĩβ���NULL
		*pulRetLen = numCharacters+1;
	}
	return status;
}

NTSTATUS CY001Drv::FirmwareReset(IN UCHAR resetBit)
{
	NTSTATUS status = STATUS_SUCCESS;
	WDF_USB_CONTROL_SETUP_PACKET controlPacket;
	WDF_MEMORY_DESCRIPTOR memDescriptor;
	PDEVICE_CONTEXT Context = GetDeviceContext(m_hDevice);

	KDBG(DPFLTR_INFO_LEVEL, "[FirmwareReset]");

	WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&memDescriptor, &resetBit, 1);

	// д��ַMCU_RESET_REG
	WDF_USB_CONTROL_SETUP_PACKET_INIT_VENDOR(
		&controlPacket,
		BmRequestHostToDevice,
		BmRequestToDevice,
		CY001_LOAD_REQUEST,// Vendor����
		MCU_RESET_REG,	   // ָ����ַ
		0);

	status = WdfUsbTargetDeviceSendControlTransferSynchronously(
		m_hUsbDevice, 
		NULL, NULL,	
		&controlPacket,
		&memDescriptor,
		NULL);

	if(!NT_SUCCESS(status))
		KDBG(DPFLTR_ERROR_LEVEL, "FirmwareReset failed: 0x%X!!!", status);

	return status;
}

// ��һ�ζ����ƵĹ̼�����д�뿪����ָ����ַ����
//
NTSTATUS CY001Drv::FirmwareUpload(PUCHAR pData, ULONG ulLen, WORD offset)
{
	NTSTATUS ntStatus;
	WDF_USB_CONTROL_SETUP_PACKET controlPacket;
	ULONG chunkCount = 0;
	ULONG ulWritten;
	WDF_MEMORY_DESCRIPTOR memDescriptor;
	WDF_OBJECT_ATTRIBUTES attributes;
	int i;

	chunkCount = ((ulLen + CHUNK_SIZE - 1) / CHUNK_SIZE);

	// Ϊ��ȫ��������ع����У�������ݱ��ָ����64�ֽ�Ϊ��λ��С����з��͡�
	// ����Դ����д��ݣ����ܻᷢ�����ݶ�ʧ�������
	//
	for (i = 0; i < chunkCount; i++)
	{
		// �����ڴ�������
		WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&memDescriptor, pData, (i < chunkCount-1)?
			CHUNK_SIZE : 
			(ulLen - (chunkCount-1) * CHUNK_SIZE));// ����������һ���飬��CHUNK_SIZE�ֽڣ�����Ҫ����β�ͳ��ȡ�

		// ��ʼ����������
		WDF_USB_CONTROL_SETUP_PACKET_INIT_VENDOR(
			&controlPacket,
			BmRequestHostToDevice,
			BmRequestToDevice,
			CY001_LOAD_REQUEST,		// Vendor ����A3
			offset + i*CHUNK_SIZE,  // д����ʼ��ַ
			0);

		ntStatus = WdfUsbTargetDeviceSendControlTransferSynchronously(
			m_hUsbDevice, 
			NULL, NULL, 
			&controlPacket,
			&memDescriptor, 
			&ulWritten);

		if (!NT_SUCCESS(ntStatus)){
			KDBG( DPFLTR_ERROR_LEVEL, "FirmwareUpload Failed :0x%0.8x!!!", ntStatus);
			break;
		}else			
			KDBG( DPFLTR_INFO_LEVEL, "%d bytes are written.", ulWritten);

		pData += CHUNK_SIZE;
	}

	return ntStatus;
}

// �ӿ������ڴ�ĵ�ָ����ַ����ȡ��ǰ����
//
NTSTATUS CY001Drv::ReadRAM(WDFREQUEST Request, ULONG* pLen)
{
	NTSTATUS ntStatus;
	WDF_USB_CONTROL_SETUP_PACKET controlPacket;    
	WDFMEMORY   hMem = NULL;
	PFIRMWARE_UPLOAD pUpLoad = NULL;
	WDFREQUEST newRequest;
	void* pData = NULL;
	size_t size;

	KDBG(DPFLTR_INFO_LEVEL, "[ReadRAM]");

	ASSERT(pLen);
	*pLen = 0;

	if(!NT_SUCCESS(WdfRequestRetrieveInputBuffer(Request, sizeof(FIRMWARE_UPLOAD), (void**)&pUpLoad, NULL)) ||
		!NT_SUCCESS(WdfRequestRetrieveOutputBuffer(Request, 1, &pData, &size)))
	{		
		KDBG( DPFLTR_ERROR_LEVEL, "Failed to retrieve memory handle\n");
		return STATUS_INVALID_PARAMETER;
	}

	// �����ڴ�������
	ntStatus = WdfMemoryCreatePreallocated(WDF_NO_OBJECT_ATTRIBUTES, pData, min(size, pUpLoad->len), &hMem);
	if(!NT_SUCCESS(ntStatus))
		return ntStatus;

	// ��ʼ����������
	WDF_USB_CONTROL_SETUP_PACKET_INIT_VENDOR(	
		&controlPacket,
		BmRequestDeviceToHost,// input����
		BmRequestToDevice,
		CY001_LOAD_REQUEST,// Vendor ����A0
		pUpLoad->addr,// ��ַ
		0);

	// ��������ʼ��WDF REQUEST����
	ntStatus = WdfRequestCreate(NULL, NULL, &newRequest);
	if(!NT_SUCCESS(ntStatus))
		return ntStatus;

	WdfUsbTargetDeviceFormatRequestForControlTransfer(m_hUsbDevice,
		newRequest, &controlPacket, hMem, NULL);	

	if(NT_SUCCESS(ntStatus))
	{
		WDF_REQUEST_SEND_OPTIONS opt;
		WDF_REQUEST_SEND_OPTIONS_INIT(&opt, WDF_REQUEST_SEND_OPTION_SYNCHRONOUS);
		if(WdfRequestSend(newRequest, WdfDeviceGetIoTarget(m_hDevice), &opt))
		{
			WDF_REQUEST_COMPLETION_PARAMS par;
			WDF_REQUEST_COMPLETION_PARAMS_INIT(&par);
			WdfRequestGetCompletionParams(newRequest, &par);

			// ȡ�ö�ȡ�����ַ����ȡ�
			*pLen = par.Parameters.Usb.Completion->Parameters.DeviceControlTransfer.Length;
		}
	}

	// ͨ��WdfXxxCreate�����Ķ��󣬱���ɾ��
	WdfObjectDelete(newRequest);

	return ntStatus;
}

// ɾ���½���������ԭʼ���
void ControlRequestComplete(IN WDFREQUEST  Request,
							IN WDFIOTARGET  Target,
							IN PWDF_REQUEST_COMPLETION_PARAMS  Params,
							IN WDFCONTEXT  Context)
{
	ULONG len = 0;
	WDFREQUEST OriginalReqeust = (WDFREQUEST)Context;// ���ԭʼ����
	NTSTATUS status = WdfRequestGetStatus(Request);

	KDBG(DPFLTR_INFO_LEVEL, "[ControlRequestComplete] status: %08X", status);

	if(status == STATUS_IO_TIMEOUT)
		KDBG(DPFLTR_ERROR_LEVEL, "the control request is time out, should be checked.");
	else
		len = Params->Parameters.Usb.Completion->Parameters.DeviceControlTransfer.Length;

	WdfObjectDelete(Request); // ɾ��֮�������½�Ҳ
	WdfRequestCompleteWithInformation(OriginalReqeust, status, len);// ���֮�����������û�����Ҳ
}

// USB���ƶ˿������Щ�������USBԤ��������Vendor�Զ���������������ඨ������ȡ�
// 
NTSTATUS CY001Drv::UsbControlRequest(IN WDFREQUEST Request)
{
	NTSTATUS status;
	WDFREQUEST RequestNew = NULL;
	WDFMEMORY memHandle = NULL;
	WDF_USB_CONTROL_SETUP_PACKET controlPacket;
	WDF_REQUEST_SEND_OPTIONS  opt;

	PUSB_CTL_REQ pRequestControl;
	char* pOutputBuf;
	WDF_USB_BMREQUEST_DIRECTION dir;
	WDF_USB_BMREQUEST_RECIPIENT recipient;

	KDBG(DPFLTR_INFO_LEVEL, "[UsbControlRequest]");
	
	__try
	{
		// �������Ϊһ��USB_CTL_REQ���͵Ľṹ��
		status = WdfRequestRetrieveInputBuffer(Request, sizeof(USB_CTL_REQ)-1, (void**)&pRequestControl, NULL);
		if(!NT_SUCCESS(status))
			__leave;

		// ���������ΪUSB_CTL_REQ�е�buf��Ա��������̳���Ϊ1.
		status = WdfRequestRetrieveOutputBuffer(Request, max(1, pRequestControl->length), (void**)&pOutputBuf, NULL);
		if(!NT_SUCCESS(status))
			__leave;

		// �ж���������������
		if(pRequestControl->type.Request.bDirInput) 
			dir = BmRequestDeviceToHost;
		else
			dir = BmRequestHostToDevice;

		// USB�豸�еĽ��ܷ����������豸�����ӿڡ��˵㣬������֮���δ֪�ߡ�
		switch(pRequestControl->type.Request.recepient){
			case 0: 
				recipient = BmRequestToDevice;
				break;
			case 1:
				recipient = BmRequestToInterface;
				break;
			case 2:
				recipient = BmRequestToEndpoint;
				break;
			case 3:
			default:
				recipient = BmRequestToOther;
		}

		// ��������ص����ݱ�������buf��Աָ���С�
		if(pRequestControl->length)
		{
			status = WdfMemoryCreatePreallocated(NULL, pOutputBuf, pRequestControl->length, &memHandle);
			if(!NT_SUCCESS(status))
				__leave;
		}

		KDBG(DPFLTR_INFO_LEVEL, "%s: recepient:%d type:%d",
			pRequestControl->type.Request.bDirInput?"In Req":"Out Req", 
			pRequestControl->type.Request.recepient, 
			pRequestControl->type.Request.Type);

		// ��ʼ��Setup�ṹ�塣WDF�ṩ��5�г�ʼ���꣬������ֱ���չʾ��
		if(pRequestControl->type.Request.Type == 0x1) // Class ����
		{			
			KDBG(DPFLTR_INFO_LEVEL, "Class Request");
			WDF_USB_CONTROL_SETUP_PACKET_INIT_CLASS (
				&controlPacket,
				dir, recipient,
				pRequestControl->req,
				pRequestControl->value,
				pRequestControl->index
				);
		}
		else if(pRequestControl->type.Request.Type == 0x2) // Vendor ����
		{
			if(pRequestControl->req == 0xA0 || (pRequestControl->req >= 0xD1 && pRequestControl->req <= 0xD5))
			{
				KDBG(DPFLTR_INFO_LEVEL, "Known Vendor Request.");// ��ʶ���Vendor ����
			}
			else
			{
				KDBG(DPFLTR_INFO_LEVEL, "Unknown Vendor Request. DANGER!!!");// ����ʶ��Σ��!!!!
			}

			WDF_USB_CONTROL_SETUP_PACKET_INIT_VENDOR (
				&controlPacket,
				dir,
				recipient,
				pRequestControl->req,
				pRequestControl->value,
				pRequestControl->index
				);
		}
		else if(pRequestControl->type.Request.Type == 0x0) // ��׼����
		{
			KDBG(DPFLTR_INFO_LEVEL, "Standard Request");

			if(pRequestControl->req == 1 || pRequestControl->req == 3) // Feature����
			{
				KDBG(DPFLTR_INFO_LEVEL, "Feature Request");
				WDF_USB_CONTROL_SETUP_PACKET_INIT_FEATURE(
					&controlPacket,
					recipient,
					pRequestControl->value,
					pRequestControl->index,				
					pRequestControl->req == 1 ? FALSE: // clear
												TRUE   // set
				);
			}
			else if(pRequestControl->req == 0)							// Status����
			{			
				KDBG(DPFLTR_INFO_LEVEL, "Status Request");
				WDF_USB_CONTROL_SETUP_PACKET_INIT_GET_STATUS(
					&controlPacket,
					recipient,
					pRequestControl->index
				);
			}
			else														// ����
			{				
				WDF_USB_CONTROL_SETUP_PACKET_INIT (
					&controlPacket,
					dir, recipient,
					pRequestControl->req,
					pRequestControl->value,
					pRequestControl->index
					);
			}
		}

		// ����һ���µ�Request����
		status = WdfRequestCreate(NULL, WdfDeviceGetIoTarget(m_hDevice), &RequestNew);
		if(!NT_SUCCESS(status))
			__leave;

		// ���Usb�豸����controlPacket�ṹ��ʽ���´�����Request����
		status = WdfUsbTargetDeviceFormatRequestForControlTransfer(
			m_hUsbDevice,
			RequestNew, &controlPacket, 
			memHandle, NULL
			);

		if (!NT_SUCCESS(status)){
			KDBG( DPFLTR_ERROR_LEVEL, "WdfUsbTargetDeviceFormatRequestForControlTransfer failed");
			__leave;
		}

		// �첽����
		// ����Timeout��־���Է�ֹ�û��ڷ����˲���ʶ�����������������ڵȴ�������
		// �ȴ�����2�룬���󽫱�ȡ����
		WDF_REQUEST_SEND_OPTIONS_INIT(&opt, 0);
		WDF_REQUEST_SEND_OPTIONS_SET_TIMEOUT(&opt, WDF_REL_TIMEOUT_IN_SEC(2));
		WdfRequestSetCompletionRoutine(RequestNew, ControlRequestComplete, Request);
		if(WdfRequestSend(RequestNew, WdfDeviceGetIoTarget(m_hDevice), &opt) == FALSE) 
		{
			status = WdfRequestGetStatus(RequestNew);
			KDBG(DPFLTR_ERROR_LEVEL, "WdfRequestSend failed");
			__leave;
		}		
	}
	__finally{
	}
	
	if(!NT_SUCCESS(status))
		KDBG(DPFLTR_ERROR_LEVEL, "status: %08X", status);
	
	return status;
}

// ͨ���ܵ�Item�����Եõ��ܵ����������Abort��������ֹPipe�ϵ���������
NTSTATUS CY001Drv::AbortPipe(IN ULONG nPipeNum)
{
	KDBG(DPFLTR_INFO_LEVEL, "[AbortPipe]");

	WDFUSBPIPE pipe = WdfUsbInterfaceGetConfiguredPipe(m_hUsbInterface, nPipeNum, NULL);

	if(pipe == NULL)
		return STATUS_INVALID_PARAMETER;// ����nPipeNum̫����

	NTSTATUS status = WdfUsbTargetPipeAbortSynchronously(pipe, NULL, NULL);
	if(!NT_SUCCESS(status))
		KDBG( DPFLTR_ERROR_LEVEL, "AbortPipe Failed: 0x%0.8X", status);

	return status;
}

NTSTATUS CY001Drv::UsbSetOrClearFeature(WDFREQUEST Request)
{
	NTSTATUS status;
	WDFREQUEST Request_New = NULL;
	WDF_USB_CONTROL_SETUP_PACKET controlPacket;
	PSET_FEATURE_CONTROL pFeaturePacket;

	KDBG(DPFLTR_INFO_LEVEL, "[UsbSetOrClearFeature]");

	status = WdfRequestRetrieveInputBuffer(Request, sizeof(SET_FEATURE_CONTROL), (void**)&pFeaturePacket, NULL);
	if(!NT_SUCCESS(status))return status;

	WDF_USB_CONTROL_SETUP_PACKET_INIT_FEATURE(&controlPacket, 
		BmRequestToDevice,
		pFeaturePacket->FeatureSelector,
		pFeaturePacket->Index,
		pFeaturePacket->bSetOrClear
		);

	status = WdfRequestCreate(NULL, NULL, &Request_New);
	if(!NT_SUCCESS(status)){
		KDBG( DPFLTR_ERROR_LEVEL, "WdfRequestCreate Failed: 0x%0.8X", status);
		return status;
	}

	WdfUsbTargetDeviceFormatRequestForControlTransfer(
		m_hUsbDevice,
		Request_New, 
		&controlPacket, 
		NULL, NULL);

	if(FALSE == WdfRequestSend(Request_New, WdfDeviceGetIoTarget(m_hDevice), NULL))
		status = WdfRequestGetStatus(Request_New);
	WdfObjectDelete(Request_New);

	return status;
}

