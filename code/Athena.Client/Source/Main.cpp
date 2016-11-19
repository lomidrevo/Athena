#include <Athena.h>
#include <conio.h>
#include <Debug.h>
#include <Log.h>
#include <MemoryManager.h>
#include <StringHelpers.h>
#include <Scene.h>
#include <strsafe.h>
#include <Objects.h>
#include <Timer.h>
#include <Timers.h>
#include <Win32.h>

// TODO prerobit tak, aby som vobec nemusel vediet o nazve renderera (dynamicky enum, ...)
#define ATHENA_RENDERER_DLL_MASK "Athena.Renderer.*.dll"
#define ATHENA_OPENCL_RENDERER_DLL_FILENAME "Athena.Renderer.OpenCl.dll"
#define ATHENA_CUDA_RENDERER_DLL_FILENAME "Athena.Renderer.Cuda.dll"

static bool32 running;
static bool32 paused;
static bool32 fpsLimit = true;
static win32_offscreen_buffer globalBackBuffer;

bool Win32LoadAthenaRenderer(const char* dllFilename, const char* tempDllFilename, AthenaRenderer& dll, 
	MemoryManager* memoryManagerInstance)
{
	if (!dllFilename || !tempDllFilename)
		return false;

	dll.dllLastWriteTime = Win32GetLastWriteTime(dllFilename);

	CopyFileA(dllFilename, tempDllFilename, false);
	dll.dllHandle = LoadLibraryA(tempDllFilename);
	if (!dll.dllHandle)
	{
		DWORD lastError = GetLastError();
		LOG_TL(LogLevel::Error, "LoadLibraryA returned error: %d", lastError);

		// TODO prerobit ziskanie stringu chybovej hlasky
		LPVOID lpMsgBuf;
		FormatMessage(
			FORMAT_MESSAGE_ALLOCATE_BUFFER |
			FORMAT_MESSAGE_FROM_SYSTEM |
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			lastError,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPTSTR)&lpMsgBuf,
			0, NULL);
		LPVOID lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT,
			(lstrlen((LPCTSTR)lpMsgBuf) + lstrlen((LPCTSTR)"Load") + 40) * sizeof(TCHAR));
		StringCchPrintf((LPTSTR)lpDisplayBuf,
			LocalSize(lpDisplayBuf) / sizeof(TCHAR),
			TEXT("%s failed with error %d: %s"),
			"Load", lastError, lpMsgBuf);
		MessageBox(NULL, (LPCTSTR)lpDisplayBuf, TEXT("Error"), MB_OK);

		return false;
	}

	dll.Initialize = (InitializeFunc*)GetProcAddress(dll.dllHandle, "Initialize");
	dll.Update = (UpdateFunc*)GetProcAddress(dll.dllHandle, "Update");
	dll.Render = (RenderFunc*)GetProcAddress(dll.dllHandle, "Render");
	dll.CleanUp = (CleanUpFunc*)GetProcAddress(dll.dllHandle, "CleanUp");

	if (dll.Initialize && dll.Update && dll.Render && dll.CleanUp)
	{
		dll.filename = _MEM_ALLOC_STRING(memoryManagerInstance, dllFilename);
		dll.tempFilename = _MEM_ALLOC_STRING(memoryManagerInstance, tempDllFilename);

		return true;
	}

	return false;
}

void Win32UnLoadAthenaRenderer(AthenaRenderer& dll, MemoryManager* memoryManagerInstance)
{
	if (dll.tempFilename.ptr && dll.dllHandle)
	{
		FreeLibrary(dll.dllHandle);
		DeleteFileA(dll.tempFilename.ptr);
	}

	dll.Initialize = NULL;
	dll.Update = NULL;
	dll.Render = NULL;
	dll.CleanUp = NULL;

	_MEM_FREE_STRING(memoryManagerInstance, &dll.filename);
	_MEM_FREE_STRING(memoryManagerInstance, &dll.tempFilename);
}

