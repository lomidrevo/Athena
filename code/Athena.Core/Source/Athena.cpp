#include "Athena.h"
#include "Animations.h"
#include "Camera.h"
#include "Convert.h"
#include "Debug.h"
#include "Input.h"
#include "MemoryManager.h"
#include "Random.h"
#include "Rendering.h"
#include "Scene.h"
#include "UserInterface.h"
#include "Timer.h"
#include "Timers.h"
#include "ZOrder.h"


void GenerateScene(MemoryManager* memoryManagerInstance, Scene* scene);
void GenerateLandscape(Scene* scene, int materialCount);
void GenerateEverything(Scene* scene, uint complexity, int materialCount);
void GenerateMesh(Scene* scene, MemoryManager* memoryManagerInstance);
void GenerateGrid(Scene* scene, const v3f& position, const v3ui& count, const v3f& size, const v3f& spacing,
	int materialCount);

void CreateUserInteface(MemoryManager* memoryManagerInstance, AthenaStorage* storage);
void OnGuiButtonClicked(GUI_BUTTON_CLICKED_PARAMETERS);
void OnSceneButtonClicked(SCENE_CLICKED_PARAMETERS);
const ObjectId* GetObjectIdAt(v2ui point, const Frame& frame);
void ProcessInput(AthenaStorage* storage, const application_input* input, real32 timeElapsed);
void RenderOnThread(AthenaStorage* storage, uint32 threadId = 0);
void PostProcessOutput(AthenaStorage* storage, array_of<uint32> output);


