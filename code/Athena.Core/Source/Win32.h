#ifndef __win32_h
#define __win32_h

#include <Array.h>
#include <Input.h>
#include <TypeDefs.h>
#include <Vectors.h>
#include <Windows.h>

// konstantna velkost okrajov a hlavicky pre okno
#define WIN32_WINDOW_WIDTH_BORDER	16
#define WIN32_WINDOW_HEIGHT_BORDER	39


struct win32_offscreen_buffer
{
	BITMAPINFO info;
	array_of<ui32> memory;

	int width;
	int height;
	int pitch;
	int bytesPerPixel;
};

struct win32_read_file_result
{
	void* memory;
	uint32 memorySize;
};

struct win32_state
{
	HANDLE recordingHandle;
	HANDLE playbackHandle;

	int inputRecordingIndex;
	int inputPlaybackIndex;
};

DLL_EXPORT void Win32ToggleFullscreen(HWND window);
DLL_EXPORT void Win32FreeFileMemory(win32_read_file_result* file);
DLL_EXPORT bool32 Win32WriteFile(const char* filename, uint32 memorySize, void* memory);
DLL_EXPORT win32_read_file_result Win32ReadFile(const char* filename);
DLL_EXPORT void Win32GetWindowDimension(HWND window, vector2i& windowDimension);
DLL_EXPORT void Win32CreateOffscreenBuffer(win32_offscreen_buffer* buffer, int width, int height);
DLL_EXPORT void Win32DisplayBufferInWindow(win32_offscreen_buffer* buffer, HDC deviceContext, int windowWidth, 
	int windowHeight);
//void Win32BeginRecordingInput(win32_state* state, int inputRecordingIndex)
//void Win32EndRecordingInput(win32_state* state);
//void Win32BeginInputPlayback(win32_state* state, int inputPlaybackIndex);
//void Win32EndInputPlayback(win32_state* state);
//void Win32RecordInput(win32_state* state, application_input* input);
//void Win32PlaybackInput(win32_state* state, application_input* input);
DLL_EXPORT void Win32ProcessMouseButtonStateMessage(mouse_button_state* newState, bool16 isDown);
DLL_EXPORT void Win32ProcessKeyboardStateMessage(button_state* newState, bool32 isDown);
DLL_EXPORT FILETIME Win32GetLastWriteTime(const char* filename);
DLL_EXPORT void Win32GetExecutableDirectory(char* directoryBuffer);
DLL_EXPORT void Win32CreateConsole();
DLL_EXPORT void Win32CreateConsoleWin10();

#endif __win32_h
