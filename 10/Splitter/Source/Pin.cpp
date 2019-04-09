
#include "Spliter.h"

/**************************************************************************

    PAGED CODE

**************************************************************************/

#ifdef ALLOC_PRAGMA
#pragma code_seg("PAGE")
#endif // ALLOC_PRAGMA

// DispatchCreate��һ���ྲ̬��������ע���AVStream��ܣ������Ҫ
// Ϊ����Filter�������е�����Ϊ������Pin�����ʱ�򣬽����ô˺�����
NTSTATUS CPin::DispatchCreate (
    IN PKSPIN pKsPin,
    IN PIRP Irp
    )
{
    PAGED_CODE();

    NTSTATUS Status = STATUS_SUCCESS;

    CPin *pPinObj = new (NonPagedPool) CPin (pKsPin);

    if (!pPinObj) 
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Status = KsAddItemToObjectBag (
        pKsPin -> Bag,
        reinterpret_cast <PVOID> (pPinObj),
        reinterpret_cast <PFNKSFREE> (ClearObj));

    if (!NT_SUCCESS (Status))
    {
        delete pPinObj;
    }

    // ����һ���ǳ���Ҫ:��Pin�����ָ����Ϊ��������������
    // ��KSPIN��ܶ����С����ڿ�ܶ�����Ψһ�Ҳ���ģ�����ÿ
    // ��Pin�ص���ִ�е�ʱ�򣬶����Դӿ�ܶ�����ȡ�������
    pKsPin -> Context = reinterpret_cast <PVOID> (pPinObj);

    return Status;
}

// ��Pin����״̬Ǩ�Ƶ�ʱ�򣬻���ô˻ص�������
// ����������������治�����κζ��������
NTSTATUS CPin::SetState (
                         IN KSSTATE ToState,
                         IN KSSTATE FromState
                         )
{
    PAGED_CODE();

    switch (ToState) 
    {
    case KSSTATE_RUN:
        KdPrint(("KSSTATE_RUN"));
        break;
    case KSSTATE_STOP:  
        KdPrint(("KSSTATE_STOP"));
        break;
    case KSSTATE_ACQUIRE:
        KdPrint(("KSSTATE_ACQUIRE"));
        break;
    case KSSTATE_PAUSE:
        KdPrint(("KSSTATE_PAUSE"));
        break;
    }

    m_State = ToState;
    return STATUS_SUCCESS;
}