ATHENA_DLL_EXPORT ATHENA_INITIALIZE(AthenaInitialize)
{
	AthenaStorage* storage = _MEM_ALLOC(memoryManagerInstance, AthenaStorage);
	storage->camera = _MEM_ALLOC(memoryManagerInstance, Camera);
	storage->scene = _MEM_ALLOC(memoryManagerInstance, Scene);

	// debug parameters collection
	storage->debugParameters.Initialize(memoryManagerInstance);
	DEBUG_REGION(memoryManagerInstance, storage, "root");

	// timers
	storage->timers = _MEM_ALLOC_ARRAY(memoryManagerInstance, Timer, TimerId::Count);

	// timers log
	storage->timersLog = _MEM_ALLOC_ARRAY(memoryManagerInstance, array_of<Timer>, 128);
	for (uint i = 0; i < storage->timersLog.count; i++)
		storage->timersLog[i] = _MEM_ALLOC_ARRAY(memoryManagerInstance, Timer, TimerId::Count);

	// add all defined timers to debug parameters
	auto timersRegionId = DEBUG_REGION(memoryManagerInstance, storage, "Timers");
	for (int i = 0; i < TimerId::Count; i++)
		DEBUG_PARAMETER_READONLY(memoryManagerInstance, storage, TimerId::GetString((TimerId::Enum)i), Type::f32,
			(const f32*)&storage->timers[i].lastDurationMs, timersRegionId);

	// create frame buffers
	storage->frame.size = outputSize;
	const uint32 frameBufferLength = storage->frame.size.x * storage->frame.size.y;
	for (int i = 0; i < FrameBuffer::Count; ++i)
		storage->frame.buffer[i] = _MEM_ALLOC_ARRAY(memoryManagerInstance, v4b, frameBufferLength);
	storage->frame.objectIdBuffer = _MEM_ALLOC_ARRAY(memoryManagerInstance, ObjectId, frameBufferLength);
	
	// color buffer for accumulative rendering
	storage->frame.colorAccBuffer = _MEM_ALLOC_ARRAY(memoryManagerInstance, v3f, frameBufferLength);

	// initialize frame count debug parameters
	storage->frame.count = storage->frame.countSinceChange = 0;

	auto frameRegionId = DEBUG_REGION(memoryManagerInstance, storage, "Frame");
	DEBUG_PARAMETER_READONLY(memoryManagerInstance, storage, "count", Type::ui64, (const ui64*)&storage->frame.count, 
		frameRegionId);
	DEBUG_PARAMETER_READONLY(memoryManagerInstance, storage, "countSinceChange", Type::ui64, 
		(const ui64*)&storage->frame.countSinceChange, frameRegionId);
	storage->frame.countSinceChangeMax = 64;
	DEBUG_PARAMETER(memoryManagerInstance, storage, "countSinceChangeMax", Type::ui64, 
		(ui64*)&storage->frame.countSinceChangeMax, null, frameRegionId);
	storage->frame.current = FrameBuffer::Color;
	DEBUG_PARAMETER(memoryManagerInstance, storage, "current", Type::frameBufferEnum, &storage->frame.current, null, 
		frameRegionId);

	// create rendering regions
	// 8x8 = 160x90
	// 16x16 = 80x45
	storage->renderingRegions.Set(80, 45);
	storage->renderingRegionSize = storage->frame.size / storage->renderingRegions;
	
	storage->pixelSizes.Initialize(memoryManagerInstance, "v2ui", 8);
	storage->pixelSizes.Add(v2ui(1, 1));
	storage->pixelSizes.Add(v2ui(2, 2));
	storage->pixelSizes.Add(v2ui(4, 4));
	storage->pixelSizes.Add(v2ui(8, 8));
	storage->pixelSizes.Add(v2ui(16, 16));
	
	// threads
	storage->threads = _MEM_ALLOC_ARRAY(memoryManagerInstance, std::thread, 16);

	// set camera
	storage->camera->Set(v3f(0, 0, -5000), v3f(0, 0, 0));

	// create scene
	storage->scene->Create("test", memoryManagerInstance);
	GenerateScene(memoryManagerInstance, storage->scene);

	// add camera properties to debug parameters
	auto cameraRegionId = DEBUG_REGION(memoryManagerInstance, storage, "Camera");
	for (int i = 0; i < CameraParameter::Count; i++)
		DEBUG_PARAMETER_READONLY(memoryManagerInstance, storage, CameraParameter::GetString((CameraParameter::Enum)i), 
		Type::v3f, (const v3f*)&storage->camera->	GetParameters()[i], cameraRegionId);

	auto renderingParametersRegionId = DEBUG_REGION(memoryManagerInstance, storage, "Rendering");

	// default values for rendering parameters
	storage->renderingParameters.softwareRenderingThreadsCount = 4;
	storage->renderingParameters.currentPixelSizeId = 2;
	storage->renderingParameters.maxBihDepth = 8;
	storage->renderingParameters.maxBihLeafObjects = 4;
	storage->renderingParameters.ambientOcclusionSamples = 0;
	storage->renderingParameters.ambientOcclusionModifier = .4;
	storage->renderingParameters.maxRayTracingDepth = 4;
	storage->renderingParameters.maxOctreeDepth = 0;
	storage->renderingParameters.multiThreadedOctreeUpdate = false;
	storage->renderingParameters.renderingMode = RenderingMode::Progressive;
	storage->renderingParameters.renderingMethod = RenderingMethod::RayTracing;
	storage->renderingParameters.tracingMethod = TracingMethod::BoundingIntervalHierarchy;
	storage->renderingParameters.currentRenderer = Renderer::GPU_OpenCL;

#ifdef DEBUG
	storage->renderingParameters.softwareRenderingThreadsCount = 1;
#endif

	DEBUG_PARAMETER(memoryManagerInstance, storage, "softwareRenderingThreadsCount", Type::ui32,
		&storage->renderingParameters.softwareRenderingThreadsCount, &storage->threads.count, renderingParametersRegionId);
	DEBUG_PARAMETER(memoryManagerInstance, storage, "currentPixelSizeId", Type::ui32,
		&storage->renderingParameters.currentPixelSizeId, &storage->pixelSizes.currentCount, renderingParametersRegionId);
	DEBUG_PARAMETER(memoryManagerInstance, storage, "maxBihDepth", Type::ui32,
		&storage->renderingParameters.maxBihDepth, null, renderingParametersRegionId);
	DEBUG_PARAMETER(memoryManagerInstance, storage, "maxBihLeafObjects", Type::ui32,
		&storage->renderingParameters.maxBihLeafObjects, null, renderingParametersRegionId);
	DEBUG_PARAMETER(memoryManagerInstance, storage, "ambientOcclusionSamples", Type::ui32,
		&storage->renderingParameters.ambientOcclusionSamples, null, renderingParametersRegionId);
	DEBUG_PARAMETER(memoryManagerInstance, storage, "ambientOcclusionModifier", Type::real,
		&storage->renderingParameters.ambientOcclusionModifier, null, renderingParametersRegionId);
	DEBUG_PARAMETER(memoryManagerInstance, storage, "maxRayTracingDepth", Type::ui32,
		&storage->renderingParameters.maxRayTracingDepth, null, renderingParametersRegionId);
	DEBUG_PARAMETER(memoryManagerInstance, storage, "maxOctreeDepth", Type::ui32,
		&storage->renderingParameters.maxOctreeDepth, null, renderingParametersRegionId);
	DEBUG_PARAMETER(memoryManagerInstance, storage, "multiThreadedOctreeUpdate", Type::b32,
		&storage->renderingParameters.multiThreadedOctreeUpdate, null, renderingParametersRegionId);
	DEBUG_PARAMETER(memoryManagerInstance, storage, "renderingMode", Type::renderingModeEnum,
		&storage->renderingParameters.renderingMode, null, renderingParametersRegionId);
	DEBUG_PARAMETER(memoryManagerInstance, storage, "renderingMethod", Type::renderingMethodEnum,
		&storage->renderingParameters.renderingMethod, null, renderingParametersRegionId);
	DEBUG_PARAMETER(memoryManagerInstance, storage, "tracingMethod", Type::tracingMethodEnum,
		&storage->renderingParameters.tracingMethod, null, renderingParametersRegionId);
	DEBUG_PARAMETER(memoryManagerInstance, storage, "renderer", Type::rendererEnum, 
		&storage->renderingParameters.currentRenderer, null, renderingParametersRegionId);

	// initialize user interface
	storage->userInterface = _MEM_ALLOC(memoryManagerInstance, UserInterface);
	storage->userInterface->Initialize(memoryManagerInstance, storage);
	//CreateUserInteface(memoryManagerInstance, storage);
	//storage->userInterface->sceneClickedFunction = OnSceneButtonClicked;

	LOG_TL(LogLevel::Info, "Athena::Initialize [window: %dx%d; regions: %dx%d; renderingThreads: %d]",
		storage->frame.size.x, storage->frame.size.y,
		storage->renderingRegions.x, storage->renderingRegions.y,
		storage->renderingParameters.softwareRenderingThreadsCount);
	
	return storage;
}

