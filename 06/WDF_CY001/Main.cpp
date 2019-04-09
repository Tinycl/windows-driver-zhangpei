/*
	��Ȩ��CY001 WDF USB��������Ŀ  2009/9/1
	
	����ӵ�еĴ˷ݴ��뿽�����������ڸ���ѧϰ���κ���ҵ�����������;����ʹ�ã�����������
	�����δ�������벻Ҫ�������������ϴ�������Ŀ�������ѽ����뷢����������ά����

	���ߣ��������		���� 
		  ����������	����
		  AMD			�ĺ���

	���ڣ�2009/9/1

	�ļ���Main.c
	˵������ں���
	��ʷ��
	������
*/
#ifdef __cplusplus
extern "C"{
#endif

#include "public.h"
#pragma alloc_text(INIT, DriverEntry)

#ifdef __cplusplus
}
#endif

#include "NewDelete.h"
#include "DrvClass.h"


VOID DrvClassDestroy(IN WDFOBJECT  Object)
{
	PVOID pBuf = WdfMemoryGetBuffer((WDFMEMORY)Object, NULL);
	delete pBuf;
}

NTSTATUS DrvClass::DriverEntry(IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING RegistryPath)
{
	KDBG(DPFLTR_INFO_LEVEL, "[DrvClass::DriverEntry] this = %p", this);

	WDF_OBJECT_ATTRIBUTES attributes;
	WDF_DRIVER_CONFIG config;

	// �趨�豸�����鳤��
	// ���ڲ������sizeof(DEVICE_CONTEXT)��ṹ�峤��
	WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, DRIVER_CONTEXT);

	// ����WDF������������������������Զ�ò�����Ҳ������¶������ȷʵ�����š�
	// ���ǿ��԰�������һֻ�����ŵ��ְɡ�
	// DriverEntry��������ֳ�ʼ�������������е�С�˿�����һģһ������ֱ���޶��¡�
	// ���߿��Ծݴ˲ο�����Scasi���ļ����ˡ�NDIS��StreamPort�����͵�С�˿�����������������ġ�
	WDF_DRIVER_CONFIG_INIT(&config, DrvClass::PnpAdd_sta);

	NTSTATUS status = WdfDriverCreate(DriverObject, // WDF��������
		RegistryPath,
		&attributes,
		&config, // ���ò���
		&m_hDriver);

	if(NT_SUCCESS(status))
	{
		m_pDriverContext = GetDriverContext(m_hDriver);
		ASSERT(m_pDriverContext);
		m_pDriverContext->par1 = (PVOID)this;	

		WDFMEMORY hMemDrv;
		WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
		attributes.ParentObject = m_hDriver;
		attributes.EvtDestroyCallback  = DrvClassDestroy;
		WdfMemoryCreatePreallocated(&attributes, (PVOID)this, GetSize(), &hMemDrv);
	}

	return status;
}

DrvClass* GetDrvClass();

extern "C" NTSTATUS DriverEntry(
    IN PDRIVER_OBJECT  DriverObject, 
    IN PUNICODE_STRING  RegistryPath
    )
{
	DrvClass* myDriver = GetDrvClass();//new(NonPagedPool, 'CY01')DrvClass();
	if(myDriver == NULL)return STATUS_UNSUCCESSFUL;
	return myDriver->DriverEntry(DriverObject, RegistryPath);
}
