#include "Log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "StringHelpers.h"
#include <time.h>


namespace Common
{
	namespace Strings
	{
		DLL_EXPORT const char* GetMemSizeString(char* outputBuffer, uint64 memSize)
		{
			MemUnit::Enum unit = MemUnit::B;
			auto memSizeTmp = (real)memSize;
			while (memSizeTmp > 1024)
			{
				memSizeTmp /= 1024;
				unit = (MemUnit::Enum)((int)unit + 1);
			}

			if (unit == MemUnit::B)
				sprintf(outputBuffer, "%d%s", (int)memSizeTmp, MemUnit::GetString(MemUnit::B));
			else
				sprintf(outputBuffer, "~%d%s", (int)memSizeTmp, MemUnit::GetString(unit));
			
			return outputBuffer;
		}

		DLL_EXPORT const char* GetTimeString(char* outputBuffer, const char* format)
		{
			time_t rawTime;
			time(&rawTime);
			tm* t = localtime(&rawTime);

			int outputPos = 0;
			int formatLength = (int)strlen(format);
			for (int i = 0; i < formatLength; i++)
			{
				if (format[i] == 'Y')
				{
					sprintf(&outputBuffer[outputPos], "%d", t->tm_year + 1900);
					outputPos += 4;
				}
				else if (format[i] == 'M')
				{
					if ((t->tm_mon + 1) < 10)
						sprintf(&outputBuffer[outputPos], "0%d", t->tm_mon + 1);
					else
						sprintf(&outputBuffer[outputPos], "%d", t->tm_mon + 1);

					outputPos += 2;
				}
				else if (format[i] == 'D')
				{
					if ((t->tm_mday) < 10)
						sprintf(&outputBuffer[outputPos], "0%d", t->tm_mday);
					else
						sprintf(&outputBuffer[outputPos], "%d", t->tm_mday);

					outputPos += 2;
				}
				else if (format[i] == 'h')
				{
					if ((t->tm_hour) < 10)
						sprintf(&outputBuffer[outputPos], "0%d", t->tm_hour);
					else
						sprintf(&outputBuffer[outputPos], "%d", t->tm_hour);

					outputPos += 2;
				}
				else if (format[i] == 'm')
				{
					if ((t->tm_min) < 10)
						sprintf(&outputBuffer[outputPos], "0%d", t->tm_min);
					else
						sprintf(&outputBuffer[outputPos], "%d", t->tm_min);

					outputPos += 2;
				}
				else if (format[i] == 's')
				{
					if ((t->tm_sec) < 10)
						sprintf(&outputBuffer[outputPos], "0%d", t->tm_sec);
					else
						sprintf(&outputBuffer[outputPos], "%d", t->tm_sec);

					outputPos += 2;
				}
				else
				{
					sprintf(&outputBuffer[outputPos], "%c", format[i]);
					outputPos += 1;
				}
			}

			return outputBuffer;
		}

		DLL_EXPORT const char* GetDurationString(char* outputBuffer, real miliseconds)
		{
			// TODO get time duration string from miliseconds

			sprintf(outputBuffer, "%.3fms", miliseconds);
	
			return outputBuffer;
		}

		DLL_EXPORT int CountOccurence(const char* source, char c)
		{
			int result = 0;

			for (int i = 0; source[i] != 0; i++)
				if (source[i] == c)
					result++;

			return result;
		}

		DLL_EXPORT int IndexOfChar(const char* source, char c)
		{
			for (int i = 0; source[i] != 0; i++)
				if (source[i] == c)
					return i;

			return -1;
		}
	}
}
