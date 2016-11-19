#ifndef __rendering_h
#define __rendering_h

#include "TypeDefs.h"


#define RENDERER_VALUES(_) \
    _(CPU,=0) \
    _(GPU_CUDA,) \
	_(GPU_OpenCL, )
DECLARE_ENUM(Renderer, RENDERER_VALUES)
#undef RENDERER_VALUES


#define TRACING_METHOD_VALUES(_) \
    _(StraightForward,=0) \
    _(BoundingIntervalHierarchy,) \
    _(Octree,)
DECLARE_ENUM(TracingMethod, TRACING_METHOD_VALUES)
#undef TRACING_METHOD_VALUES


#define RENDERING_MODE_VALUES(_) \
    _(Continuous,=0) \
    _(Progressive,) \
    _(SingleImage,)
DECLARE_ENUM(RenderingMode, RENDERING_MODE_VALUES)
#undef RENDERING_MODE_VALUES


#define RENDERING_METHOD_VALUES(_) \
    _(RayTracing,=0) \
    _(RayMarching,)
DECLARE_ENUM(RenderingMethod, RENDERING_METHOD_VALUES)
#undef RENDERING_METHOD_VALUES


struct RenderingParameters
{
	ui32 ambientOcclusionSamples;
	real ambientOcclusionModifier;
	ui32 maxRayTracingDepth;
	ui32 currentPixelSizeId;
	b32 multiThreadedOctreeUpdate;
	ui32 maxOctreeDepth;
	ui32 maxBihDepth;
	ui32 maxBihLeafObjects;
	ui32 softwareRenderingThreadsCount;

	RenderingMethod::Enum renderingMethod;
	RenderingMode::Enum renderingMode;
	TracingMethod::Enum tracingMethod;
	Renderer::Enum currentRenderer;
};

class Camera;
struct Frame;
class Scene;

void Render(const Camera* camera, const Scene* scene, const Frame& frame, uint frameOffset,
	const v2ui& pixel, const v2ui& pixelSize, uint frameCountSinceChange, const RenderingParameters& parameters);

#endif __rendering_h
