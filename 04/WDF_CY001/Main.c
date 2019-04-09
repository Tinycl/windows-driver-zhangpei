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

#include "public.h"

#pragma alloc_text(INIT, DriverEntry)

#define WDF_USB

NTSTATUS DriverEntry(
    IN PDRIVER_OBJECT  DriverObject, 
    IN PUNICODE_STRING  RegistryPath
    )
{
  WDF_DRIVER_CONFIG config;
  NTSTATUS status = STATUS_SUCCESS;

  // �������__DATE__, __TIME__��Checked��������Ч
  // ��ʾ����ʱ���ʱ���
  KDBG(DPFLTR_INFO_LEVEL, "[***CY001.sys DriverEntry ***]");
  KDBG(DPFLTR_INFO_LEVEL, "Compiled time: %s (%s)", __DATE__, __TIME__);

  //
  // ������ʹ����һ�������ж�������䡣��������ͬһ�������У���ʹ��WDF��ʹ��WDM��ʽ��̡�

#ifdef WDF_USB

  // ����WDF������������������������Զ�ò�����Ҳ������¶������ȷʵ�����š�
  // ���ǿ��԰�������һֻ�����ŵ��ְɡ�
  // DriverEntry��������ֳ�ʼ�������������е�С�˿�����һģһ������ֱ���޶��¡�
  // ���߿��Ծݴ˲ο�����Scasi���ļ����ˡ�NDIS��StreamPort�����͵�С�˿�����������������ġ�
  WDF_DRIVER_CONFIG_INIT(&config, PnpAdd);

  status = WdfDriverCreate(DriverObject, // WDF��������
                      RegistryPath,
                      WDF_NO_OBJECT_ATTRIBUTES,
                      &config, // ���ò���
                      WDF_NO_HANDLE);

  //gDriverObj = DriverObject; // �����Ҫ�����԰��������󱣴���ȫ�ֱ�����
  
#else
	//�ڴ˴�����WDM�����ַ��ص�����
#endif

  return status;
}