void Win32LoadAvailableAthenaRenderer(const char* directory, AthenaStorage* athenaStorage, 
	MemoryManager* memoryManagerInstance)
{
	HANDLE dir;
	WIN32_FIND_DATA file_data;

	char directorySearchPath[MAX_PATH] = {};
	strcpy(directorySearchPath, directory);
	strcat(directorySearchPath, ATHENA_RENDERER_DLL_MASK);

	char athenaRenderingDll[MAX_PATH] = {};
	char athenaRenderingDllTemp[MAX_PATH] = {};

	if ((dir = FindFirstFile(directorySearchPath, &file_data)) == INVALID_HANDLE_VALUE)
		return;

	do
	{
		const char* rendererFilename = file_data.cFileName;
		const bool is_directory = (file_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;

		//if (rendererFilename[0] == '.')
		//	continue;

		//if (is_directory)
		//	continue;

		Renderer::Enum rendererType = Renderer::CPU;
		if (!strcmp(rendererFilename, ATHENA_OPENCL_RENDERER_DLL_FILENAME))
			rendererType = Renderer::GPU_OpenCL;

		else if (!strcmp(rendererFilename, ATHENA_CUDA_RENDERER_DLL_FILENAME))
			rendererType = Renderer::GPU_CUDA;

		else
		{
			LOG_TL(LogLevel::Info, "Unknown renderer found: %s", athenaRenderingDll);
			continue;
		}

		strcpy(athenaRenderingDll, directory);
		strcat(athenaRenderingDll, rendererFilename);

		strcpy(athenaRenderingDllTemp, directory);
		strcat(athenaRenderingDllTemp, "_");
		strcat(athenaRenderingDllTemp, rendererFilename);

		if (Win32LoadAthenaRenderer(athenaRenderingDll, athenaRenderingDllTemp, 
			athenaStorage->renderer[rendererType], memoryManagerInstance))
		{
			athenaStorage->renderer[rendererType].Initialize(athenaStorage->frame.size);
			LOG_TL(LogLevel::Info, "Renderer %s loaded", Renderer::GetString(rendererType));
		}
	}
	while (FindNextFile(dir, &file_data));

	FindClose(dir);
}

int AtExit(void)
{
	if (globalBackBuffer.memory.ptr)
		_MEM_FREE_ARRAY(MEM_MANAGER_INSTANCE, uint32, &globalBackBuffer.memory);

	MemoryManager::Destroy();
	Log::Destroy();

	auto keyPressed = _getch();

	return 0;
}

LRESULT CALLBACK Win32MainWindowCallback(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
	LRESULT result = 0;

	switch (message)
	{
		case WM_SIZE:
		{
		} break;

		case WM_CLOSE:
		{
			running = false;
		} break;

		case WM_ACTIVATEAPP:
		{

		} break;

		case WM_DESTROY:
		{
			running = false;
		} break;

		case WM_PAINT:
		{
			PAINTSTRUCT paint;
			HDC deviceContext = BeginPaint(window, &paint);
			int x = paint.rcPaint.left;
			int y = paint.rcPaint.top;
			int width = paint.rcPaint.right - paint.rcPaint.left;
			int height = paint.rcPaint.bottom - paint.rcPaint.top;

			vector2i dimension;
			Win32GetWindowDimension(window, dimension);
			Win32DisplayBufferInWindow(&globalBackBuffer, deviceContext, dimension.x, dimension.y);

			EndPaint(window, &paint);
		} break;

		default:
		{
			result = DefWindowProc(window, message, wParam, lParam);
		} break;
	};

	return result;
}

void Win32ProcessPendingMessages(win32_state* state, controller_input* keyboardController)
{
	MSG message;
	while (PeekMessage(&message, 0, 0, 0, PM_REMOVE))
	{
		switch (message.message)
		{
			case WM_QUIT:
			{
				running = false;
			} break;

			case WM_SYSKEYUP:
			case WM_SYSKEYDOWN:
			case WM_KEYUP:
			case WM_KEYDOWN:
			{
				uint32 vkCode = (uint32)message.wParam;
				bool32 wasDown = (message.lParam & (1 << 30)) != 0;
				bool32 isDown = (message.lParam & (1 << 31)) == 0;
				bool32 altKeyWasDown = (message.lParam & (1 << 29)) != 0;

				if (wasDown != isDown)
				{
					if (vkCode == 'W')
					{
						Win32ProcessKeyboardStateMessage(&keyboardController->moveUp, isDown);
					}
					else if (vkCode == 'A')
					{
						Win32ProcessKeyboardStateMessage(&keyboardController->moveLeft, isDown);
					}
					else if (vkCode == 'S')
					{
						Win32ProcessKeyboardStateMessage(&keyboardController->moveDown, isDown);
					}
					else if (vkCode == 'D')
					{
						Win32ProcessKeyboardStateMessage(&keyboardController->moveRight, isDown);
					}
					else if (vkCode == VK_UP)
					{
						Win32ProcessKeyboardStateMessage(&keyboardController->actionUp, isDown);
					}
					else if (vkCode == VK_LEFT)
					{
						Win32ProcessKeyboardStateMessage(&keyboardController->actionLeft, isDown);
					}
					else if (vkCode == VK_DOWN)
					{
						Win32ProcessKeyboardStateMessage(&keyboardController->actionDown, isDown);
					}
					else if (vkCode == VK_RIGHT)
					{
						Win32ProcessKeyboardStateMessage(&keyboardController->actionRight, isDown);
					}

					else if (vkCode == VK_ESCAPE || (vkCode == VK_F4 && altKeyWasDown))
					{
						running = false;
					}

					else if (vkCode == 'P' && isDown)
					{
						paused = !paused;
					}

					else if (vkCode == 'F' && isDown)
					{
						fpsLimit = !fpsLimit;
					}

					// input recording & playback
					//else if (vkCode == VK_F5 && altKeyWasDown)
					//{
					//	if (isDown)
					//	{
					//		if (state->inputRecordingIndex == 0)
					//			Win32BeginRecordingInput(state, 1);
					//		else
					//		{
					//			Win32EndRecordingInput(state);
					//			Win32BeginInputPlayback(state, 1);
					//		}
					//	}
					//}
					//else if (vkCode == VK_F5)
					//{
					//	if (isDown)
					//	{
					//		if (state->inputRecordingIndex)
					//			Win32EndRecordingInput(state);

					//		else if (state->inputPlaybackIndex)
					//			Win32EndInputPlayback(state);
					//	}
					//}
					else if (vkCode == VK_RETURN && altKeyWasDown)
					{
						// TODO suradnice cursora su vo fullscreene nejak natiahnute

						if (isDown)
							Win32ToggleFullscreen(message.hwnd);
					}
					else if (vkCode == VK_F1 && isDown)
					{
						keyboardController->menu.isDown = !keyboardController->menu.isDown;
					}
					//else if (vkCode == VK_F1)
					//{
					//	Win32ProcessKeyboardStateMessage(&keyboardController->menu, isDown);
					//}
					//else if (vkCode >= VK_F1 && vkCode <= VK_F12)
					//{
					//	keyboardController->fKeys[vkCode - VK_F1] = isDown;
					//}
				}

			} break;

			default:
			{
				TranslateMessage(&message);
				DispatchMessageA(&message);
			} break;
		}
	}
}

ui64 hash(ui64 x)
{
	x = (x ^ (x >> 30)) * UINT64_C(0xbf58476d1ce4e5b9);
	x = (x ^ (x >> 27)) * UINT64_C(0x94d049bb133111eb);
	x = (x ^ (x >> 31));

	return x;
}

// returns random number <0-1>
f64 GetRandom64(ui64* seed0, ui64* seed1, ui64* seed2, ui64* seed3)
{
	*seed0 = hash(*seed0);
	*seed1 = hash(*seed1);
	*seed2 = hash(*seed2);
	*seed3 = hash(*seed3);

	ui64 ires =
		(*seed0 & 0x000000000000ffff) +
		(*seed1 & 0x00000000ffff0000) +
		(*seed2 & 0x0000ffff00000000) +
		(*seed3 & 0xffff000000000000);

	return (f64)ires / (f64)0xffffffffffffffff;
}

int CALLBACK WinMain(
	_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPSTR lpCmdLine,
	_In_ int nShowCmd
	)
{
	//ui64 x = 1;
	//ui64 y = 2;
	//ui64 frame = 3;
	//ui64 seed = (ui64)time(null);

	//f64 random0 = GetRandom64(&x, &y, &frame, &seed);
	//f64 random1 = GetRandom64(&x, &y, &frame, &seed);
	//f64 random2 = GetRandom64(&x, &y, &frame, &seed);
	//f64 random3 = GetRandom64(&x, &y, &frame, &seed);
	//f64 random4 = GetRandom64(&x, &y, &frame, &seed);
	//f64 random5 = GetRandom64(&x, &y, &frame, &seed);
	//f64 random6 = GetRandom64(&x, &y, &frame, &seed);
	//f64 random7 = GetRandom64(&x, &y, &frame, &seed);
	
	Win32CreateConsole();

	Log::Instance()->Initialize("Athena", true);
	Log::Instance()->SetLogLevel(LogLevel::DebugMem);
	MemoryManager::Instance()->Initialize(GIGABYTES(1));
	
	//if (!QueryPerformanceFrequency((LARGE_INTEGER*)&Timer::freq))
	//	LOG_L(LogLevel::Error, "QueryPerformanceFrequency() failed");

	// get path to executable directory
	char executableDirectory[MAX_PATH];
	Win32GetExecutableDirectory(executableDirectory);

	// file I/O test
	//win32_read_file_result file = Win32ReadFile(__FILE__);
	//Win32WriteFile("output.txt", file.memorySize, file.memory);
	//Win32FreeFileMemory(&file);

	// set windows scheduler granularity to 1ms, so Sleep() cam be more accurate
	MMRESULT timePeriodResult = timeBeginPeriod(1);
	bool32 sleepIsGranular = timePeriodResult == TIMERR_NOERROR;
	if (!sleepIsGranular)
		LOG_TL(LogLevel::Warning, "timeBeginPeriod(1) returned %d", timePeriodResult)

	Win32CreateOffscreenBuffer(&globalBackBuffer, 1280, 720);
	if (!globalBackBuffer.memory.ptr)
		globalBackBuffer.memory = MEM_ALLOC_ARRAY(uint32, 
		globalBackBuffer.bytesPerPixel * globalBackBuffer.width * globalBackBuffer.height);

	WNDCLASS windowClass = {};
	windowClass.style = CS_HREDRAW | CS_VREDRAW;
	windowClass.lpfnWndProc = Win32MainWindowCallback;
	windowClass.hInstance = hInstance;
	windowClass.lpszClassName = "Athena";
	windowClass.hCursor = LoadCursor(0, IDC_CROSS);

	if (RegisterClassA(&windowClass))
	{
		// TODO WIN32_WINDOW_WIDTH_BORDER a WIN32_WINDOW_HEIGHT_BORDER nesmu byt konstany, ziskat inak
		// do rozmerov okna treba pri vytvarani pridat hlavicku a okraje
		HWND window = CreateWindowExA(
			0,
			windowClass.lpszClassName, "Athena",
			WS_VISIBLE | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX/* | WS_MAXIMIZEBOX*/,
			CW_USEDEFAULT, CW_USEDEFAULT,
			globalBackBuffer.width + WIN32_WINDOW_WIDTH_BORDER, globalBackBuffer.height + WIN32_WINDOW_HEIGHT_BORDER,
			0, 0, hInstance, 0);

		if (window)
		{
			// get monitor refresh rate and compute target number of seconds per frame
			int monitorRefreshRate = 60;
			HDC deviceContext = GetDC(window);
			int win32MonitorRefreshRate = GetDeviceCaps(deviceContext, VREFRESH);
			ReleaseDC(window, deviceContext);

			if (win32MonitorRefreshRate > 1)
				monitorRefreshRate = win32MonitorRefreshRate;
			else
				LOG_TL(LogLevel::Warning, "GetDeviceCaps(VREFRESH) returned %d!", win32MonitorRefreshRate);

			real32 targetSecondsPerFrame = (real32)(1 / (real32)monitorRefreshRate);

			application_input input[2] = {};
			application_input* newInput = &input[0];
			application_input* oldInput = &input[1];
			controller_input zeroController = {};

			win32_state state = {};

			// initialize athena
			AthenaStorage* athenaStorage = 
				AthenaInitialize(v2ui(globalBackBuffer.width, globalBackBuffer.height), MEM_MANAGER_INSTANCE);

			Win32LoadAvailableAthenaRenderer(executableDirectory, athenaStorage, MEM_MANAGER_INSTANCE);

			v3i mousePosition;
			DEBUG_PARAMETER_READONLY(MEM_MANAGER_INSTANCE, athenaStorage, "mousePosition", Type::v3i, 
				(const v3i*)&mousePosition, 0);

			ui32 fps = 0;
			DEBUG_PARAMETER_READONLY(MEM_MANAGER_INSTANCE, athenaStorage, "fps", Type::ui32, (const ui32*)&fps, 0);

			BEGIN_TIMED_BLOCK(athenaStorage->timers[TimerId::Application]);

			running = true;
			while (running)
			{
				TIMED_BLOCK(&athenaStorage->timers[TimerId::Frame]);

				BEGIN_TIMED_BLOCK(athenaStorage->timers[TimerId::SystemProcessing]);

				// check if dll was changed and if so, reload
				//FILETIME actualDllLastWriteTime = Win32GetLastWriteTime(athenaDllFilePath);
				//if (CompareFileTime(&actualDllLastWriteTime, &athena.dllLastWriteTime) != 0)
				//{
				//	Win32UnLoadDll(&athena, athenaTempDllFilePath);
				//	athena = Win32LoadAthenaDll(athenaDllFilePath, athenaTempDllFilePath);
				//}

				const controller_input* oldKeyboardController = oldInput->controllers;
				controller_input *newKeyboardController = newInput->controllers;
				*newKeyboardController = zeroController;
				newKeyboardController->isConnected = true;

				for (int i = 0; i < NUM_OF_CONTROLLER_BUTTONS; ++i)
					newKeyboardController->buttons[i].isDown = oldKeyboardController->buttons[i].isDown;

				// process windows messages
				Win32ProcessPendingMessages(&state, newKeyboardController);

				// if paused, skip update and rendering, just basic message processing
				//if (paused)
				//	continue;

				// get mouse position
				POINT tmpMousePosition;
				GetCursorPos(&tmpMousePosition);
				ScreenToClient(window, &tmpMousePosition);
				mousePosition.Set(tmpMousePosition.x, tmpMousePosition.y, 0 /*TODO mouse wheel position*/);
				newInput->mousePosition = mousePosition;
				
				// get mouse buttons
				Win32ProcessMouseButtonStateMessage(&newInput->mouseButtons[MouseButton::Left], 
					GetKeyState(VK_LBUTTON) & (1 << 15));
				Win32ProcessMouseButtonStateMessage(&newInput->mouseButtons[MouseButton::Middle], 
					GetKeyState(VK_MBUTTON) & (1 << 15));
				Win32ProcessMouseButtonStateMessage(&newInput->mouseButtons[MouseButton::Right], 
					GetKeyState(VK_RBUTTON) & (1 << 15));
				Win32ProcessMouseButtonStateMessage(&newInput->mouseButtons[3], GetKeyState(VK_XBUTTON1) & (1 << 15));
				Win32ProcessMouseButtonStateMessage(&newInput->mouseButtons[4], GetKeyState(VK_XBUTTON2) & (1 << 15));

				for (int i = 0; i < NUM_OF_MOUSE_BUTTONS; i++)
				{
					newInput->mouseButtons[i].pressed =
						(newInput->mouseButtons[i].isDown && !oldInput->mouseButtons[i].isDown);
					newInput->mouseButtons[i].released =
						(!newInput->mouseButtons[i].isDown && oldInput->mouseButtons[i].isDown);
				}

				// hide cursor if right mouse button is pressed
				//if (newInput->mouseButtons[MouseButton::Right].pressed)
				//	ShowCursor(false);
				//else if (newInput->mouseButtons[MouseButton::Right].released)
				//	ShowCursor(true);

				END_TIMED_BLOCK(athenaStorage->timers[TimerId::SystemProcessing]);

				// frame update before rendering
				AthenaUpdateFrame(athenaStorage, newInput, athenaStorage->timers[TimerId::Frame].lastDurationSec);

				// render frame
				AthenaRenderFrame(athenaStorage, globalBackBuffer.memory);
			
				if (fpsLimit)
				{
				//	// sleep for the rest of the desired frame time
				//	LARGE_INTEGER workCounter = Win32GetCounter();
				//	real32 workSecondsElapsed = Win32GetSecondsElapsed(lastCounter, workCounter);
				//	real32 secondsElapsedForFrame = workSecondsElapsed;
				//	if (secondsElapsedForFrame < targetSecondsPerFrame)
				//	{
				//		if (sleepIsGranular)
				//		{
				//			DWORD sleep = (DWORD)(1000 * (targetSecondsPerFrame - secondsElapsedForFrame));
				//			if (sleep > 0)
				//				Sleep(sleep);
				//		}
				//		while (secondsElapsedForFrame < targetSecondsPerFrame)
				//			secondsElapsedForFrame = Win32GetSecondsElapsed(lastCounter, Win32GetCounter());
				//	}
				//	else
				//	{
				//		//LOG_TL(LogLevel::Debug, "Desired framerate missed [#%d, %.03fms > %.03fms]!",
				//		//	frameCounter,
				//		//	secondsElapsedForFrame * 1000,
				//		//	targetSecondsPerFrame * 1000);
				//	}
				}

				// redraw window
				BEGIN_TIMED_BLOCK(athenaStorage->timers[TimerId::Display]);
				vector2i dimension;
				Win32GetWindowDimension(window, dimension);
				HDC deviceContext = GetDC(window);
				Win32DisplayBufferInWindow(&globalBackBuffer, deviceContext, dimension.x, dimension.y);
				ReleaseDC(window, deviceContext);
				END_TIMED_BLOCK(athenaStorage->timers[TimerId::Display]);

				SWAP(oldInput, newInput, application_input);

				fps = (ui32)((f32)1 / athenaStorage->timers[TimerId::Frame].lastDurationSec);

				const ui32 logId = (ui32)(athenaStorage->frame.count % athenaStorage->timersLog.count);
				memcpy(athenaStorage->timersLog[logId].ptr, athenaStorage->timers.ptr, TimerId::Count * sizeof(Timer));

				athenaStorage->frame.count++;
			}

			END_TIMED_BLOCK(athenaStorage->timers[TimerId::Application]);

			// runtime stats
			const real32 runTimeInSeconds = athenaStorage->timers[TimerId::Application].lastDurationSec;
			LOG_TL(LogLevel::Info, "%I64d frames rendered in %.0f seconds (~%.0ffps)",
				athenaStorage->frame.count, runTimeInSeconds, (real32)athenaStorage->frame.count / runTimeInSeconds);

			for (int i = 0; i < Renderer::Count; ++i)
			{
				if (i != Renderer::CPU && athenaStorage->renderer[i].dllHandle)
				{
					athenaStorage->renderer[i].CleanUp(athenaStorage->renderer[i].objects, &athenaStorage->frame);
					Win32UnLoadAthenaRenderer(athenaStorage->renderer[i], MEM_MANAGER_INSTANCE);

					LOG_TL(LogLevel::Info, "Renderer %s unloaded", Renderer::GetString((Renderer::Enum)i));
				}
			}

			AthenaCleanUp(athenaStorage, MEM_MANAGER_INSTANCE);
		}
		else
		{
			LOG_TL(LogLevel::Error, "CreateWindowExA failed!");
		}
	}
	else
	{
		LOG_TL(LogLevel::Error, "RegisterClass failed!");
	}

	return AtExit();
}