ATHENA_DLL_EXPORT ATHENA_CLEAN_UP(AthenaCleanUp)
{
	if (storage->scene)
	{
		storage->scene->Destroy();
		_MEM_FREE(memoryManagerInstance, storage->scene);
	}

	_MEM_FREE(memoryManagerInstance, storage->camera);

	for (int i = 0; i < FrameBuffer::Count; ++i)
		_MEM_FREE_ARRAY(memoryManagerInstance, v4b, &storage->frame.buffer[i]);
	_MEM_FREE_ARRAY(memoryManagerInstance, ObjectId, &storage->frame.objectIdBuffer);
	_MEM_FREE_ARRAY(memoryManagerInstance, v3f, &storage->frame.colorAccBuffer);

	_MEM_FREE_ARRAY(memoryManagerInstance, std::thread, &storage->threads);
	storage->pixelSizes.Destroy();

	if (storage->userInterface)
	{
		storage->userInterface->Destroy(memoryManagerInstance);
		_MEM_FREE(memoryManagerInstance, storage->userInterface);
	}

	for (uint i = 0; i < storage->debugParameters.currentCount; ++i)
		_MEM_FREE_STRING(memoryManagerInstance, &storage->debugParameters.array[i].name);
	storage->debugParameters.Destroy();
	
	_MEM_FREE_ARRAY(memoryManagerInstance, Timer, &storage->timers);
	
	for (uint i = 0; i < storage->timersLog.count; i++)
		_MEM_FREE_ARRAY(memoryManagerInstance, Timer, &storage->timersLog.ptr[i]);
	_MEM_FREE_ARRAY(memoryManagerInstance, array_of<Timer>, &storage->timersLog);

	_MEM_FREE(memoryManagerInstance, storage);
}

ATHENA_DLL_EXPORT ATHENA_UPDATE_FRAME(AthenaUpdateFrame)
{
	TIMED_BLOCK(&storage->timers[TimerId::Update]);

	ProcessInput(storage, input, timeElapsed);

	const RenderingParameters& params = storage->renderingParameters;

	// update camera
	bool viewChanged = storage->camera->Update(storage->frame.size);

	// update scene
	viewChanged |= storage->scene->Update(timeElapsed, storage,
		params.maxOctreeDepth, /* max octree depth */
		params.maxBihDepth, params.maxBihLeafObjects /* bih parameters */
		);

	// rendering parameters changed
	viewChanged |=
		memcmp(&params, &storage->previousRenderingParameters, sizeof(RenderingParameters)) != 0;

	if (viewChanged)
	{
		storage->frame.countSinceChange = 0;
		memset(storage->frame.colorAccBuffer.ptr, 0, sizeof(v3f) * storage->frame.colorAccBuffer.count);
		memcpy(&storage->previousRenderingParameters, &params, sizeof(RenderingParameters));
	}
	
	if (params.renderingMode == RenderingMode::Continuous ||
		storage->frame.countSinceChange < storage->frame.countSinceChangeMax)
		storage->frame.countSinceChange++;

	if (params.currentRenderer != Renderer::CPU && storage->renderer[params.currentRenderer].dllHandle)
		storage->renderer[params.currentRenderer].Update(
		storage->renderer[params.currentRenderer].objects, storage->scene->GetObjects(),
		storage->camera->GetParameters());
}

ATHENA_DLL_EXPORT ATHENA_RENDER_FRAME(AthenaRenderFrame)
{
	TIMED_BLOCK(&storage->timers[TimerId::Draw]);
	
	BEGIN_TIMED_BLOCK(storage->timers[TimerId::Render]);

	const RenderingMode::Enum mode = storage->renderingParameters.renderingMode;
	if ((mode == RenderingMode::Continuous) ||
		(mode == RenderingMode::SingleImage && storage->frame.countSinceChange == 1) ||
		(mode == RenderingMode::Progressive && storage->frame.countSinceChange < storage->frame.countSinceChangeMax))
	{
		if (storage->renderingParameters.currentRenderer == Renderer::CPU)
		{
			if (storage->renderingParameters.softwareRenderingThreadsCount > 1)
			{
				const ui32 threadsCount = storage->renderingParameters.softwareRenderingThreadsCount;
				for (ui32 threadId = 0; threadId < threadsCount; ++threadId)
					storage->threads[threadId] = std::thread(RenderOnThread, storage, threadId);
				for (ui32 threadId = 0; threadId < threadsCount; ++threadId)
					storage->threads[threadId].join();
			}
			else
				RenderOnThread(storage);
		}
		else
		{
			if (storage->renderer[storage->renderingParameters.currentRenderer].dllHandle)
			{
				// NOTE (CUDA) 1024 threads pre block is max for GeForce GT 555M/760

				storage->renderer[storage->renderingParameters.currentRenderer].Render(
					storage->renderer[storage->renderingParameters.currentRenderer].objects,
					storage->renderingRegions,
					storage->renderingRegionSize / storage->pixelSizes[storage->renderingParameters.currentPixelSizeId],
					storage->pixelSizes[storage->renderingParameters.currentPixelSizeId],
					&storage->frame);
			}
		}
	}
	END_TIMED_BLOCK(storage->timers[TimerId::Render]);

	PostProcessOutput(storage, output);
	storage->userInterface->Draw(storage, output);
}

