/*
	��Ȩ��CY001 WDF USB��������Ŀ  2009/9/1
	
	����ӵ�еĴ˷ݴ��뿽�����������ڸ���ѧϰ���κ���ҵ�����������;����ʹ�ã�����������
	�����δ�������벻Ҫ�������������ϴ�������Ŀ�������ѽ����뷢����������ά����

	���ߣ��������		���� 
		  ����������	����
		  AMD			�ĺ���

	���ڣ�2009/9/1

	�ļ���ReadWrite.c
	˵����USB�豸��д
	��ʷ��
	������
*/

#include "Cy001Drv.h"

VOID CY001Drv::InterruptRead_sta(
				  WDFUSBPIPE  Pipe,
				  WDFMEMORY  Buffer,
				  size_t  NumBytesTransferred,
				  WDFCONTEXT  pContext
				  )
{
	CY001Drv *pThis = (CY001Drv*)pContext;
	pThis->InterruptRead(Buffer, NumBytesTransferred);
}

VOID CY001Drv::BulkRead_sta(
			 IN WDFQUEUE  Queue,
			 IN WDFREQUEST  Request,
			 IN size_t  Length
			 )
{
	CY001Drv* pThis = (CY001Drv*)CY001Drv::GetDrvClassFromDevice(WdfIoQueueGetDevice(Queue));
	pThis->BulkRead(Queue, Request, Length);
}

VOID CY001Drv::BulkWrite_sta(
			  IN WDFQUEUE  Queue,
			  IN WDFREQUEST  Request,
			  IN size_t  Length
			  )
{
	CY001Drv* pThis = (CY001Drv*)CY001Drv::GetDrvClassFromDevice(WdfIoQueueGetDevice(Queue));
	pThis->BulkWrite(Queue, Request, Length);
}

VOID CY001Drv::BulkReadComplete_sta(
					 IN WDFREQUEST  Request,
					 IN WDFIOTARGET  Target,
					 IN PWDF_REQUEST_COMPLETION_PARAMS  Params,
					 IN WDFCONTEXT  pContext
					 )
{
	CY001Drv *pThis = (CY001Drv*)pContext;
	pThis->BulkReadComplete(Request, Target, Params);
}

VOID CY001Drv::BulkWriteComplete_sta(
					  IN WDFREQUEST  Request,
					  IN WDFIOTARGET  Target,
					  IN PWDF_REQUEST_COMPLETION_PARAMS  Params,
					  IN WDFCONTEXT  pContext
					  )
{
	CY001Drv *pThis = (CY001Drv*)pContext;
	pThis->BulkWriteComplete(Request, Target, Params);
}

// �ж�Pipe�ص�����������һ���豸�������ж���Ϣ���������ܹ���ȡ����
//
VOID CY001Drv::InterruptRead(WDFMEMORY Buffer, size_t NumBytesTransferred)
{
	NTSTATUS status;
	size_t size = 0;
	WDFREQUEST Request = NULL;
	CHAR *pchBuf = NULL;

	KDBG(DPFLTR_INFO_LEVEL, "[InterruptRead]");

	// Read���ݻ�������ע�⵽���������������ǹܵ��������ȵı�����
	// ��������ֻ�û������ĵ�һ����Ч�ֽڡ�
	pchBuf = (CHAR*)WdfMemoryGetBuffer(Buffer, &size);
	if(pchBuf == NULL || size == 0)
		return;

	// ��һ���ֽ�Ϊȷ���ֽڣ�һ����0xD4
	//if(pchBuf[0] != 0xD4)return;

	// �Ӷ�������ȡһ��δ�������
	status = WdfIoQueueRetrieveNextRequest(m_hInterruptManualQueue, &Request);

	if(NT_SUCCESS(status))
	{
		CHAR* pOutputBuffer = NULL;
		status = WdfRequestRetrieveOutputBuffer(Request, 1, (PVOID*)&pOutputBuffer, NULL);

		if(NT_SUCCESS(status))
		{
			// �ѽ�����ظ�Ӧ�ó���
			pOutputBuffer[0] = pchBuf[1];
			WdfRequestCompleteWithInformation(Request, status, 1);
		}
		else
		{
			// ���ش���
			WdfRequestComplete(Request, status);
		}

		KDBG(DPFLTR_INFO_LEVEL, "Get and finish an interrupt read request.");
	}else{
		// ���пգ����������豸��ȡ�����ݡ�
		KDBG(DPFLTR_INFO_LEVEL, "Manual interrupt queue is empty!!!");
	}
}