NTSTATUS CPin::IntersectHandler (
    IN PKSFILTER pKsFilter,
    IN PIRP Irp,
    IN PKSP_PIN PinInstance,
    IN PKSDATARANGE CallerDataRange,
    IN PKSDATARANGE DescriptorDataRange,
    IN ULONG BufferSize,
    OUT PVOID Data OPTIONAL,
    OUT PULONG DataSize
    )
{    
    PAGED_CODE();
    PKSPIN pKsPin;

    PKSDATARANGE_AUDIO CallerAudioRange =
        reinterpret_cast <PKSDATARANGE_AUDIO> (CallerDataRange);

    PKSDATARANGE_AUDIO DescriptorAudioRange =
        reinterpret_cast <PKSDATARANGE_AUDIO> (DescriptorDataRange);

    // ��Ϊ����/���Pin�����ݸ�ʽ����ͳһ��
    // ��������Ҫ����һ�����飬�������ж϶�Ӧ�����/����Pin�����Ƿ����
    // ����Ѿ����ڣ���Pin�����ݸ�ʽһ���Ѿ����ú��ˣ�ֻҪ������������
    // ��������ڣ������ֶ���ͷ����
    if (PinInstance->PinId == 0)
    {
        pKsPin = KsFilterGetFirstChildPin (pKsFilter, 1);
    }
    else
    {
        pKsPin = KsFilterGetFirstChildPin (pKsFilter, 0);
    }

    if(pKsPin)
    {
        // ר�����Ի�ȡ���ȣ����涨Ӧ����STATUS_BUFFER_OVERFLOW
        if(BufferSize == 0)
        {
            *DataSize = pKsPin->ConnectionFormat->FormatSize;
            return STATUS_BUFFER_OVERFLOW;
        }

        // �������������̫�̣�����STATUS_BUFFER_TOO_SMALL
        if(BufferSize < pKsPin->ConnectionFormat->FormatSize)
        {
            return STATUS_BUFFER_TOO_SMALL;
        }

        // �ڿ���֮ǰ���ȶ�pKsPin�е����ݸ�ʽ���򵥵��ж�
        if(IsEqualGUIDAligned (CallerDataRange->Specifier, 
            KSDATAFORMAT_SPECIFIER_WAVEFORMATEX))
        {
            if(CallerDataRange->FormatSize < sizeof(KSDATARANGE_AUDIO))
            {
                return STATUS_INVALID_PARAMETER;
            }

            PWAVEFORMATEX pWaveFormatEx = 
                (PWAVEFORMATEX)(pKsPin->ConnectionFormat + 1);

            if (DescriptorAudioRange -> MaximumChannels < pWaveFormatEx -> nChannels ||
                DescriptorAudioRange -> MinimumBitsPerSample > pWaveFormatEx -> wBitsPerSample ||
                DescriptorAudioRange -> MaximumBitsPerSample < pWaveFormatEx -> wBitsPerSample ||
                DescriptorAudioRange -> MaximumSampleFrequency < pWaveFormatEx -> nSamplesPerSec ||
                DescriptorAudioRange -> MinimumSampleFrequency > pWaveFormatEx -> nSamplesPerSec) 
            {
                *DataSize = sizeof (KSDATAFORMAT_WAVEFORMATEX);
                return STATUS_NO_MATCH;
            }
        }
        else
        {
            return STATUS_NO_MATCH;
        }

        // �������ݸ�ʽ�ṹ��
        *DataSize = pKsPin->ConnectionFormat->FormatSize;

        RtlCopyMemory (Data,
            pKsPin->ConnectionFormat, 
            *DataSize);

        return STATUS_SUCCESS;
    }

    // �����ֶ����ø�ʽ�ṹ��
    // 

    if(0 == BufferSize)
    {
        *DataSize = sizeof(KSDATAFORMAT_WAVEFORMATEX);
        return STATUS_BUFFER_OVERFLOW;
    }

    if (BufferSize < sizeof(KSDATAFORMAT_WAVEFORMATEX)) 
    {
        return STATUS_NO_MATCH;
    }

    if(CallerDataRange->FormatSize < sizeof(KSDATARANGE_AUDIO))
    {
        return STATUS_INVALID_PARAMETER;
    }

    if (DescriptorAudioRange -> MinimumBitsPerSample > CallerAudioRange -> MaximumBitsPerSample ||
        DescriptorAudioRange -> MaximumBitsPerSample < CallerAudioRange -> MinimumBitsPerSample ||
        DescriptorAudioRange -> MaximumSampleFrequency < CallerAudioRange -> MinimumSampleFrequency ||
        DescriptorAudioRange -> MinimumSampleFrequency > CallerAudioRange -> MaximumSampleFrequency) 
    {
        return STATUS_NO_MATCH;
    }

    if(IsEqualGUIDAligned (CallerDataRange->Specifier, 
        KSDATAFORMAT_SPECIFIER_WAVEFORMATEX))
    {
        *DataSize = sizeof (KSDATAFORMAT_WAVEFORMATEX);

        PWAVEFORMATEX WaveFormat = (PWAVEFORMATEX)((PKSDATAFORMAT)Data + 1);
        PKSDATARANGE_AUDIO DescriptorAudioRange = (PKSDATARANGE_AUDIO)DescriptorDataRange;

        // �����ֶ����ø�ʽ�ĸ������棡
        *(PKSDATAFORMAT)Data = *DescriptorDataRange;

        WaveFormat -> wFormatTag = WAVE_FORMAT_PCM;
        WaveFormat -> nChannels = (WORD)DescriptorAudioRange -> MaximumChannels;
        WaveFormat -> nSamplesPerSec = DescriptorAudioRange -> MinimumSampleFrequency;
        WaveFormat -> wBitsPerSample = (WORD)DescriptorAudioRange -> MinimumBitsPerSample;
        WaveFormat -> nBlockAlign = (WaveFormat -> wBitsPerSample / 8) * WaveFormat -> nChannels;
        WaveFormat -> nAvgBytesPerSec = (WaveFormat -> nBlockAlign) * (WaveFormat -> nSamplesPerSec);
        WaveFormat -> cbSize = 0;

        ((PKSDATAFORMAT)Data)->SampleSize = WaveFormat -> nBlockAlign;
        ((PKSDATAFORMAT)Data)->FormatSize = sizeof (KSDATAFORMAT_WAVEFORMATEX);

        return STATUS_SUCCESS;
    }

    return STATUS_NO_MATCH;
}

