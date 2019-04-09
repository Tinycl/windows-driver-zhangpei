/// 
///
#include "Spliter.h"

void ClearObj (IN void* obj)
{
    delete obj;
}

/**************************************************************************

    INITIALIZATION CODE

**************************************************************************/


extern "C"
NTSTATUS
DriverEntry (
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )
{

    // ��ں�������ֻҪ�������������飬
    // ����ز�Ҫ���ǵ���KsInitializeDriver��
    // KsInitializeDriver��AVStream��ܱ�¶�Ľӿں�����
    // ������ʼ������йصĽṹ�塣��Ϊ����Ҫ�Ĳ�����
    // ������Ӧ����ϸ��׼ȷ�������豸��������
    return KsInitializeDriver (
            DriverObject,
            RegistryPath,
            &gKsSplitterDevice
            );

}

/**************************************************************************

    �������������б������豸��Filter�����������������ȶ�������

**************************************************************************/

//=====================������Pin��ص�������

// ����/���Pin��֧�ֵ����ݸ�ʽ������ʵ��һ����ʽ��Χ��Range��
// �������Χ���棬�������������С���ã������С�������
//
// ����Ľṹ������ʾ�ĸ�ʽ��Χ�ǣ�
// ˫������24λ������44.1����48K������
KSDATARANGE_AUDIO gPinDataFormatRange =
{
    {
        sizeof(KSDATARANGE_AUDIO),
        0, 6, 0,
        STATIC_KSDATAFORMAT_TYPE_AUDIO,
        STATICGUIDOF(KSDATAFORMAT_SUBTYPE_PCM),
        STATICGUIDOF(KSDATAFORMAT_SPECIFIER_WAVEFORMATEX) 
    },
    2,          // channels
    16,         // min. bits per sample
    24,         // max. bits per sample
    44100,      // min. sample rate
    48000       // max. sample rate
};

const PKSDATARANGE gPinDataFormatRanges[] =
{
    PKSDATARANGE(&gPinDataFormatRange)
};

const KSPIN_DISPATCH gPinDispatch = 
{
    CPin::DispatchCreate,                    // Pin Create
    NULL,                                   // Pin Close
    NULL,                                   // Pin Process
    NULL,                                   // Pin Reset
    CPin::DispatchSetFormat,                // Pin Set Data Format
    CPin::DispatchSetState,                    // Pin Set Device State
    NULL,                                   // Pin Connect
    NULL,                                   // Pin Disconnect
    NULL,                                   // Clock Dispatch
    NULL                                    // Allocator Dispatch
};

// ����֡��ʽ��Allocator Framing��
DECLARE_SIMPLE_FRAMING_EX (
    gAllocatorFraming,
    STATICGUIDOF (KSMEMORY_TYPE_KERNEL_NONPAGED),
    KSALLOCATOR_REQUIREMENTF_SYSTEM_MEMORY |
    KSALLOCATOR_REQUIREMENTF_PREFERENCES_ONLY,
    25,
    0,
    2 * PAGE_SIZE,
    2 * PAGE_SIZE
);

// ���������б�
// �±��а������������ԣ��ֱ�������ȡ��Ƶλ���Լ���������任
DEFINE_KSPROPERTY_TABLE (gPinProperty)
{
    DEFINE_KSPROPERTY_ITEM
    (
        KSPROPERTY_AUDIO_POSITION,      // property item defined in ksmedia.h
        CPin::AudioPositionHandler,     // our "get" property handler
        sizeof(KSPROPERTY),             // minimum buffer length for property
        sizeof(KSAUDIO_POSITION),       // minimum buffer length for returned data
        CPin::AudioPositionHandler,     // our "set" property handler
        NULL,                           // default values
        0,                              // related properties
        NULL,
        NULL,                           // no raw serialization handler
        0                                // don't serialize        
    ), 
#if 0
    DEFINE_KSPROPERTY_ITEM
    (
        KSPROPERTY_AUDIO_SAMPLING_RATE,
        CPin::AudioSampleRateHandler,
        sizeof(KSPROPERTY),
        sizeof(KSAUDIO_POSITION),
        CPin::AudioSampleRateHandler,
        NULL,
        0,
        NULL,
        NULL,
        0
    )
#endif
};

// �������Լ��б�
// Ψһ��Property Set��ϵͳ����ı�׼�����Լ�
DEFINE_KSPROPERTY_SET_TABLE (gPinPropertySet)
{
    DEFINE_KSPROPERTY_SET
    (
        &KSPROPSETID_Audio,
        SIZEOF_ARRAY(gPinProperty),
        gPinProperty,
        0,
        NULL
    )
};

// �����Կر�
// �Կر������ԡ��������¼��������
// ����û�б�Ҫ������£����Բ���������
// ��������ʵ��������
DEFINE_KSAUTOMATION_TABLE (gPinAutomation)
{
    DEFINE_KSAUTOMATION_PROPERTIES(gPinPropertySet),
    DEFINE_KSAUTOMATION_METHODS_NULL,
    DEFINE_KSAUTOMATION_EVENTS_NULL
};

