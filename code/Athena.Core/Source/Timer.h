#ifndef __timer_h
#define __timer_h

#include "TypeDefs.h"


struct Timer
{
	ui64 start, stop;
	f32 lastDurationMs;
	f32 lastDurationSec;
	ui64 freq;
};

namespace Timers
{
	DLL_EXPORT void Reset(Timer& timer);
	// returns duration in seconds
	DLL_EXPORT f32 GetDuration(const Timer& timer);
	DLL_EXPORT void Start(Timer& timer);
	DLL_EXPORT void Stop(Timer& timer);
	DLL_EXPORT int64 GetTickCount(const Timer& timer);
}

#define BEGIN_TIMED_BLOCK(timer)	Timers::Start(timer);
#define END_TIMED_BLOCK(timer)		Timers::Stop(timer);

struct TimedBlock
{
	Timer* timer;

	TimedBlock(Timer* timer)
	{
		this->timer = timer;
		BEGIN_TIMED_BLOCK(*timer);
	}

	~TimedBlock()
	{
		if (timer)
			END_TIMED_BLOCK(*timer);
		timer = null;
	}
};

#define TIMED_BLOCK(timer)		TimedBlock timedBlock##__COUNTER__(timer);

#endif __timer_h
