#ifndef __string_helpers_h
#define __string_helpers_h

#include "TypeDefs.h"

#define MEM_UNIT_VALUES(_) \
	_(B, ) \
	_(KB, ) \
	_(MB, ) \
	_(GB, ) \
	_(TB, ) \
	_(PB, )
DECLARE_ENUM(MemUnit, MEM_UNIT_VALUES)
#undef MEM_UNIT_VALUES

namespace Common
{
	namespace Strings
	{
		DLL_EXPORT const char* GetMemSizeString(char* outputBuffer, ui64 memSize);
		DLL_EXPORT const char* GetTimeString(char* outputBuffer, const char* format);
		DLL_EXPORT const char* GetDurationString(char* outputBuffer, real miliseconds);
		DLL_EXPORT int CountOccurence(const char* source, char c);
		DLL_EXPORT int IndexOfChar(const char* source, char c);
	}
}

#endif __string_helpers_h
