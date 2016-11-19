
#include "Log.h"
#include "MemoryManager.h"
#include "StringHelpers.h"

#define MEM_PTR_IN_RANGE(ptr) (((uint64)ptr - (uint64)memory) < memorySize)


using namespace Common::Strings;


MemoryManager::MemoryManager()
{
	memory = null;
	baseMemoryDescriptor = null;
	memorySize = maxMemoryUsed = currentMemoryUsed = 0;
}

MemoryManager::~MemoryManager()
{
	using namespace Common::Strings;

	char tmpBuffer[256] = {};

	ASSERT(!baseMemoryDescriptor->used)
		
	CombineUnusedMemoryBlocks(baseMemoryDescriptor);
	ASSERT(baseMemoryDescriptor->size == memorySize - sizeof(MemoryDescriptor))

	// set all pointers as invalid
	auto memDescriptor = baseMemoryDescriptor;
	while (MEM_PTR_IN_RANGE(memDescriptor) && memDescriptor->size != 0)
	{
		if (memDescriptor->used)
			LOG_MEM("MemoryManager::Destroy [ptr still valid! %s]", GetMemSizeString(tmpBuffer, memDescriptor->size));

		memDescriptor->used = 0;
		memDescriptor = (MemoryDescriptor*)((uint8*)memDescriptor + sizeof(MemoryDescriptor) + memDescriptor->size);
	}

	VirtualFree(memory, 0, MEM_RELEASE);
	memory = null;
	memorySize = 0;

	LOG_TL(LogLevel::Info, "MemoryManager::Destroy [max memory usage %s]", GetMemSizeString(tmpBuffer, maxMemoryUsed));
}

