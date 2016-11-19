#ifndef __memory_manager_h
#define __memory_manager_h

#include "Array.h"
#include "Mutex.h"
#include "Singleton.h"
#include "TypeDefs.h"


#define _MEM_ALLOC(instance, type)						(type*)instance->Alloc(sizeof(type), #type, 1)
#define _MEM_ALLOC_ARRAY(instance, type, count)			instance->Alloc<type>(count, #type)
#define _MEM_ALLOC_STRING(instance, stringValue)		instance->Alloc(stringValue)
#define _MEM_FREE(instance, ptr)						{ instance->Free(ptr, #ptr); ptr = null; }
#define _MEM_FREE_ARRAY(instance, type, array_of_ptr)	instance->Free<type>(array_of_ptr, #array_of_ptr)
#define _MEM_FREE_STRING(instance, array_of_char)		instance->Free<char>(array_of_char, null)

#define	MEM_MANAGER_INSTANCE							MemoryManager::Instance()
#define MEM_ALLOC(type)									_MEM_ALLOC(MEM_MANAGER_INSTANCE, type)
#define MEM_ALLOC_ARRAY(type, count)					_MEM_ALLOC_ARRAY(MEM_MANAGER_INSTANCE, type, count)
#define MEM_ALLOC_STRING(stringValue)					_MEM_ALLOC_STRING(MEM_MANAGER_INSTANCE, stringValue)
#define MEM_MANAGER_FREE(ptr)							_MEM_FREE(MEM_MANAGER_INSTANCE, ptr)

#define MEM_MAX_ALLOC_SIZE								0x1000000000000


struct MemoryDescriptor
{
	//ui64 size : 63; // max size: 0xCCCCCCCCCCCCCCC (2^63)
	//ui64 used : 1;

	ui64 size		: 48;	// max size: 0x1000000000000 = 2^48 = 256TB 
	ui64 _unused	: 15;
	ui64 used		: 1;	// 1 if pointer is used
};


class DLL_EXPORT MemoryManager : public Singleton<MemoryManager>
{
public:

	MemoryManager();
	~MemoryManager();

	void Initialize(ui64 size);

	void* Alloc(ui64 elementSize, const char* elementName = null, ui64 count = 1);
	template <typename T> array_of<T> Alloc(ui64 count, const char* elementName = null)
	{
		return array_of<T>((T*)Alloc(sizeof(T), elementName, count), count);
	}
	String Alloc(const char* stringValue);

	void Free(void* ptr, const char* elementName = null);
	template <typename T> void Free(array_of<T>* ptr, const char* elementName = null)
	{
		Free((void*)ptr->ptr, elementName);
		ptr->ptr = null;
		ptr->count = 0;
	}

	inline ui64 GetMemorySize() const { return memorySize; }

	ui64 GetFreeMemorySize() const;
	ui64 GetUsedMemorySize() const;

	static bool IsValid(const void* ptr)
	{ 
		return ((MemoryDescriptor*)((ui8*)ptr - sizeof(MemoryDescriptor)))->used; 
	}

private:

	void CombineUnusedMemoryBlocks(MemoryDescriptor* firstMemoryDescriptor);

	ui64 maxMemoryUsed, currentMemoryUsed;
	ui64 memorySize;

	void* memory;

	Mutex criticalSection;
	MemoryDescriptor* baseMemoryDescriptor;
};

#endif __memory_manager_h
