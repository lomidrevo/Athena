
#include "Log.h"
#include "StringHelpers.h"
#include <time.h>

using namespace Common::Strings;


Log::Log()
{
	file = null;
	startString = null;
	filename = null;
	logName = null;
	SetLogLevel(LogLevel::Everything);
}

Log::~Log()
{
	CloseLog();

	logAccess.Lock();

	SAFE_DELETE_ARRAY(filename);
	SAFE_DELETE_ARRAY(logName);
	SAFE_DELETE_ARRAY(startString);

	logAccess.UnLock();
}

void Log::OpenLog()
{
	// TODO vytvorit samostatny adresar pre log

	if (filename) 
	//	fileStream.open(filename, std::ios::out | std::ios::app);
		file = fopen(filename, "a");
}

void Log::CloseLog()
{
	//fileStream.close();
	if (file != null)
	{
		fclose(file);
		file = null;
	}
}

void Log::Add(LogLevel::Enum logLevel, const char* format, ...)
{
	if (!IsLogGranted(logLevel))
		return;

	logAccess.Lock();

	OpenLog();
	//if (fileStream.is_open())
	if (file != null)
	{
		va_list	ap;	

		va_start(ap, format);
		vsprintf(logLine, format, ap);
		va_end(ap);
		
		//fileStream << logLine;
		fprintf(file, logLine);

		printf(logLine);

		CloseLog();
	}

	logAccess.UnLock();
}

void Log::AddLine(LogLevel::Enum logLevel, const char* format, ...)
{
	if (!IsLogGranted(logLevel))
		return;

	logAccess.Lock();

	OpenLog();
	//if (fileStream.is_open())
	if (file != null)
	{
		va_list	ap;	

		va_start(ap, format);
		vsprintf(logLine, format, ap);
		va_end(ap);

		//fileStream << logLine;
		//fileStream << "\n";
		fprintf(file, logLine);
		fprintf(file, "\n");

		printf(logLine);
		printf("\n");

		CloseLog();
	}

	logAccess.UnLock();
}

void Log::AddTime(LogLevel::Enum logLevel)
{
	if (!IsLogGranted(logLevel))
		return;

	logAccess.Lock();

	OpenLog();
	//if (fileStream.is_open())
	if (file != null)
	{
		time_t rawTime;
		time(&rawTime);
		tm* t = localtime(&rawTime);

		sprintf(logLine, "%d:%d:%d", t->tm_hour, t->tm_min, t->tm_sec);
		
		//fileStream << logLine;
		fprintf(file, logLine);

		printf(logLine);

		CloseLog();
	}

	logAccess.UnLock();
}

void Log::AddLineWithTime(LogLevel::Enum logLevel, const char* format, ...)
{
	if (!IsLogGranted(logLevel))
		return;

	logAccess.Lock();

	OpenLog();
	if (file != null)
	{
		va_list	ap;	

		va_start(ap, format);
		vsprintf(logLine, format, ap);
		va_end(ap);

		char timeStr[20];
		char tmpBuffer[MAX_LOG_LINE_LENGTH] = {};

		sprintf(tmpBuffer, "%s\t%25s:%-10s\t%s\n",
			GetTimeString(timeStr, "Y/M/D h:m:s"),
			logName,
			LogLevel::GetString(logLevel),
			logLine);

		fprintf(file, "%s", tmpBuffer);
		printf("%s", tmpBuffer);

		CloseLog();
	}

	logAccess.UnLock();
}

void Log::Initialize(const char* logName, bool addStartTime)
{
	SAFE_DELETE_ARRAY(this->filename);
	SAFE_DELETE_ARRAY(this->logName);

	if (!startString)
	{
		// get start of application string (yyyyMMdd.HHmmss)
		startString = new char[strlen("YYYYMMDD.hhmmss") + 1];
		GetTimeString(startString, "YMD.hms");
	}

	this->logName = new char[strlen(logName) + 1];
	sprintf(this->logName, "%s", logName);

#ifdef COMMON_LOG_FILENAME
	if (!addStartTime)
	{
		this->filename = new char[strlen(COMMON_LOG_FILENAME) + strlen(".log") + 1];
		sprintf(this->filename, "%s.log", COMMON_LOG_FILENAME);
	}
	else
	{
		this->filename = new char[strlen(COMMON_LOG_FILENAME) + strlen(startString) + strlen("_.log") + 1];
		sprintf(this->filename, "%s_%s.log", COMMON_LOG_FILENAME, startString);
	}
#else
	if (!addStartTime)
	{
		this->filename = new char[strlen(logName) + strlen(".log") + 1];
		sprintf(this->filename, "%s.log", logName);
	}
	else
	{
		this->filename = new char[strlen(logName) + strlen(startString) + strlen("_.log") + 1];
		sprintf(this->filename, "%s_%s.log", logName, startString);
	}
#endif

	OpenLog();
	CloseLog();
}