void CreateUserInteface(MemoryManager* memoryManagerInstance, AthenaStorage* storage)
{
	// TODO velkost buttonu na zaklade textu a najdlhsieho buttony v ramci jednej urovne
	v2ui elementSize = v2ui(150, 32);

	array_of<GuiElement> contextMenuItems;

	// context menu
	contextMenuItems = _MEM_ALLOC_ARRAY(memoryManagerInstance, GuiElement, 4);

	// first level context menu items
	contextMenuItems[0].Set(GuiElementType::GuiMenuItem, elementSize, GuiElementId::CurrentFrameBufferMenuItem);
	contextMenuItems[0].text = _MEM_ALLOC_STRING(memoryManagerInstance, "Output");

	// frame buffer type buttons
	contextMenuItems[0].childrenItems = _MEM_ALLOC_ARRAY(memoryManagerInstance, GuiElement, 4);
	contextMenuItems[0].childrenItems[0].Set(GuiElementType::GuiButton, elementSize, GuiElementId::ColorFrameBufferBtn);
	contextMenuItems[0].childrenItems[0].text = _MEM_ALLOC_STRING(memoryManagerInstance,
		FrameBuffer::GetString(FrameBuffer::Color));
	contextMenuItems[0].childrenItems[1].Set(GuiElementType::GuiButton, elementSize, GuiElementId::NormalFrameBufferBtn);
	contextMenuItems[0].childrenItems[1].text = _MEM_ALLOC_STRING(memoryManagerInstance,
		FrameBuffer::GetString(FrameBuffer::Normal));
	contextMenuItems[0].childrenItems[2].Set(GuiElementType::GuiButton, elementSize, GuiElementId::DepthFrameBufferBtn);
	contextMenuItems[0].childrenItems[2].text = _MEM_ALLOC_STRING(memoryManagerInstance,
		FrameBuffer::GetString(FrameBuffer::Depth));
	contextMenuItems[0].childrenItems[3].Set(GuiElementType::GuiButton, elementSize, GuiElementId::DebugFrameBufferBtn);
	contextMenuItems[0].childrenItems[3].text = _MEM_ALLOC_STRING(memoryManagerInstance,
		FrameBuffer::GetString(FrameBuffer::Debug));
	contextMenuItems[0].childrenItems[0].buttonClickedFunction = OnGuiButtonClicked;
	contextMenuItems[0].childrenItems[1].buttonClickedFunction = OnGuiButtonClicked;
	contextMenuItems[0].childrenItems[2].buttonClickedFunction = OnGuiButtonClicked;
	contextMenuItems[0].childrenItems[3].buttonClickedFunction = OnGuiButtonClicked;

	storage->userInterface->SetContextMenu(contextMenuItems);
}

void GenerateScene(MemoryManager* memoryManagerInstance, Scene* scene)
{
	// create materials
	auto whiteMaterialId = scene->AddMaterial(v3f(1, 1, 1), v3f(1, 1, 1), 32);
	auto redMaterialId = scene->AddMaterial(v3f(1, 0, 0), v3f(1, 1, 1), 32);
	auto greenMaterialId = scene->AddMaterial(v3f(0, 1, 0), v3f(1, 1, 1), 32);
	auto blueMaterialId = scene->AddMaterial(v3f(0, 0, 1), v3f(1, 1, 1), 32);
	auto yellowMaterialId = scene->AddMaterial(v3f(1, 1, 0), v3f(1, 1, 1), 32);
	auto cyanMaterialId = scene->AddMaterial(v3f(0, 1, 1), v3f(1, 1, 1), 32);
	auto purpleMaterialId = scene->AddMaterial(v3f(1, 0, 1), v3f(1, 1, 1), 32);
	auto darkRedMaterialId = scene->AddMaterial(v3f(.5, 0, 0), v3f(1, 1, 1), 32);
	auto darkGreenMaterialId = scene->AddMaterial(v3f(0, .5, 0), v3f(1, 1, 1), 32);
	auto darkBlueMaterialId = scene->AddMaterial(v3f(0, 0, .5), v3f(1, 1, 1), 32);
	auto mirrorMaterialId = scene->AddMaterial(v3f(1, 1, 1), v3f(1, 1, 1), 16, .8, 0, 0);
	auto glassMaterialId = scene->AddMaterial(v3f(1, 1, 1), v3f(1, 1, 1), 32, 0, 1, 1.33);

	//GenerateLandscape(scene, 1);
	//GenerateMesh(scene, memoryManagerInstance);
	GenerateEverything(scene, 32, 9);
	//GenerateGrid(scene, v3f(0, 0, 0), v3ui(4, 4, 4), v3f(200, 200, 200), v3f(100, 100, 100), 9);

	//scene->AddMesh(v3f(700, 0, 0), "c:\\filip\\programming\\_projects\\Athena\\data\\objects\\teapot.obj");
	//scene->AddMesh(v3f(200, 0, 0), "d:\\stuff\\Projects\\Athena\\data\\objects\\box.obj");
}