// ����Pin������
const KSPIN_DESCRIPTOR_EX gPins [2] = 
{
    // ����Pin
    // ��ʵ������Pin����Ϊ1��������һ������Pin��
    {
        &gPinDispatch,
        &gPinAutomation,
        {
            DEFINE_KSPIN_DEFAULT_INTERFACES,
            DEFINE_KSPIN_DEFAULT_MEDIUMS,
            SIZEOF_ARRAY(gPinDataFormatRanges), 
            gPinDataFormatRanges,
            KSPIN_DATAFLOW_IN,
            KSPIN_COMMUNICATION_BOTH,
            0, 0, 0
        },
        0,
        1,
        1,
        &gAllocatorFraming,
        reinterpret_cast <PFNKSINTERSECTHANDLEREX> (CPin::IntersectHandler)
    },

    // ���Pin
    // �ѿ�ʵ������Pin��������Ϊ2���������������Pin
    {
        &gPinDispatch,
        &gPinAutomation,
        {
            DEFINE_KSPIN_DEFAULT_INTERFACES, 
            DEFINE_KSPIN_DEFAULT_MEDIUMS,
            SIZEOF_ARRAY(gPinDataFormatRanges),
            gPinDataFormatRanges,
            KSPIN_DATAFLOW_OUT,
            KSPIN_COMMUNICATION_BOTH,
            0, 0, 0
        },
        0,
        2,
        1,
        &gAllocatorFraming,
        reinterpret_cast <PFNKSINTERSECTHANDLEREX> (CPin::IntersectHandler)
    }
};

// =====================���濪ʼ��Filter�Ķ���

const KSFILTER_DISPATCH gFilterDispatch = 
{
    CFilter::DispatchCreate,         // Filter Create
    NULL,                            // Filter Close
    CFilter::DispatchProcess,        // Filter Process
    NULL                             // Filter Reset
};

// ������Node��Connection������
const KSNODE_DESCRIPTOR gNodes[] =
{
    DEFINE_NODE_DESCRIPTOR(NULL, &KSNODETYPE_DEMUX, NULL)
};

// Filter��������
// KS�����Ͷ�����GUID����ʾ�ģ���������һ��GUIDֵ
// �������������GUID��
// KSCATEGORY_AUDIO���������Filter������������Ƶ���ݵģ�������Ƶ���ݣ�
// KSCATEGORY_DATATRANSFORM���������Filter����Ƶ���ݽ���ת������������ѹ��������������������
const GUID gFilterCategories [2] = 
{
    STATICGUIDOF (KSCATEGORY_AUDIO),
    STATICGUIDOF (KSCATEGORY_DATATRANSFORM)
};

// �������ǵ�ʾ��Driver����ʵ��û���������Node����
// �����Node����ʡȥ�������Connection�������н���
// ʣ��һ�����������������Pin��
// {KSFILTER_NODE,  0, KSFILTER_NODE,  1}
KSTOPOLOGY_CONNECTION gConnections[] = 
{
    { KSFILTER_NODE,  0,    0, 1},
    { 0, 0,   KSFILTER_NODE,   1}
};

// Filter���������豸�������п��԰���һ��Filter��������
// ������Ҫ���������ĸ���������0�����߶�����±������Ψһ��Filter��������
const KSFILTER_DESCRIPTOR gSplitterFilter = 
{
    &gFilterDispatch,                        // Dispatch Table
    NULL,                                    // Automation Table
    KSFILTER_DESCRIPTOR_VERSION,            // Version
    KSFILTER_FLAG_DISPATCH_LEVEL_PROCESSING,// Flags
    &KSNAME_Filter,                            // Reference GUID
    DEFINE_KSFILTER_PIN_DESCRIPTORS (gPins),
    DEFINE_KSFILTER_CATEGORIES (gFilterCategories),
    DEFINE_KSFILTER_NODE_DESCRIPTORS(gNodes),
    DEFINE_KSFILTER_CONNECTIONS(gConnections),
    NULL                                    // Component ID
};

DEFINE_KSFILTER_DESCRIPTOR_TABLE (gKsFilters) 
{
    &gSplitterFilter
};

// ===============================�豸������

const KSDEVICE_DESCRIPTOR gKsSplitterDevice = 
{
    // ��һ����������Pin-Centre���͵�AVStream�����Ƿǳ���Ҫ�ģ�
    // �෴������������Filter-Centre���͵���������һ�����ʡ�ԡ�
    // ����һ��PNP��صĻص�����������Start/Stop/Power�ȵȻص���
    NULL,

    // ������Filter������
    SIZEOF_ARRAY (gKsFilters),
    gKsFilters,

    // ��������������������Ϊ0
    0,
    0
};
