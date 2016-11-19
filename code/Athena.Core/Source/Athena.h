#ifndef __athena_h
#define __athena_h

#include "Array.h"
#include "Frame.h"
#include "List.h"
#include "Objects.h"
#include "Rendering.h"
#include "TypeDefs.h"
#include <thread>
#include "Vectors.h"

// TODO prerobit na Athena.Core
// TODO exportnut funkcie ktore vola Athena.Client

#define ATHENA_GPU_INITIALIZE(name)	void name(v2ui frameSize)
#define ATHENA_GPU_UPDATE(name)		void name(Objects& deviceObjects, const Objects& objects, const v3f* camera)
#define ATHENA_GPU_RENDER(name) \
	void name(const Objects& objects, const v2ui& regions, const v2ui& regionSize, const v2ui& pixelSize, Frame* frame)
#define ATHENA_GPU_CLEAN_UP(name)	void name(Objects& objects, Frame* frame)

typedef ATHENA_GPU_INITIALIZE(InitializeFunc);
typedef ATHENA_GPU_UPDATE(UpdateFunc);
typedef ATHENA_GPU_RENDER(RenderFunc);
typedef ATHENA_GPU_CLEAN_UP(CleanUpFunc);


struct AthenaRenderer
{
	HMODULE dllHandle;
	FILETIME dllLastWriteTime;

	String filename;
	String tempFilename;

	// TODO najst lepsie miesto pre objekty
	Objects objects;

	InitializeFunc* Initialize;
	UpdateFunc* Update;
	RenderFunc* Render;
	CleanUpFunc* CleanUp;
};

class Scene;
class Camera;
class UserInterface;	
struct Parameter;
struct Timer;

struct AthenaStorage
{
	Scene* scene;
	Camera* camera;
	UserInterface* userInterface;
	v2ui lastMousePosition;

	Frame frame;

	v2ui renderingRegions;
	v2ui renderingRegionSize;
		
	array_of<std::thread> threads;
	list_of<v2ui> pixelSizes;

	list_of<Parameter> debugParameters;
	array_of<Timer> timers;
	array_of<array_of<Timer>> timersLog;

	RenderingParameters renderingParameters;
	RenderingParameters previousRenderingParameters;

	AthenaRenderer renderer[Renderer::Count];
};

#define ATHENA_INITIALIZE(name)		AthenaStorage* name(v2ui outputSize, MemoryManager* memoryManagerInstance)
#define ATHENA_UPDATE_FRAME(name)	void name(AthenaStorage* storage, const application_input* input, f32 timeElapsed)
#define ATHENA_RENDER_FRAME(name)	void name(AthenaStorage* storage, array_of<uint32> output)
#define ATHENA_CLEAN_UP(name)		void name(AthenaStorage* storage, MemoryManager* memoryManagerInstance)

class MemoryManager;
struct application_input;

ATHENA_DLL_EXPORT ATHENA_INITIALIZE(AthenaInitialize);
ATHENA_DLL_EXPORT ATHENA_UPDATE_FRAME(AthenaUpdateFrame);
ATHENA_DLL_EXPORT ATHENA_RENDER_FRAME(AthenaRenderFrame);
ATHENA_DLL_EXPORT ATHENA_CLEAN_UP(AthenaCleanUp);

#endif __athena_h
