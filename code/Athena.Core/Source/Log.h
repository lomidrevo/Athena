#pragma once

#include <stdio.h>

#include "Mutex.h"
#include "Singleton.h"

#define MAX_LOG_LINE_LENGTH		512
#define MAX_STRING_LENGTH		MAX_LOG_LINE_LENGTH

#define LOG(logLevel, ...)		Log::Instance()->Add(logLevel, __VA_ARGS__);
#define LOG_L(logLevel, ...)	Log::Instance()->AddLine(logLevel, __VA_ARGS__);
#define LOG_T(logLevel, ...)	Log::Instance()->AddTime(logLevel, __VA_ARGS__);
#define LOG_TL(logLevel, ...)	Log::Instance()->AddLineWithTime(logLevel, __VA_ARGS__);

#ifdef DEBUG
#define LOG_DEBUG(...)			LOG_TL(LogLevel::Debug, __VA_ARGS__)
#define LOG_MEM(...)			LOG_TL(LogLevel::DebugMem, __VA_ARGS__)
#else
#define LOG_DEBUG(...)			{ }
#define LOG_MEM(...)			{ }
#endif

#define LOG_LEVEL_VALUES(_) \
    _(Zero,) \
    _(Error,) \
    _(Warning,) \
    _(Info,) \
    _(Debug,) \
    _(DebugMem,) \
    _(Everything,) \

DECLARE_ENUM(LogLevel, LOG_LEVEL_VALUES)
#undef LOG_LEVEL_VALUES


class DLL_EXPORT Log : public Singleton<Log>
{
public:

	Log();
	~Log();

	void Initialize(const char* logName, bool addStartTime = false);

	inline void SetLogLevel(LogLevel::Enum newMaxLevel) { maxLogLevel = newMaxLevel; }

	void Add(LogLevel::Enum logLevel, const char* format, ...);
	void AddTime(LogLevel::Enum logLevel);
	void AddLine(LogLevel::Enum logLevel, const char* format, ...);
	void AddLineWithTime(LogLevel::Enum logLevel, const char* format, ...);

private:

	inline bool IsLogGranted(LogLevel::Enum logLevel) { return (logLevel <= maxLogLevel); }
	
	void CreateLog();
	void OpenLog();
	void CloseLog();

private:

	char* startString;

	Mutex logAccess;

	char logLine[MAX_LOG_LINE_LENGTH + 1];
	//std::ofstream fileStream;
	FILE* file;

	char* filename;
	char* logName;
	LogLevel::Enum maxLogLevel;
};