// Bulk�ܵ�д����
//
VOID CY001Drv::BulkWrite(IN WDFQUEUE  Queue, IN WDFREQUEST  Request, IN size_t  Length)
{
	NTSTATUS status = STATUS_SUCCESS;
	WDFMEMORY hMem = NULL;
	WDFDEVICE hDevice = NULL;
	UCHAR* lpBuf;

	UNREFERENCED_PARAMETER(Length);

	KDBG(DPFLTR_INFO_LEVEL, "[BulkWrite] size: %d", Length);

	// ��ȡ�ܵ����
	hDevice = WdfIoQueueGetDevice(Queue);

	if(NULL == m_hUsbBulkOutPipe)
	{
		KDBG(DPFLTR_ERROR_LEVEL, "BulkOutputPipe = NULL");
		WdfRequestComplete(Request, STATUS_UNSUCCESSFUL);
		return;
	}

	status = WdfRequestRetrieveInputMemory(Request, &hMem);
	if(!NT_SUCCESS(status))
	{
		KDBG(DPFLTR_ERROR_LEVEL, "WdfRequestRetrieveInputMemory failed(status = 0x%0.8x)!!!", status);
		WdfRequestComplete(Request, status);
		return;
	}

	// ��ӡ��offsetֵ��
	// ��д�����ǰ�����ֽ��д���write offset��ֵ
	lpBuf = (UCHAR*)WdfMemoryGetBuffer(hMem, 0);
	KDBG(DPFLTR_TRACE_LEVEL, "write offset: %hd", *(WORD*)lpBuf);

	// �ѵ�ǰ��Request������������ã����͸�USB���ߡ�
	// ��ʽ��Request��������Pipe�������ɺ����ȡ�
	status = WdfUsbTargetPipeFormatRequestForWrite(m_hUsbBulkOutPipe, Request, hMem, NULL);
	if(!NT_SUCCESS(status))
	{
		KDBG(DPFLTR_ERROR_LEVEL, "WdfUsbTargetPipeFormatRequestForWrite(status 0x%0.8x)!!!", status);
		WdfRequestComplete(Request, status);
		return;
	}

	WdfRequestSetCompletionRoutine(Request, BulkWriteComplete_sta, this); 
	if(FALSE == WdfRequestSend(Request, WdfUsbTargetPipeGetIoTarget(m_hUsbBulkOutPipe), NULL))
	{
		status = WdfRequestGetStatus(Request);
		KDBG(DPFLTR_ERROR_LEVEL, "WdfRequestSend failed with status 0x%0.8x\n", status);		
		WdfRequestComplete(Request, status);
	}
}

// Bulk�ܵ�д��������ɺ���
//
VOID CY001Drv::BulkWriteComplete(IN WDFREQUEST  Request, IN WDFIOTARGET  Target, 
						IN PWDF_REQUEST_COMPLETION_PARAMS  Params)
{
	PWDF_USB_REQUEST_COMPLETION_PARAMS usbCompletionParams;
	NTSTATUS ntStatus;
	ULONG_PTR ulLen;
	LONG* lpBuf;

	KDBG(DPFLTR_INFO_LEVEL, "[BulkWriteComplete]");

	usbCompletionParams = Params->Parameters.Usb.Completion;
	ntStatus = Params->IoStatus.Status;
	ulLen = usbCompletionParams->Parameters.PipeWrite.Length;
	lpBuf = (LONG*)WdfMemoryGetBuffer(usbCompletionParams->Parameters.PipeWrite.Buffer, NULL);

	if(NT_SUCCESS(ntStatus))
		KDBG(DPFLTR_INFO_LEVEL, "%d bytes written to USB device successfully.", ulLen);
	else
		KDBG(DPFLTR_INFO_LEVEL, "Failed to write: 0x%08x!!!", ntStatus);

	// �������
	// Ӧ�ó����յ�֪ͨ��
	WdfRequestCompleteWithInformation(Request, ntStatus, ulLen);
}

