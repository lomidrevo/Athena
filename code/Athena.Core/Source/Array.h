#ifndef __array_h
#define __array_h

#include "TypeDefs.h"


template <typename T> struct array_of
{
	T* ptr;
	uint count;

	__device__ array_of(T* ptr = null, uint count = 0)
	{
		this->ptr = ptr;
		this->count = count;
	}

	inline __device__ T& operator[](uint index) const
	{
		ASSERT(index < count);
		return ptr[index];
	}
};

typedef array_of<char> String;

// warning C4251 : ... needs to have dll - interface to be used by clients of struct ...
#define DLL_EXPORT_ARRAY_OF(type) template struct DLL_EXPORT array_of<type>;

#endif __array_h