void GenerateLandscape(Scene* scene, int materialCount)
{
	scene->AddSphereLightSource(v3f(0, 5000, 0), 50, 1000000, v3f(1, 1, 1));
	
	ui32 visibility = 128;

	v2i heightRange(128, 1024);
	v3f elementSize(512, 1, 512);
	v3f center(
		((visibility - 1) * elementSize.x) * -.5,
		heightRange.y * -2,
		((visibility - 1) * elementSize.z) * -.5);

	for (ui32 y = 0; y < visibility; y++)
		for (ui32 x = 0; x < visibility; x++)
		{
			const real height = fRND(heightRange.x, heightRange.y);
			scene->AddBox(v3f(x, height / 2, y) * elementSize + center, v3f(elementSize.x, height, elementSize.z), 
				iRND(0, materialCount - 1));
		}
}

void GenerateEverything(Scene* scene, uint complexity, int materialCount)
{
	//scene->AddPointLightSource(v3f(0, 2000, 0), 1000000, v3f(1, 1, 1));
	//scene->AddPointLightSource(v3f(1000, 0, 0), 100000, v3f(1, 0, 0));
	//scene->AddPointLightSource(v3f(-1000, 0, 0), 100000, v3f(0, 1, 0));
	//scene->AddPointLightSource(v3f(0, 0, -1000), 100000, v3f(0, 0, 1));
	//scene->AddPointLightSource(v3f(0, 0, 1000), 100000, v3f(1, 1, 1));

	scene->AddSphereLightSource(v3f(0, 2000, 0), 50, 1000000, v3f(1, 1, 1));
	scene->AddSphereLightSource(v3f(1000, 0, 0), 50, 100000, v3f(1, 0, 0));
	scene->AddSphereLightSource(v3f(-1000, 0, 0), 50, 100000, v3f(0, 1, 0));
	scene->AddSphereLightSource(v3f(0, 0, -1000), 50, 100000, v3f(0, 0, 1));
	scene->AddSphereLightSource(v3f(0, 0, 1000), 50, 100000, v3f(1, 1, 1));

	//scene->AddRotationAroundAnimation(&light1.position, v3f(), v3f(0, 1, 0), 1);
	//scene->AddRotationAroundAnimation(&light2.position, v3f(), v3f(0, 1, 0), 1);
	//scene->AddRotationAroundAnimation(&light3.position, v3f(), v3f(0, 1, 0), 1);
	//scene->AddRotationAroundAnimation(&light4.position, v3f(), v3f(0, 1, 0), 1);

	//v3f axis, center;
	for (auto i = 0; i < complexity; ++i)
	{
		scene->AddSphere(v3f(fRND(-500, 500), fRND(-500, 500), fRND(-500, 500)), fRND(100, 150), iRND(0, 9));
		//center.Set(RANDOMf(-100, 100), RANDOMf(-100, 100), RANDOMf(-100, 100));
		//axis.Set(RANDOMf(-1, 1), RANDOMf(-1, 1), RANDOMf(-1, 1));
		//vectors::Normalize(axis);
		//scene->AddRotationAroundAnimation(&sphere.position, center, axis, RANDOMf(-1, 1));

		scene->AddBox(
			v3f(fRND(-500, 500), fRND(-500, 500), fRND(-500, 500)),
			v3f(fRND(100, 150), fRND(100, 150), fRND(100, 150)),
			iRND(0, 9));
		//center.Set(RANDOMf(-100, 100), RANDOMf(-100, 100), RANDOMf(-100, 100));
		//axis.Set(RANDOMf(-1, 1), RANDOMf(-1, 1), RANDOMf(-1, 1));
		//vectors::Normalize(axis);
		//scene->AddRotationAroundAnimation(&box.position, center, axis, RANDOMf(-1, 1));
	}

	//scene->AddPlane(v3f(0, -1000, 0), v3f(0, 1, 0), 0);
}