// Bulk�ܵ�������
//
VOID CY001Drv::BulkRead(IN WDFQUEUE  Queue, IN WDFREQUEST  Request, IN size_t  Length)
{
	NTSTATUS status = STATUS_SUCCESS;
	WDFMEMORY hMem;

	KDBG(DPFLTR_INFO_LEVEL, "[BulkRead] size: %d", Length);

	if(NULL == m_hUsbBulkInPipe){
		WdfRequestComplete(Request, STATUS_UNSUCCESSFUL);
		return;
	}

	status = WdfRequestRetrieveOutputMemory(Request, &hMem);
	if(!NT_SUCCESS(status))
	{
		KDBG(DPFLTR_INFO_LEVEL, "WdfRequestRetrieveOutputMemory failed with status 0x%0.8x!!!", status);
		WdfRequestComplete(Request, status);
		return;
	}

	// ��дһ������Ȼ�Ƕ�Request�����������
	// �����������⣬Ҳ�����½�һ��Request�����½��ķ����ڱ����������ط��õý϶ࡣ
	status = WdfUsbTargetPipeFormatRequestForRead(m_hUsbBulkInPipe, Request, hMem, NULL);
	if(!NT_SUCCESS(status))
	{
		KDBG(DPFLTR_INFO_LEVEL, "WdfUsbTargetPipeFormatRequestForRead failed with status 0x%08x\n", status);
		WdfRequestComplete(Request, status);
		return;
	}

	WdfRequestSetCompletionRoutine(Request, BulkReadComplete_sta, this);
	if(FALSE == WdfRequestSend(Request, WdfUsbTargetPipeGetIoTarget(m_hUsbBulkInPipe), NULL))
	{
		status = WdfRequestGetStatus(Request);
		KDBG(DPFLTR_INFO_LEVEL, "WdfRequestSend failed with status 0x%08x\n", status);
		WdfRequestComplete(Request, status);
	}
}

// Bulk�ܵ�����������ɺ���
//
VOID CY001Drv::BulkReadComplete(IN WDFREQUEST  Request, IN WDFIOTARGET  Target,
					   IN PWDF_REQUEST_COMPLETION_PARAMS  Params)
{
	PWDF_USB_REQUEST_COMPLETION_PARAMS usbCompletionParams;
	NTSTATUS ntStatus;
	ULONG_PTR ulLen;
	LONG* lpBuf;

	KDBG(DPFLTR_INFO_LEVEL, "[BulkReadComplete]");

	usbCompletionParams = Params->Parameters.Usb.Completion;
	ntStatus = Params->IoStatus.Status;
	ulLen = usbCompletionParams->Parameters.PipeRead.Length;
	lpBuf = (LONG*)WdfMemoryGetBuffer(usbCompletionParams->Parameters.PipeRead.Buffer, NULL);

	if(NT_SUCCESS(ntStatus))
		KDBG(DPFLTR_INFO_LEVEL, "%d bytes readen from USB device successfully.", ulLen);
	else
		KDBG(DPFLTR_INFO_LEVEL, "Failed to read: 0x%08x!!!", ntStatus);

	// ��ɲ���
	// Ӧ�ó����յ�֪ͨ��
	WdfRequestCompleteWithInformation(Request, ntStatus, ulLen);
}