void MemoryManager::Initialize(uint64 size)
{
	ASSERT(memory == null)
	ASSERT(size % 8 == 0)

	// kvoli debugovaniu, vzdy bude pamat zacinat na rovnakom mieste
	auto baseAddress = (LPVOID)TERABYTES((uint64)1);

	memorySize = size;
	memory = VirtualAlloc(baseAddress, (SIZE_T)memorySize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

	char tmpBuffer[256] = {};
	LOG_TL(LogLevel::Info, "MemoryManager::Initialize [%s]", GetMemSizeString(tmpBuffer, memorySize));

	const auto count64 = memorySize / 8;
	for (auto i = 0; i < count64; ++i)
	{
		ASSERT((int64*)memory + i != null)
		*((int64*)memory + i) = 0;
	}

	baseMemoryDescriptor = (MemoryDescriptor*)memory;
	ASSERT(baseMemoryDescriptor != null)

	baseMemoryDescriptor->size = memorySize - sizeof(MemoryDescriptor);
}

void* MemoryManager::Alloc(uint64 elementSize, const char* elementName, uint64 count)
{
	void* result = null;

	criticalSection.Lock();
	{
		ASSERT(memory != null)

		uint64 size = elementSize * count;

		if (size < MEM_MAX_ALLOC_SIZE)
		{
			auto memDescriptor = baseMemoryDescriptor;
			while (MEM_PTR_IN_RANGE(memDescriptor) && memDescriptor->size != 0)
			{
				if (!memDescriptor->used &&
					(memDescriptor->size == size || (memDescriptor->size > size + sizeof(MemoryDescriptor))))
					break;

				memDescriptor = (MemoryDescriptor*)((uint8*)memDescriptor + sizeof(MemoryDescriptor)+
					memDescriptor->size);
			}

			if (!MEM_PTR_IN_RANGE(memDescriptor))
			{
				// TODO add more free space to memory ?
			}
			else
			{
				result = (void*)((uint8*)memDescriptor + sizeof(MemoryDescriptor));

				if (memDescriptor->size > size)
				{
					// add new unused descriptor
					MemoryDescriptor* unusedDescriptor = (MemoryDescriptor*)((uint8*)result + size);
					unusedDescriptor->used = 0;
					unusedDescriptor->size = memDescriptor->size - (sizeof(MemoryDescriptor)+size);
				}

				memDescriptor->size = size;
				memDescriptor->used = 1;

				currentMemoryUsed += memDescriptor->size + sizeof(MemoryDescriptor);
				if (currentMemoryUsed > maxMemoryUsed)
					maxMemoryUsed = currentMemoryUsed;

				char tmpBuffer[256] = {};
				LOG_MEM("MemoryManager::Alloc [%s, %s]", GetMemSizeString(tmpBuffer, size), elementName);
			}
		}
	}
	criticalSection.UnLock();

	return result;
}

String MemoryManager::Alloc(const char* stringValue)
{
	size_t length = strlen(stringValue);
	String result((char*)Alloc(sizeof(char), "String", length + 1), length);

	strcpy(result.ptr, stringValue);

	return result;
}

void MemoryManager::Free(void* ptr, const char* elementName)
{
	if (!ptr || !MemoryManager::IsValid(ptr))
		return;

	criticalSection.Lock();
	{
		auto memDescriptor = (MemoryDescriptor*)((uint8*)ptr - sizeof(MemoryDescriptor));

		ASSERT(memDescriptor->used == 1)

		memDescriptor->used = 0;

		currentMemoryUsed -= memDescriptor->size + sizeof(MemoryDescriptor);

		char tmpBuffer[256] = {};
		LOG_MEM("MemoryManager::Free [%s, %s]", GetMemSizeString(tmpBuffer, memDescriptor->size), elementName);

		CombineUnusedMemoryBlocks(null);
	}
	criticalSection.UnLock();
}

void MemoryManager::CombineUnusedMemoryBlocks(MemoryDescriptor* firstMemoryDescriptor)
{
	// TODO vypocitat aktualnu velkost obsadenej pamate - currentMemoryUsed

	if (firstMemoryDescriptor == null)
		firstMemoryDescriptor = baseMemoryDescriptor;

	auto nextMemDescriptor = (MemoryDescriptor*)((uint8*)firstMemoryDescriptor + sizeof(MemoryDescriptor) + 
		firstMemoryDescriptor->size);

	if (firstMemoryDescriptor->used)
	{
		// find first unused memory descriptor
		while (nextMemDescriptor->used)
		{
			nextMemDescriptor = (MemoryDescriptor*)((uint8*)nextMemDescriptor + sizeof(MemoryDescriptor) + 
				nextMemDescriptor->size);
		}

		firstMemoryDescriptor = nextMemDescriptor;
		nextMemDescriptor = (MemoryDescriptor*)((uint8*)nextMemDescriptor + sizeof(MemoryDescriptor) + 
			nextMemDescriptor->size);
	}

	while (MEM_PTR_IN_RANGE(nextMemDescriptor) && !nextMemDescriptor->used)
	{
		firstMemoryDescriptor->size += nextMemDescriptor->size + sizeof(MemoryDescriptor);

		auto tmpOldSize = nextMemDescriptor->size;

		nextMemDescriptor->size = 0;
		nextMemDescriptor = (MemoryDescriptor*)((uint8*)nextMemDescriptor + sizeof(MemoryDescriptor) + tmpOldSize);
	}

	if (MEM_PTR_IN_RANGE(nextMemDescriptor))
		CombineUnusedMemoryBlocks(nextMemDescriptor);
}

uint64 MemoryManager::GetFreeMemorySize() const
{
	return memorySize - GetUsedMemorySize();
}

uint64 MemoryManager::GetUsedMemorySize() const
{
	uint64 usedMemory = 0;
	auto memDescriptor = baseMemoryDescriptor;
	while (MEM_PTR_IN_RANGE(memDescriptor) && memDescriptor->size != 0)
	{
		if (memDescriptor->used)
			usedMemory += memDescriptor->size + sizeof(MemoryDescriptor);
		else
			usedMemory += sizeof(MemoryDescriptor);

		memDescriptor = (MemoryDescriptor*)((uint8*)memDescriptor + sizeof(MemoryDescriptor) + memDescriptor->size);
	}

	return usedMemory;
}