void GenerateMesh(Scene* scene, MemoryManager* memoryManagerInstance)
{
	uint redLight = scene->AddPointLightSource(v3f(1000, 0, 0), 100000, v3f(1, 0, 0));
	uint greenLight = scene->AddPointLightSource(v3f(0, 1000, 0), 100000, v3f(0, 1, 0));
	uint blueLight = scene->AddPointLightSource(v3f(0, 0, 1000), 100000, v3f(0, 0, 1));

	//scene->AddSphereLightSource(v3f(1000, 0, 0), 50, 100000, v3f(1, 0, 0));
	//scene->AddSphereLightSource(v3f(0, 1000, 0), 50, 100000, v3f(0, 1, 0));
	//scene->AddSphereLightSource(v3f(0, 0, 1000), 50, 100000, v3f(0, 0, 1));

	scene->AddRotationAroundAnimation(
		&scene->GetObjects().pointLights[redLight].position, v3f(), v3f(0, 1, 0), 1);
	scene->AddRotationAroundAnimation(
		&scene->GetObjects().pointLights[greenLight].position, v3f(), v3f(0, 1, 0), 1);
	scene->AddRotationAroundAnimation(
		&scene->GetObjects().pointLights[blueLight].position, v3f(), v3f(0, 1, 0), 1);

	// teapot
	Mesh teapot(false);
	LoadWavefrontObjectFromFile("c:\\filip\\programming\\_projects\\Athena\\data\\objects\\teapot.obj", 
		memoryManagerInstance, teapot);
	for (uint i = 0; i < teapot.vertices.count; ++i)
		teapot.vertices[i] *= v3f(200, 200, 200);
	uint teapotId = scene->AddMesh(teapot);

	list_of<v3f> vertices(memoryManagerInstance);
	list_of<Triangle> triangles(memoryManagerInstance);

	//Mesh mesh(true);
	//vertices.Clear();
	//vertices.Add(v3f(-250, 0, -250));
	//vertices.Add(v3f(250, 0, -250));
	//vertices.Add(v3f(250, 0, 250));
	//vertices.Add(v3f(-250, 0, 250));
	//vertices.Add(v3f(0, 750, 0));
	//mesh.vertices = vertices.CopyToArray();
	//triangles.Clear();
	//triangles.Add(Triangle(v3i(4, 1, 0)));
	//triangles.Add(Triangle(v3i(4, 2, 1)));
	//triangles.Add(Triangle(v3i(4, 3, 2)));
	//triangles.Add(Triangle(v3i(4, 0, 3)));
	//triangles.Add(Triangle(v3i(0, 1, 2)));
	//triangles.Add(Triangle(v3i(0, 2, 3)));
	//mesh.triangles = triangles.CopyToArray();
	//uint meshId = scene->AddMesh(mesh);

	//scene->AddRotationAroundAnimation(
	//	&scene->GetObjects().meshes[meshId].vertices[0], v3f(), v3f(0, 1, 0), 1);
	//scene->AddRotationAroundAnimation(
	//	&scene->GetObjects().meshes[meshId].vertices[1], v3f(), v3f(0, 1, 0), 1);
	//scene->AddRotationAroundAnimation(
	//	&scene->GetObjects().meshes[meshId].vertices[2], v3f(), v3f(0, 1, 0), 1);
	//scene->AddRotationAroundAnimation(
	//	&scene->GetObjects().meshes[meshId].vertices[3], v3f(), v3f(0, 1, 0), 1);

	//Mesh minuteArrowMesh(true);
	//vertices.Clear();
	//vertices.Add(v3f(-100, 0, 0));
	//vertices.Add(v3f(100, 0, 0));
	//vertices.Add(v3f(0, 800, 0));
	//vertices.Add(v3f(0, 0, -100));
	//vertices.Add(v3f(0, -100, 0));
	//minuteArrowMesh.vertices = vertices.CopyToArray();
	//triangles.Clear();
	//triangles.Add(Triangle(v3i(0, 1, 2)));
	//triangles.Add(Triangle(v3i(3, 0, 2)));
	//triangles.Add(Triangle(v3i(1, 3, 2)));
	//triangles.Add(Triangle(v3i(1, 4, 3)));
	//triangles.Add(Triangle(v3i(0, 3, 4)));
	//triangles.Add(Triangle(v3i(1, 0, 4)));
	//minuteArrowMesh.triangles = triangles.CopyToArray();
	//uint minuteArrowMeshId = scene->AddMesh(minuteArrowMesh);

	//scene->AddRotationAroundAnimation(
	//	&scene->GetObjects().meshes[minuteArrowMeshId].vertices[2], v3f(), v3f(0, 0, -1), 1);
	//scene->AddRotationAroundAnimation(
	//	&scene->GetObjects().meshes[minuteArrowMeshId].vertices[0], v3f(), v3f(0, 0, -1), 1);
	//scene->AddRotationAroundAnimation(
	//	&scene->GetObjects().meshes[minuteArrowMeshId].vertices[1], v3f(), v3f(0, 0, -1), 1);
	//scene->AddRotationAroundAnimation(
	//	&scene->GetObjects().meshes[minuteArrowMeshId].vertices[4], v3f(), v3f(0, 0, -1), 1);

	//Mesh hourArrowMesh(true);
	//vertices.Clear();
	//vertices.Add(v3f(-100, 0, 0));
	//vertices.Add(v3f(100, 0, 0));
	//vertices.Add(v3f(0, 600, 0));
	//vertices.Add(v3f(0, 0, -100));
	//vertices.Add(v3f(0, -100, 0));
	//hourArrowMesh.vertices = vertices.CopyToArray();
	//triangles.Clear();
	//triangles.Add(Triangle(v3i(0, 1, 2)));
	//triangles.Add(Triangle(v3i(3, 0, 2)));
	//triangles.Add(Triangle(v3i(1, 3, 2)));
	//triangles.Add(Triangle(v3i(1, 4, 3)));
	//triangles.Add(Triangle(v3i(0, 3, 4)));
	//triangles.Add(Triangle(v3i(1, 0, 4)));
	//hourArrowMesh.triangles = triangles.CopyToArray();
	//uint hourArrowMeshId = scene->AddMesh(hourArrowMesh);

	//scene->AddRotationAroundAnimation(
	//	&scene->GetObjects().meshes[hourArrowMeshId].vertices[2], v3f(), v3f(0, 0, -1), 1.0/12);
	//scene->AddRotationAroundAnimation(
	//	&scene->GetObjects().meshes[hourArrowMeshId].vertices[0], v3f(), v3f(0, 0, -1), 1.0/12);
	//scene->AddRotationAroundAnimation(
	//	&scene->GetObjects().meshes[hourArrowMeshId].vertices[1], v3f(), v3f(0, 0, -1), 1.0/12);
	//scene->AddRotationAroundAnimation(
	//	&scene->GetObjects().meshes[hourArrowMeshId].vertices[4], v3f(), v3f(0, 0, -1), 1.0/12);

	vertices.Destroy();
	triangles.Destroy();
}

