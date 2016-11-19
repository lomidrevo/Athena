// Win32CreateConsole()
#include <io.h>
#include <fcntl.h>
#include <stdio.h>
// Win32CreateConsoleWin10()
#include <iostream>
#include <cstdio>
#include <fstream>
#include <Win32.h>


DLL_EXPORT void Win32ToggleFullscreen(HWND window)
{
	DWORD windowStyle = GetWindowLong(window, GWL_STYLE);

	WINDOWPLACEMENT windowPosition = { sizeof(windowPosition) };

	if (windowStyle & WS_OVERLAPPEDWINDOW)
	{
		MONITORINFO monitorInfo = { sizeof(monitorInfo) };
		if (GetWindowPlacement(window, &windowPosition) &&
			GetMonitorInfo(MonitorFromWindow(window, MONITOR_DEFAULTTOPRIMARY), &monitorInfo))
		{
			SetWindowLong(window, GWL_STYLE, windowStyle & ~WS_OVERLAPPEDWINDOW);
			SetWindowPos(window, HWND_TOP, 
				monitorInfo.rcMonitor.left, monitorInfo.rcMonitor.top,
				monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left,
				monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top,
				SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
		}
	}
	else 
	{
		SetWindowLong(window, GWL_STYLE, windowStyle | WS_OVERLAPPEDWINDOW);
		SetWindowPlacement(window, &windowPosition);
		SetWindowPos(window, NULL, 0, 0, 0, 0, 
			SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
	}
}

DLL_EXPORT void Win32FreeFileMemory(win32_read_file_result* file)
{
	if (file->memory)
	{
		VirtualFree(file->memory, 0, MEM_RELEASE);
		file->memory = null;
		file->memorySize = 0;
	}
}

DLL_EXPORT bool32 Win32WriteFile(const char* filename, uint32 memorySize, void* memory)
{
	bool32 result = false;

	HANDLE fileHandle = CreateFileA(filename, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
	if (fileHandle != INVALID_HANDLE_VALUE)
	{
		DWORD bytesWritten;
		if (WriteFile(fileHandle, memory, memorySize, &bytesWritten, 0))
		{
			result = (bytesWritten == memorySize);
		}
		else
		{

		}

		CloseHandle(fileHandle);
	}
	else
	{

	}

	return result;
}

DLL_EXPORT win32_read_file_result Win32ReadFile(const char* filename)
{
	win32_read_file_result result = {};

	HANDLE fileHandle = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
	if (fileHandle != INVALID_HANDLE_VALUE)
	{
		LARGE_INTEGER fileSize;
		if (GetFileSizeEx(fileHandle, &fileSize))
		{
			// TODO toto asi nevie nacitat viac ako 4GB subor... treba prerobit

			result.memorySize = (uint32)fileSize.QuadPart;
			result.memory = VirtualAlloc(0, result.memorySize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
			if (result.memory)
			{
				DWORD bytesRead;
				if (ReadFile(fileHandle, result.memory, result.memorySize, &bytesRead, 0) && 
					result.memorySize == bytesRead)
				{

				}
				else
				{
					Win32FreeFileMemory(&result);
				}
			}
			else
			{

			}
		}
		else
		{

		}

		CloseHandle(fileHandle);
	}

	return result;
}

DLL_EXPORT void Win32GetWindowDimension(HWND window, vector2i& windowDimension)
{
	RECT clientRect;
	GetClientRect(window, &clientRect);
	windowDimension.x = clientRect.right - clientRect.left;
	windowDimension.y = clientRect.bottom - clientRect.top;
}

DLL_EXPORT void Win32CreateOffscreenBuffer(win32_offscreen_buffer* buffer, int width, int height)
{
	buffer->width = width;
	buffer->height = height;
	buffer->bytesPerPixel = 4;

	buffer->info.bmiHeader.biSize = sizeof(buffer->info.bmiHeader);
	buffer->info.bmiHeader.biWidth = buffer->width;
	buffer->info.bmiHeader.biHeight = buffer->height;
	buffer->info.bmiHeader.biPlanes = 1;
	buffer->info.bmiHeader.biBitCount = 32;
	buffer->info.bmiHeader.biCompression = BI_RGB;

	int bitmapMemorySize = buffer->width * buffer->height * buffer->bytesPerPixel;
	
	//buffer->memory = VirtualAlloc(0, bitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);

	buffer->pitch = width * buffer->bytesPerPixel;
}

DLL_EXPORT void Win32DisplayBufferInWindow(win32_offscreen_buffer* buffer, HDC deviceContext,
	int windowWidth, int windowHeight)
{
	if (windowWidth >= buffer->width * 2 && windowHeight >= buffer->height * 2)
	{
		StretchDIBits(deviceContext, 
			0, 0, 2*buffer->width, 2*buffer->height,
			0, 0, buffer->width, buffer->height,
			buffer->memory.ptr, &buffer->info, DIB_RGB_COLORS, SRCCOPY);
	}
	else
	{
		StretchDIBits(deviceContext, 
			0, 0, /*buffer->width, buffer->height,*/ windowWidth, windowHeight,
			0, 0, buffer->width, buffer->height,
			buffer->memory.ptr, &buffer->info, DIB_RGB_COLORS, SRCCOPY);
	}
}

//void Win32BeginRecordingInput(win32_state* state, int inputRecordingIndex)
//{
//	state->inputRecordingIndex = inputRecordingIndex;
//
//	state->recordingHandle = CreateFileA(WIN32_STATE_RECORDING_FILENAME, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
//
//	DWORD bytesToWrite = (DWORD)MemoryManager::Instance()->GetMemorySize();
//	ASSERT(MemoryManager::Instance()->GetMemorySize() == bytesToWrite);
//
//	DWORD bytesWritten;
//	WriteFile(state->recordingHandle, MemoryManager::Instance()->GetMemoryStart(), bytesToWrite, &bytesWritten, 0);
//}
//
//void Win32EndRecordingInput(win32_state* state)
//{
//	CloseHandle(state->recordingHandle);
//	state->recordingHandle = null;
//	state->inputRecordingIndex = 0;
//}
//
//void Win32BeginInputPlayback(win32_state* state, int inputPlaybackIndex)
//{
//	state->inputPlaybackIndex = inputPlaybackIndex;
//
//	state->playbackHandle = CreateFileA(WIN32_STATE_RECORDING_FILENAME, GENERIC_READ, FILE_SHARE_READ, 
//		0, OPEN_EXISTING, 0, 0);
//
//	DWORD bytesToRead = (DWORD)MemoryManager::Instance()->GetMemorySize();
//	ASSERT(MemoryManager::Instance()->GetMemorySize() == bytesToRead);
//
//	DWORD bytesRead;
//	ReadFile(state->playbackHandle, MemoryManager::Instance()->GetMemoryStart(), bytesToRead, &bytesRead, 0);
//}
//
//void Win32EndInputPlayback(win32_state* state)
//{
//	CloseHandle(state->playbackHandle);
//	state->playbackHandle = null;
//	state->inputPlaybackIndex = 0;
//}
//
//void Win32RecordInput(win32_state* state, application_input* input)
//{
//	DWORD bytesWritten;
//	WriteFile(state->recordingHandle, input, sizeof(*input), &bytesWritten, 0);
//}
//
//void Win32PlaybackInput(win32_state* state, application_input* input)
//{
//	DWORD bytesRead;
//	if (ReadFile(state->playbackHandle, input, sizeof(*input), &bytesRead, 0) && bytesRead)
//	{
//		// zo suboru nacitavam postupne po jednom frame, v tomto pripade este stale ostava nieco
//	}
//	else
//	{
//		// bol nacitany posledny frame
//		int playingIndex = state->inputPlaybackIndex;
//		Win32EndInputPlayback(state);
//		Win32BeginInputPlayback(state, playingIndex);
//	}
//}

DLL_EXPORT void Win32ProcessMouseButtonStateMessage(mouse_button_state* newState, bool16 isDown)
{
	newState->isDown = isDown;
}

DLL_EXPORT void Win32ProcessKeyboardStateMessage(button_state* newState, bool32 isDown)
{
	if (newState->isDown != isDown)
	{
		newState->isDown = isDown;
		++newState->halfTransitionCount;
	}
}

DLL_EXPORT FILETIME Win32GetLastWriteTime(const char* filename)
{
	FILETIME lastWriteTime = {};
	WIN32_FIND_DATA findData;
	HANDLE fileHandle = FindFirstFileA(filename, &findData);
	if (fileHandle != INVALID_HANDLE_VALUE)
	{
		lastWriteTime = findData.ftLastWriteTime;
		FindClose(fileHandle);
	}

	return lastWriteTime;
}

DLL_EXPORT void Win32GetExecutableDirectory(char* directoryBuffer)
{
	DWORD sizeOfExecutableFilePath = GetModuleFileNameA(0, directoryBuffer, MAX_PATH);
	char* onePastLastSlash = directoryBuffer + sizeOfExecutableFilePath;
	for (char* scan = directoryBuffer; *scan; ++scan)
		if (*scan == '\\')
			onePastLastSlash = scan + 1;
	*onePastLastSlash = 0;
}

// http://stackoverflow.com/questions/32185512/output-to-console-from-a-win32-gui-application-on-windows-10
class outbuf : public std::streambuf
{
public:

	outbuf()
	{
		setp(0, 0);
	}

	virtual int_type overflow(int_type c = traits_type::eof())
	{
		return fputc(c, stdout) == EOF ? traits_type::eof() : c;
	}
};

DLL_EXPORT void Win32CreateConsoleWin10()
{
	// create the console
	if (AllocConsole())
	{
		FILE* pCout;
		freopen_s(&pCout, "CONOUT$", "w", stdout);
		SetConsoleTitle("Debug Console");
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_RED);
	}

	// set std::cout to use my custom streambuf
	outbuf ob;
	std::streambuf *sb = std::cout.rdbuf(&ob);
}

DLL_EXPORT void Win32CreateConsole()
{
	Win32CreateConsoleWin10();

	//AllocConsole();

	//HANDLE handle_out = GetStdHandle(STD_OUTPUT_HANDLE);
	//int hCrt = _open_osfhandle((intptr_t)handle_out, _O_TEXT);
	//FILE* hf_out = _fdopen(hCrt, "w");
	//setvbuf(hf_out, NULL, _IONBF, 1);
	//*stdout = *hf_out;

	//HANDLE handle_in = GetStdHandle(STD_INPUT_HANDLE);
	//hCrt = _open_osfhandle((intptr_t)handle_in, _O_TEXT);
	//FILE* hf_in = _fdopen(hCrt, "r");
	//setvbuf(hf_in, NULL, _IONBF, 128);
	//*stdin = *hf_in;
}
