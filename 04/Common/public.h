/*
	��Ȩ��CY001 WDF USB��������Ŀ  2009/9/1
	
	����ӵ�еĴ˷ݴ��뿽�����������ڸ���ѧϰ���κ���ҵ�����������;����ʹ�ã�����������
	�����δ�������벻Ҫ�������������ϴ�������Ŀ�������ѽ����뷢����������ά����

	���ߣ��������		���� 
		  ����������	����
		  AMD			�ĺ���

	���ڣ�2009/9/1

	�ļ���public.h
	˵��������ͷ�ļ�
	��ʷ��
	������
*/

#ifndef __PUBLIC_H__
#define __PUBLIC_H__

#ifdef __cplusplus
extern "C"{
#endif

#pragma warning(disable:4200)  // nameless struct/union
#pragma warning(disable:4201)  // nameless struct/union
#pragma warning(disable:4115)  // named typedef in parenthesis
#pragma warning(disable:4214)  // bit field types other than int

#include <ntddk.h>
#include <usbdi.h>
#include <wdf.h>
#include <Wdfusb.h>
#include <usbdlib.h>
#include <usbbusif.h>
#include <ntstrsafe.h>

#ifndef _WORD_DEFINED
#define _WORD_DEFINED
typedef unsigned short  WORD;
typedef unsigned int	UINT;
#endif 

#include "structure.h"

#define DRIVER_CY001	"CY001: "
#define POOLTAG			'00YC'

#define  MAX_INSTANCE_NUMBER 8
#define  MAX_INTERFACES 8

#define REGISTER_DRV_CLASS(DriverName) \
DrvClass* GetDrvClass(){\
	return  (DrvClass*)new(NonPagedPool, 'GETD') DriverName();\
}

#define REGISTER_DRV_CLASS_NULL()\
	DrvClass* GetDrvClass(){\
	return  new(NonPagedPool, 'GETD') DrvClass();\
}

// WDF�豸����
typedef struct _DEVICE_CONTEXT {
	PVOID pThis;// used by base class

	// below parameters are reserved for subclass.
	PVOID par2;
	PVOID par3;
	PVOID par4;
} DEVICE_CONTEXT, *PDEVICE_CONTEXT;

// WDF��������
typedef struct {
	PVOID pThis;// used by base class

	// below parameters are reserved for subclass.
	PVOID par2;
	PVOID par3;
	PVOID par4;
}DRIVER_CONTEXT, *PDRIVER_CONTEXT;


//===============================��������

// ������ȡ������ĺ���ָ�룬
// ͨ��GetDeviceContext�ܴ���һ��ܶ���ȡ��һ��ָ��ṹ��DEVICE_CONTEXT��ָ�롣
// ʹ��GetDeviceContext��ǰ���ǣ�Ŀ������Ѿ�������һ������ΪDEVICE_CONTEXT�Ļ����顣
// �ڱ������У�������PnpAdd������Ϊ�豸��������DEVICE_CONTEXT�����顣
WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(DEVICE_CONTEXT, GetDeviceContext);

// ������ȡ������ĺ���ָ�룬
// ͨ��GetDeviceContext�ܴ���һ��ܶ���ȡ��һ��ָ��ṹ��DEVICE_CONTEXT��ָ�롣
// ʹ��GetDeviceContext��ǰ���ǣ�Ŀ������Ѿ�������һ������ΪDEVICE_CONTEXT�Ļ����顣
// �ڱ������У�������PnpAdd������Ϊ�豸��������DEVICE_CONTEXT�����顣
WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(DRIVER_CONTEXT, GetDriverContext);

//===============================���ú���
NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT  DriverObject, 
    IN PUNICODE_STRING  RegistryPath
	);

char* 
PowerName(
	WDF_POWER_DEVICE_STATE PowerState
	);

#if (DBG)
__inline KDBG(int nLevel, char* msg, ...)
{
	static char csmsg[1024];

	va_list argp;
	va_start(argp, msg);
	vsprintf(csmsg, msg, argp);
	va_end(argp);

	KdPrintEx((DPFLTR_DEFAULT_ID, nLevel, "CY001:"));// ��ʽ��ͷ
	KdPrintEx((DPFLTR_DEFAULT_ID, nLevel, csmsg));	 // Log body
	KdPrintEx((DPFLTR_DEFAULT_ID, nLevel, "\n"));	 // ����
}
#else
__inline KDBG(int nLevel, char* msg, ...)
{
}
#endif


#ifdef __cplusplus
}
#endif

#endif