NTSTATUS CPin::DispatchSetFormat (
    IN PKSPIN pKsPin,
    IN PKSDATAFORMAT OldFormat OPTIONAL,
    IN PKSMULTIPLE_ITEM OldAttributeList OPTIONAL,
    IN const KSDATARANGE *DataRange,
    IN const KSATTRIBUTE_LIST *AttributeRange OPTIONAL
    )
{
    PAGED_CODE();

    const KSDATARANGE_AUDIO *DataRangeAudio =
        reinterpret_cast <const KSDATARANGE_AUDIO *> (DataRange);

    // ��ʽӦ����KSDATAFORMAT_WAVEFORMATEX���͵�
    if (pKsPin -> ConnectionFormat -> FormatSize <
        sizeof (KSDATAFORMAT_WAVEFORMATEX)) 
    {
        return STATUS_NO_MATCH;
    }

    // ���ж�һ�£���ʽ�Ƿ���ȷ����ʽ��ΧӦ�ɿ�ܵ��ûص�
    // ����IntersectHandler���أ�����Ϊ�������DataRange����
    PWAVEFORMATEX WaveFormat = (PWAVEFORMATEX)(pKsPin -> ConnectionFormat + 1);

    if (WaveFormat -> wFormatTag != WAVE_FORMAT_PCM ||
        WaveFormat -> nChannels != DataRangeAudio -> MaximumChannels ||
        WaveFormat -> nSamplesPerSec > DataRangeAudio -> MaximumSampleFrequency || 
		WaveFormat -> nSamplesPerSec < DataRangeAudio -> MinimumSampleFrequency ||
        WaveFormat -> wBitsPerSample > DataRangeAudio -> MaximumBitsPerSample ||
		WaveFormat -> wBitsPerSample < DataRangeAudio -> MinimumBitsPerSample) 
    {
        return STATUS_NO_MATCH;
    }

    // ����Ҫ�����������ж����롢���Pin�ĸ�ʽ�Ƿ���ͬ
    PKSPIN pKsOtherPin = 0;
    PKSFILTER pKsFilter = KsPinGetParentFilter (pKsPin);

    if(pKsPin->Id == 0)
    {
        pKsOtherPin = KsFilterGetFirstChildPin (pKsFilter, 1);
    }
    else
    {
        pKsOtherPin = KsFilterGetFirstChildPin (pKsFilter, 0);
    }

    if(!pKsOtherPin)
    {
        return STATUS_SUCCESS;
    }

    PWAVEFORMATEX   thisWaveFmt = (PWAVEFORMATEX)(pKsPin->ConnectionFormat + 1);
    PWAVEFORMATEX  otherWaveFmt = (PWAVEFORMATEX)(pKsOtherPin->ConnectionFormat + 1);

    if ((thisWaveFmt->nChannels == otherWaveFmt->nChannels) &&
        (thisWaveFmt->nSamplesPerSec == otherWaveFmt->nSamplesPerSec) &&
        (thisWaveFmt->wBitsPerSample == otherWaveFmt->wBitsPerSample))
    {
        // ���߸�ʽ��ͬ��������ȷ
        return STATUS_SUCCESS;
    }

    // �����ʽ����ȫ��ͬ��Ҫ���Խ����߽���ͳһ��
    // �ѵ�ǰ���õĸ�ʽȥ��������һ��Pin������ɹ�����ɡ�
    // Ӧע��������һ�����������ʵ�ֵġ�
    KSPROPERTY      property;
    PIKSCONTROL     pIKsControl;
    ULONG           cbReturned;

    property.Set = KSPROPSETID_Connection;
    property.Id = KSPROPERTY_CONNECTION_DATAFORMAT;
    property.Flags = KSPROPERTY_TYPE_SET;
    
    // ��ȡ���ӵ�pKsOtherPin��Pin����Ŀ��ƽӿ�
    // �����ͨ��������ƽӿ�������������Format����
    NTSTATUS status = KsPinGetConnectedPinInterface (
        pKsOtherPin,
        &IID_IKsControl, 
        (PVOID*)&pIKsControl);

    if (!NT_SUCCESS(status))
    {
        return status;
    }

    // �ͷŵ�ǰ�߳�ռ�еĿ��Mutex
    // ����������������ܻᵼ������
    KsFilterReleaseControl (pKsFilter);

    // ��������
    status = pIKsControl->KsProperty (
        &property, 
        sizeof(property),
        pKsPin->ConnectionFormat, 
        pKsPin->ConnectionFormat->FormatSize,
        &cbReturned);

    // ���»�ȡMutex��
    // ��������������ͷŵ�ʱ��Ų������
    KsFilterAcquireControl (pKsFilter);

    // �ͷſ��ƽӿ�
    pIKsControl->Release();
    return status;
}

