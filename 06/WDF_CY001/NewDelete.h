#pragma once

__forceinline void* __cdecl operator new(size_t size,
										 POOL_TYPE pool_type,
										 ULONG pool_tag) 
{
	ASSERT((pool_type < MaxPoolType) && (0 != size));
	if(size == 0)return NULL;

	// �жϼ���顣�ַ���������ϣ�ֻ�ܷ���Ƿ�ҳ�ڴ档
	ASSERT(pool_type ==  NonPagedPool || 
		(KeGetCurrentIrql() < DISPATCH_LEVEL));

	PVOID pRet = ExAllocatePoolWithTag(pool_type, size, pool_tag);
	if(pRet) RtlZeroMemory(pRet, size);

	return pRet;
}


__forceinline void __cdecl operator delete(void* pointer) 
{
	ASSERT(pointer);
	if (NULL != pointer)
		ExFreePool(pointer);
}