void GenerateGrid(Scene* scene,
	const v3f& position, const v3ui& elementCount, const v3f& elementSize, const v3f& spacing, int materialCount)
{
	v3f center(
		((elementCount.x - 1) * elementSize.x + (elementCount.x - 1) * spacing.x) * -.5,
		((elementCount.y - 1) * elementSize.y + (elementCount.y - 1) * spacing.y) * -.5,
		((elementCount.z - 1) * elementSize.z + (elementCount.z - 1) * spacing.z) * -.5);
	for (ui32 z = 0; z < elementCount.z; z++)
		for (ui32 y = 0; y < elementCount.y; y++)
			for (ui32 x = 0; x < elementCount.x; x++)
			{
				scene->AddSphere(v3f(x, y, z) * (elementSize + spacing) + center, elementSize.x / 2,
					iRND(0, materialCount - 1));
				//scene->AddBox(v3f(x, y, z) * (elementSize + spacing) + center, elementSize, iRND(0, 9));
			}
}

void ProcessInput(AthenaStorage* storage, const application_input* input, real32 timeElapsed)
{
	TIMED_BLOCK(&storage->timers[TimerId::ProcessInput]);

	const real32 step = (32 * timeElapsed);
	v2ui mousePosition(input->mousePosition.x, input->mousePosition.y);

	// update user interface
	storage->userInterface->Update(storage, mousePosition);
	for (int i = 0; i < MouseButton::Count; ++i)
	{
		if (input->mouseButtons[i].pressed)
			storage->userInterface->MouseButtonClicked((MouseButton::Enum)i, mousePosition, storage);

		storage->userInterface->MouseButtonState((MouseButton::Enum)i, input->mouseButtons[i].isDown, mousePosition,
			storage);
	}

	// right mouse button down
	if (input->mouseButtons[MouseButton::Right].isDown)
	{
		if (input->mouseButtons[MouseButton::Right].pressed)
			storage->lastMousePosition = mousePosition;

		if (mousePosition.x != storage->lastMousePosition.x || mousePosition.y != storage->lastMousePosition.y)
		{
			// rotate around target
			storage->camera->RotateAroundTarget(
				mousePosition.x - storage->lastMousePosition.x,
				mousePosition.y - storage->lastMousePosition.y);

			// fps view
			//storage->camera->RotateView(
			//	mousePosition.x - storage->lastMousePosition.x,
			//	mousePosition.y - storage->lastMousePosition.y);

			storage->lastMousePosition = mousePosition;
		}
	}

	// process keyboard & gamepads
	for (int i = 0; i < MAX_CONTROLLERS; ++i)
	{
		const controller_input* controller = application_input::GetController(input, i);

		if (!controller->isAnalog)
		{
			// camera movement
			if (controller->moveUp.isDown) storage->camera->Move(step);
			if (controller->moveDown.isDown) storage->camera->Move(-step);
			if (controller->moveLeft.isDown) storage->camera->Strafe(-step);
			if (controller->moveRight.isDown) storage->camera->Strafe(step);

			// menu handling
			if (controller->menu.isDown)
				storage->userInterface->Show();
			else
				storage->userInterface->Hide();
		}
		else
		{
			// TODO gamepad handling
		}
	}
}