NTSTATUS CPin::AudioPosition(
                 IN     PIRP irp,
                 IN     PKSPROPERTY Request,
                 IN OUT PVOID Data)
{
	PAGED_CODE ();

	PKSFILTER       pKsFilter;
	PKSPIN          pKsPinOther;
	PKSPIN          pKsPin;
	ULONG           bytesReturned;
	PIKSCONTROL     pIKsControl;
	NTSTATUS        ntStatus;

	KdPrint(("[PropertyAudioPosition]"));

	// Pin����ָ�뱣����IRP��
	pKsPin = KsGetPinFromIrp (irp);
	if (!pKsPin)
	{
		KdPrint(("[PropertyAudioPosition] this property is for a filter node?"));
		return STATUS_UNSUCCESSFUL;
	}

	// ������Ϊ��������ֻӦ�÷��͸�Pin 0
	if (pKsPin->Id != 0)
	{
		KdPrint (("[PropertyAudioPosition] this property was invoked on the source pin!"));
		return STATUS_INVALID_DEVICE_REQUEST;
	}

	// ȡ����һ��Pin 1����
	pKsFilter = KsPinGetParentFilter (pKsPin);
	KsFilterAcquireControl (pKsFilter);
	pKsPinOther = KsFilterGetFirstChildPin (pKsFilter, 1);
	if (!pKsPinOther)
	{
		KsFilterReleaseControl (pKsFilter);
		KdPrint (("[PropertyAudioPosition] couldn't find the source pin."));
		return STATUS_UNSUCCESSFUL;
	}

	//ȡ�����ӿ����Pin����һ��Filter������Pin
	ntStatus = KsPinGetConnectedPinInterface (
		pKsPinOther, 
		&IID_IKsControl,
		(PVOID*)&pIKsControl);

	if (!NT_SUCCESS (ntStatus))
	{
		KsFilterReleaseControl (pKsFilter);
		KdPrint (("[PropertyAudioPosition] couldn't get IID_IKsControl interface."));
		return STATUS_UNSUCCESSFUL;
	}

	// �ڵ�������Pin����ǰ��Ӧ�ͷŵ�ǰFilter�Ļ������
	// ���������������
	KsFilterReleaseControl (pKsFilter);

	// �ѵ�ǰ����ԭԭ�����ش�����ȥ
	ntStatus = pIKsControl->KsProperty (
		Request, 
		sizeof (KSPROPERTY),
		Data, 
		sizeof (KSAUDIO_POSITION),
		&bytesReturned);

	pIKsControl->Release();
	irp->IoStatus.Information = bytesReturned;

	return(ntStatus);
}

#ifdef ALLOC_PRAGMA
#pragma code_seg()
#endif // ALLOC_PRAGMA
