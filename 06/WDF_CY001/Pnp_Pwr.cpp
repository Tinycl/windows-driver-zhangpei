/*
	��Ȩ��CY001 WDF USB��������Ŀ  2009/9/1
	
	����ӵ�еĴ˷ݴ��뿽�����������ڸ���ѧϰ���κ���ҵ�����������;����ʹ�ã�����������
	�����δ�������벻Ҫ�������������ϴ�������Ŀ�������ѽ����뷢����������ά����

	���ߣ��������		���� 
		  ����������	����
		  AMD			�ĺ���

	���ڣ�2009/9/1

	�ļ���Pnp_Pwr.c
	˵����PNP���Դ����
	��ʷ��
	������
*/
#ifdef __cplusplus
extern "C"{
#endif

#include "public.h"

#ifdef __cplusplus
}
#endif

#include "DrvClass.h"
#pragma code_seg("PAGE")

//��������Power�ص�����WDM�е�PnpSetPower���ơ�
NTSTATUS DrvClass::PwrD0Entry(IN WDF_POWER_DEVICE_STATE  PreviousState)
{

}

// �뿪D0״̬
NTSTATUS DrvClass::PwrD0Exit(IN WDF_POWER_DEVICE_STATE  TargetState)
{
}

VOID DrvClass::PnpSurpriseRemove(IN WDFDEVICE  Device)
{
	CompleteSyncRequest(Device, SURPRISE_REMOVE, 0);
}

