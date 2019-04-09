//
//

#include "Spliter.h"

/**************************************************************************

    PAGEABLE CODE

**************************************************************************/

#ifdef ALLOC_PRAGMA
#pragma code_seg("PAGE")
#endif // ALLOC_PRAGMA


CFilter::CFilter (IN PKSFILTER Filter) :
    m_Filter (Filter)
{
}

/*************************************************/

// DispatchCreate��һ���ྲ̬��������ע���AVStream��ܣ�
// �����Ҫ����һ��Filter�����ʱ�򣬽����ô˺�����
NTSTATUS CFilter::DispatchCreate (
    IN PKSFILTER pKsFilter,
    IN PIRP Irp
    )
{
    PAGED_CODE();

    NTSTATUS Status = STATUS_SUCCESS;

    // �ȴ���һ��CFilter�������ϵͳ�ķǷ�ҳ�����������ݡ�
    CFilter *pFilterObj = new (NonPagedPool)
                            CFilter (pKsFilter);

    do{
        if (!pFilterObj) 
        {       
            Status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        // ��AVStream�ܹ��У�����Ҫ��һ����Ҫ�������ö������
        // �����뵽��������Դ���ŵ�������У����Ա�����Դй©��
        Status = KsAddItemToObjectBag (
            pKsFilter -> Bag,
            reinterpret_cast <PVOID> (pFilterObj),
            ClearObj    // �ͷź������˴�����Ϊ����ɡ�
            );

        if (!NT_SUCCESS (Status)) 
        {
            delete pFilterObj;
            break;
        }

        // ����һ���ǳ���Ҫ:��Filter�����ָ����Ϊ��������������
        // ��KSFILTER��ܶ����С����ڿ�ܶ�����Ψһ�Ҳ���ģ�����ÿ
        // ��Filter�ص���ִ�е�ʱ�򣬶����Դӿ�ܶ�����ȡ�������
        pKsFilter -> Context = reinterpret_cast <PVOID> (pFilterObj);        
    }while(0);    

    return Status;
}

/**************************************************************************

    LOCKED CODE

**************************************************************************/

#ifdef ALLOC_PRAGMA
#pragma code_seg()
#endif // ALLOC_PRAGMA

// Filter����Ĵ�������
// ���ǵĴ���������漸��:
// 1. ȡ��������Pinָ��
// 2. ����Pin�ϵ�����
// 3. ��������
NTSTATUS CFilter::Process (
    IN PKSPROCESSPIN_INDEXENTRY ProcessPinsIndex
    )
{
    KdPrint(("Process"));
    NTSTATUS Status = STATUS_SUCCESS;

    BOOLEAN PendingFrames = FALSE;
    ULONG    size;

    // ȡ����/���Pin���󡣼�ס�����������������,
    // Filter�������һ������Pin������Ŀ��Pin��
    PKSPROCESSPIN pPinInput = NULL;
    PKSPROCESSPIN pPinOutput[2] = {NULL, NULL};
    
    // ���е�Pin�������������ProcessPinsIndex����
    // ��Ҫ������������н��������ܹ�ȡ�����а�����Pin����
    ASSERT(ProcessPinsIndex[0].Count == 1);
    ASSERT(ProcessPinsIndex[1].Count > 1);

    pPinInput = ProcessPinsIndex[0].Pins[0];
    pPinOutput[0] = ProcessPinsIndex[1].Pins[0];

	if(ProcessPinsIndex[1].Count > 1)
		pPinOutput[1] = ProcessPinsIndex[1].Pins[1];

    // ����һ������ɹ���������ݳ��ȡ�
    // ����Ӧ��ѡȡ����/���Pin�н�С���Ǹ�����������
    size = min (pPinInput->BytesAvailable, pPinOutput[0]->BytesAvailable);

    //����������
    //1. ����Pin�ϵ����ݿ��������Pin�ϣ��������������Pin�Ӷ�ʵ��1��2��splitter��
    if (!pPinInput->InPlaceCounterpart)
    {
        RtlCopyMemory(pPinOutput[0]->Data, pPinInput->Data, size);
    }

	if(pPinOutput[1])
	{
		RtlCopyMemory(pPinOutput[1]->Data, pPinOutput[0]->Data, size);
	}

    // 2. ����������2��
    for(int i = 0; i < size; i++)
    {
        ((char*)pPinOutput[0]->Data)[i] *= 2;

		if(pPinOutput[1])
		{
			((char*)pPinOutput[1]->Data)[i] *= 2;
		}
    }

    // ����һ������ά��Pin����Ĳ�����
    // ֻ�����������Pin��BytesUsedֵ��ϵͳ�Żᴦ�����Pin�ϵ���Ч������ݡ�
    // ͬ��ֻ�ж�����Pin���в������ã�ϵͳ�Ż��������Render��Ч����Ƶ���ݡ�
    pPinOutput[0]->BytesUsed = pPinInput->BytesUsed = size;

	if(pPinOutput[1])
	{
		pPinOutput[1]->BytesUsed = size;
	}

    return STATUS_SUCCESS;
}

// ����ΪPin������һ�����֣������Ա�ϵͳ��ʾ
GUID gPinName_GUID = 
{0xba1184b9, 0x1fe6, 0x488a, 0xae, 0x78, 0x6e, 0x99, 0x7b, 0x2, 0xca, 0xe0};

