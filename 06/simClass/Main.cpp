/*  
	��Ȩ��
	���ߣ����� 
	���ڣ�2009/9/1

	�ļ���Main.c
	˵������ں���
*/

#ifdef __cplusplus
extern "C"{
#endif

//#pragma warning(disable:4200)  // nameless struct/union
//#pragma warning(disable:4201)  // nameless struct/union
//#pragma warning(disable:4115)  // named typedef in parenthesis
//#pragma warning(disable:4214)  // bit field types other than int

#include <ntddk.h>

NTSTATUS DriverEntry(
		IN PDRIVER_OBJECT  DriverObject, 
		IN PUNICODE_STRING  RegistryPath
		);
#ifdef __cplusplus
}
#endif

 
__forceinline void* operator new(size_t size,
				   POOL_TYPE pool_type,
				   ULONG pool_tag) 
{
	ASSERT((pool_type < MaxPoolType) && (0 != size));
	if(size == 0)return NULL;

	// �жϼ���顣�ַ���������ϣ�ֻ�ܷ���Ƿ�ҳ�ڴ档
	ASSERT(pool_type ==  NonPagedPool || 
		(KeGetCurrentIrql() < DISPATCH_LEVEL));

	return ExAllocatePoolWithTag(pool_type,
		static_cast<ULONG>(size),
		pool_tag);
}

__forceinline void __cdecl operator delete(void* pointer) {
	ASSERT(NULL != pointer);
	if (NULL != pointer)
		ExFreePool(pointer);
}

class  clsInt{
public:
	clsInt(){m_nValue = 0;DbgPrint("clsInt. this=%p m_nValue = %d\n", this, m_nValue); }
	clsInt(int nValue){ m_nValue = nValue; DbgPrint("clsInt. this=%p m_nValue = %d\n", this, m_nValue);}
	
	~clsInt(){DbgPrint("~clsInt. this=%p m_nValue = %d\n", this, m_nValue);}

	void  print(){DbgPrint("print: m_nValue=%d\n", m_nValue);}
	operator int&(){return m_nValue;}

private:
	int  m_nValue;
};

NTSTATUS DriverEntry(
    IN PDRIVER_OBJECT  DriverObject, 
    IN PUNICODE_STRING  RegistryPath
    )
{
	clsInt obj1;
	clsInt* obj2 = new(NonPagedPool, 'abcd') clsInt(3);
	obj1++;		// 0->1
	(*obj2)--;	// 3->2

	obj1.print();
	obj2->print();

	delete obj2;

	// ��ģ�����ʧ��
	return STATUS_UNSUCCESSFUL;
}
