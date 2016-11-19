#ifndef __type_defs_h
#define __type_defs_h

#pragma warning(disable : 4482)	// nonstandard extension used: enum '...' used in qualified name
#pragma warning(disable : 4996) // sprintf, vsprintf, localtime may me unsafe ...


#ifdef DEBUG
#define ASSERT(expression) \
	if (!(expression)) \
		*(int*)0 = 0;
#else
#define ASSERT(expression) { }
#endif

#ifdef ATHENA_CUDA
#include <C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v7.5\include\host_defines.h>
#else
#define __device__
#endif

#define NULL	0
#define null	NULL

// long double
#define PI128		3.141592653589793238L
// double
#define PI64		3.141592653589793
// real
#define PI32		3.1415927f

#define _INFINITY	(real)(1e+8)
#define EPSILON		(real)(1e-6)

#define DLL_EXPORT			__declspec(dllexport)
#define EXTERN_C			extern "C"
#define ATHENA_DLL_EXPORT	EXTERN_C DLL_EXPORT

#define KILOBYTES(value)	(value*1024LL)
#define MEGABYTES(value)	(KILOBYTES(value)*1024LL)
#define GIGABYTES(value)	(MEGABYTES(value)*1024LL)
#define TERABYTES(value)	(GIGABYTES(value)*1024LL)

// expansion macro for enum value definition
#define ENUM_VALUE(name, assign) name assign,

// expansion macro for enum to string conversion
#define ENUM_CASE(name, assign) case name: return #name;

// declare the access function and define enum values
#define DECLARE_ENUM(EnumName, ENUM_VALUES) \
	struct EnumName \
	{ \
		enum Enum \
		{ \
			ENUM_VALUES(ENUM_VALUE) \
			ENUM_VALUE(Count,) \
		}; \
		static const char* GetString(EnumName::Enum enumValue) \
		{ \
			switch (enumValue) \
			{ \
				ENUM_VALUES(ENUM_CASE) \
				default: return "unknown value!"; /* handle input error */ \
			} \
		} \
	};

// basic types
typedef unsigned __int8		uint8;
typedef unsigned __int16	uint16;
typedef unsigned __int32	uint32;
typedef unsigned __int64	uint64;
typedef signed __int8		int8;
typedef signed __int16		int16;
typedef signed __int32		int32;
typedef signed __int64		int64;
typedef float				real32;
typedef double				real64;
typedef int32				bool32;
typedef int16				bool16;
typedef uint64				uint;

// basic types
typedef uint8				ui8;
typedef uint16				ui16;
typedef uint32				ui32;
typedef uint64				ui64;
typedef int8				i8;
typedef int16				i16;
typedef int32				i32;
typedef int64				i64;
typedef real32				f32;
typedef real64				f64;
typedef bool16				b16;
typedef bool32				b32;

#define TYPE_VALUES(_) \
	_(undefined, = 0) \
	_(ui8, ) \
	_(ui16, ) \
	_(ui32, ) \
	_(ui64, ) \
	_(i8, ) \
	_(i16, ) \
	_(i32, ) \
	_(i64, ) \
	_(f32, ) \
	_(f64, ) \
	_(b16, ) \
	_(b32, ) \
	_(real, ) \
	_(uint, ) \
	_(v3f, ) \
	_(v4f, ) \
	_(v2i, ) \
	_(v3i, ) \
	_(v4i, ) \
	_(frameBufferEnum,) \
	_(rendererEnum,) \
	_(renderingMethodEnum,) \
	_(tracingMethodEnum,) \
	_(renderingModeEnum,) \
	_(timer, )
DECLARE_ENUM(Type, TYPE_VALUES)
#undef TYPE_VALUES

#define REAL_AS_DOUBLE
#ifdef REAL_AS_DOUBLE
typedef real64				real;
#define _PI					PI64
#else
typedef real32				real;
#define _PI					PI32
#endif

#define ABS(i)						((i) > 0 ? (i) : -(i))
#define MAX2(a, b)					((a) > (b) ? (a) : (b))
#define MIN2(a, b)					((a) < (b) ? (a) : (b))
#define MAX3(a, b, c)				((a) > (b) ? ((a) > (c) ? (a) : (c)) : ((b) > (c) ? (b) : (c)))
#define MIN3(a, b, c)				((a) < (b) ? ((a) < (c) ? (a) : (c)) : ((b) < (c) ? (b) : (c)))
#define SWAP(a, b, T)				{ T* temp = a; a = b; b = temp; }			
#define CLAMP(value, minValue, maxValue) \
{ \
	if (value < minValue) value = minValue; \
	else if (value > maxValue) value = maxValue; \
}
// NOTE unsafe for dynamic arrays
#define ARRAY_COUNT(array)			(sizeof(array) / sizeof(*(array)))
#define SAFE_DELETE_ARRAY(ptr)		{ if (ptr) delete [] ptr; ptr = NULL; }
#define SAFE_DELETE(ptr)			{ if (ptr) delete ptr; ptr = NULL; }
#define REAL_TO_BYTE(value)			(ui8)((value) * 255)

#endif __type_defs_h
