#ifndef __mutex_h
#define __mutex_h

#include "TypeDefs.h"
#include <Windows.h>


class DLL_EXPORT Mutex
{
public:

	Mutex(bool startLocked = false)
	{
		::InitializeCriticalSection(&criticalSection);

		if (startLocked)
			Lock();
	}

	~Mutex()
	{
		UnLock();
	}
	
	inline void Lock() { ::EnterCriticalSection(&criticalSection); }
	inline void UnLock() { ::LeaveCriticalSection(&criticalSection); }
	
private:

	CRITICAL_SECTION criticalSection;
};

#endif __mutex_h
