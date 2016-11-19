#ifndef __list_h
#define __list_h

#include "Array.h"
#include "Log.h"
#include "MemoryManager.h"
#include <memory.h>
#include "StringHelpers.h"
#include "TypeDefs.h"


template <typename T> struct list_of
{
	array_of<T> array;
	uint currentCount;

	MemoryManager* memoryManagerInstance;

	list_of() : memoryManagerInstance(null), currentCount(0)
	{
		this->array.ptr = null;
		this->array.count = 0;
	}

	list_of(MemoryManager* memoryManagerInstance, const char* name = null, uint count = 64)
	{
		Initialize(memoryManagerInstance, name, count);
	}

	~list_of()
	{
		Destroy();
	}

	void Initialize(MemoryManager* memoryManagerInstance, const char* name = null, uint count = 64)
	{
		this->memoryManagerInstance = memoryManagerInstance;
		this->currentCount = 0;

		Expand(count);
	}

	void Destroy()
	{
		if (memoryManagerInstance && array.ptr)
		{
			memoryManagerInstance->Free<T>(&array);
			currentCount = 0;
			memoryManagerInstance = null;
		}
	}

	__device__ inline T& operator[](uint index) const
	{
		ASSERT(index < currentCount);
		return array[index];
	}

	uint Add(T& t)
	{
		if (currentCount == array.count)
			Expand(array.count * 2);

		array[currentCount] = t;
		currentCount++;

		return currentCount - 1;
	}

	uint Add(uint count = 1)
	{
		if (count == 1 && currentCount == array.count)
			Expand(array.count * 2);
		else if (count > 1 && array.count - currentCount < count)
			Expand(MAX2(array.count * 2, currentCount + count));

		currentCount += count;
		return currentCount - count;
	}

	void Expand(uint newCount)
	{
		array_of<T> newArray = memoryManagerInstance->Alloc<T>(newCount);
		memset(newArray.ptr, 0, sizeof(T)* newArray.count);
		memcpy(newArray.ptr, array.ptr, sizeof(T)* array.count);

		memoryManagerInstance->Free<T>(&array);
		array = newArray;

		//char tmpBuffer[256] = {};
		//LOG_MEM("list_of::Expand [%d %s]", array.count,
		//	Common::Strings::GetMemSizeString(tmpBuffer, array.count * sizeof(T)));
	}

	void Clear()
	{
		memset(array.ptr, 0, sizeof(T)* currentCount);
		currentCount = 0;
	}

	array_of<T> CopyToArray() const
	{
		array_of<T> result;
		if (!currentCount)
			return result;

		result = memoryManagerInstance->Alloc<T>(currentCount);
		memcpy(result.ptr, array.ptr, currentCount * sizeof(T));

		return result;
	}
};

// warning C4251 : ... needs to have dll - interface to be used by clients of struct ...
#define DLL_EXPORT_LIST_OF(type) template struct DLL_EXPORT list_of<type>;

#endif __list_h