void RenderOnThread(AthenaStorage* storage, uint32 threadId)
{
	srand((unsigned int)time(NULL));

	const v2ui& frameSize = storage->frame.size;
	const v2ui& pixelSize = storage->pixelSizes[storage->renderingParameters.currentPixelSizeId];

	const ui32 regionCount = storage->renderingRegions.x * storage->renderingRegions.y;
	const ui32 regionIncrement = storage->renderingParameters.softwareRenderingThreadsCount > 1 ?
		storage->renderingParameters.softwareRenderingThreadsCount : 1;
	for (uint32 regionId = threadId; regionId < regionCount; regionId += regionIncrement)
	{
		// ak pocet regionov je stvorec a strany su mocniny 2
		//v2ui regionStart = ZOrder::Decode2ui(regionId);
		// inak
		v2ui regionStart(regionId % storage->renderingRegions.x, regionId / storage->renderingRegions.x);
		regionStart *= storage->renderingRegionSize;
	
		for (auto y = regionStart.y; y < (regionStart.y + storage->renderingRegionSize.y); y += pixelSize.y)
		{
			for (auto x = regionStart.x; x < (regionStart.x + storage->renderingRegionSize.x); x += pixelSize.x)
			{
				Render(
					storage->camera, 
					storage->scene, 
					storage->frame, 
					y * frameSize.x + x,
					v2ui(x, y), 
					pixelSize, 
					storage->frame.countSinceChange, 
					storage->renderingParameters);

				//v2f threadColor(
				//	(real)(x - regionStart.x) / storage->renderingRegionSize.x,
				//	(real)(y - regionStart.y) / storage->renderingRegionSize.y);
				//v4b& debug = storage->frame.buffer[FrameBuffer::Debug][y * frameSize.x + x];
				//debug.x = REAL_TO_BYTE(threadColor.x);
				//debug.y = REAL_TO_BYTE(threadColor.y);
			}
		}
	}
}

void PostProcessOutput(AthenaStorage* storage, array_of<uint32> output)
{
	TIMED_BLOCK(&storage->timers[TimerId::PostProcess]);

	// TODO optimalizovat, strasne dlho to trva pri PixelSize=1x1 (~7ms)

	const v2ui& frameSize = storage->frame.size;
	const v2ui& pixelSize = storage->pixelSizes[storage->renderingParameters.currentPixelSizeId];

	// storage->frame zacina vlavo hore a pokracuje do praveho spodneho rohu
	// output vsak zacina vlavo dole, a pokracuje do praveho horneho rohu
	// preto budem vystup spracovavat od posledneho riadku postupne az po prvy

	// source buffer
	const v4b* frameBuffer = storage->frame.buffer[storage->frame.current].ptr;

	// for each row and pixel in destination buffer
	for (uint32 y = 0; y < frameSize.y; y += pixelSize.y)
	{
		for (uint32 x = 0; x < frameSize.x; x += pixelSize.x)
		{
			const rgba_as_uint32 frameBufferColor(frameBuffer + x + frameSize.x * (frameSize.y - y - pixelSize.y));
			auto color = frameBufferColor.GetUi32();
			for (uint32 py = 0; py < pixelSize.y; ++py)
				for (uint32 px = 0; px < pixelSize.x; ++px)
					output[x + px + frameSize.x * (y + py)] = color;
		}
	}
}

void OnGuiButtonClicked(GUI_BUTTON_CLICKED_PARAMETERS)
{
	switch (buttonId)
	{
		case GuiElementId::ColorFrameBufferBtn:		storage->frame.current = FrameBuffer::Color; break;
		case GuiElementId::NormalFrameBufferBtn:	storage->frame.current = FrameBuffer::Normal; break;
		case GuiElementId::DepthFrameBufferBtn:		storage->frame.current = FrameBuffer::Depth; break;
		case GuiElementId::DebugFrameBufferBtn:		storage->frame.current = FrameBuffer::Debug; break;
	}
}

void OnSceneButtonClicked(SCENE_CLICKED_PARAMETERS)
{
	switch (mouseButton)
	{
		case MouseButton::Left:
		{
			auto objectIdSelected = GetObjectIdAt(mousePosition, storage->frame);
			if (objectIdSelected)
				LOG_DEBUG("%s-click on %s%d", MouseButton::GetString(mouseButton),
					ObjectType::GetString(objectIdSelected->Type()), objectIdSelected->index);
		}
		break;
	}
}

const ObjectId* GetObjectIdAt(v2ui point, const Frame& frame)
{
	if (point.x < frame.size.x && point.y < frame.size.y)
	{
		// TODO object picking nefunguje moc dobre, aj ked v buffry vyzeraju byt spravne hodnoty

		auto objectId = frame.objectIdBuffer.ptr + (point.x + point.y * frame.size.x);
		if (objectId->Type() != ObjectType::Unknown)
			return objectId;
	}

	return null;
}
