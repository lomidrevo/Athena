#ifndef __debug_h
#define __debug_h

#include "Array.h"
#include "MemoryManager.h"
#include "TypeDefs.h"


struct Parameter
{
	String name;
	Type::Enum type;
	bool readOnly;
	void* ptr;
	const void* readOnlyPtr;
	const void* maxValuePtr;

	// TODO min/max hodnota by sa mala ukladat sem a nacitavat podla type (cez union napriklad)
	ui64 minValue;
	ui64 maxValue;

	uint parentId;
	bool collapsed;

	Parameter(String name)
	{
		Set(name, Type::undefined, null);
	}

	Parameter(String name, Type::Enum type, void* ptr, const void* maxValue, uint parentId = 0) : maxValuePtr(maxValue)
	{
		Set(name, type, ptr, parentId);
	}

	Parameter(String name, Type::Enum type, const void* ptr, uint parentId = 0) : readOnlyPtr(ptr), maxValuePtr(null)
	{
		SetReadOnly(name, type, parentId);
	}

	void Set(String name, Type::Enum type, void* ptr, uint parentId = 0)
	{
		this->type = type;
		this->ptr = ptr;
		this->readOnly = false;
		this->name = name;
		this->parentId = parentId;
		collapsed = true;
	}

	void SetReadOnly(String name, Type::Enum type, uint parentId = 0)
	{
		this->type = type;
		this->ptr = ptr;
		this->readOnly = true;
		this->name = name;
		this->parentId = parentId;
		collapsed = true;
	}

	bool IsRegion() const { return type == Type::undefined && !ptr; }
};

#define DEBUG_REGION(memManagerInstance, storage, name) \
	storage->debugParameters.Add(Parameter(_MEM_ALLOC_STRING(memManagerInstance, name)));

#define DEBUG_PARAMETER(memManagerInstance, storage, name, type, ptr, maxValuePtr, parentId) \
	storage->debugParameters.Add(Parameter(_MEM_ALLOC_STRING(memManagerInstance, name), type, ptr, maxValuePtr, parentId));

#define DEBUG_PARAMETER_READONLY(memManagerInstance, storage, name, type, ptr, parentId) \
	storage->debugParameters.Add(Parameter(_MEM_ALLOC_STRING(memManagerInstance, name), type, ptr, parentId));

#endif __debug_h
