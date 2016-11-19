#include "Timer.h"
#include <windows.h>


namespace Timers
{
	DLL_EXPORT void Reset(Timer& timer)
	{
		((LARGE_INTEGER*)&timer.start)->QuadPart = ((LARGE_INTEGER*)&timer.stop)->QuadPart;
	}

	// returns duration in seconds
	DLL_EXPORT f32 GetDuration(const Timer& timer)
	{
		return (f32)(((LARGE_INTEGER*)&timer.stop)->QuadPart - ((LARGE_INTEGER*)&timer.start)->QuadPart) / 
				(f32)((LARGE_INTEGER*)&timer.freq)->QuadPart;
	}

	DLL_EXPORT void Start(Timer& timer)
	{
		QueryPerformanceCounter((LARGE_INTEGER*)&timer.start);
	}

	DLL_EXPORT void Stop(Timer& timer)
	{
		if (QueryPerformanceCounter((LARGE_INTEGER*)&timer.stop))
		{
			if (!timer.freq)
				QueryPerformanceFrequency((LARGE_INTEGER*)&timer.freq);

			timer.lastDurationSec = GetDuration(timer);
			timer.lastDurationMs = timer.lastDurationSec * 1000;
		}
	}

	DLL_EXPORT int64 GetTickCount(const Timer& timer)
	{
		return ((LARGE_INTEGER*)&timer.stop)->QuadPart - ((LARGE_INTEGER*)&timer.start)->QuadPart;
	}
}
