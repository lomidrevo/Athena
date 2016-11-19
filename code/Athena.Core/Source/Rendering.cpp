
#include "BoundingIntervalHierarchy.h"
#include "Camera.h"
#include "Convert.h"
#include "Frame.h"
#include "Gradient.h"
#include "Octree.h"
#include "RayMarching.h"
#include "RayTracing.h"
#include "Rendering.h"
#include "Scene.h"


void Render(const Camera* camera, const Scene* scene, const Frame& frame, uint frameOffset,
	const v2ui& pixel, const v2ui& pixelSize, uint frameCountSinceChange,
	const RenderingParameters& parameters)
{
	RayTraceResult result = RayTrace(
		scene->GetObjects(),
		parameters,
		Ray::GetPrimary(camera->GetParameters(), pixel, pixelSize, fRND(.2, .8)),
		scene->GetRandomDirections(),
		scene->GetBIH(),
		scene->GetOctree(),
		0);

	// TODO raymarching nefunguje :/
	//RayMarchResult result = RayMarch(
	//	scene, 
	//	Ray::GetPrimary(camera->GetParameters(), pixel, pixelSize), 
	//	parameters);
	
	// color is average since last view update
	frame.colorAccBuffer[frameOffset] += result.color;
	result.color = frame.colorAccBuffer[frameOffset] / (real)frameCountSinceChange;

	// color output
	frame.buffer[FrameBuffer::Color][frameOffset] = Vector3fToVector4b(result.color);

	// normal output
	frame.buffer[FrameBuffer::Normal][frameOffset] = Vector3fToVector4b(vectors::Abs(result.normal));

	// objectId output
	//frame.objectIdBuffer[frameOffset] = result.objectId;

	v4f debugValues;

	// ray marching debug values
	//if (parameters.tracingMethod != TracingMethod::Octree)
	//	debugValues.x = (real)result.rayMarchingStep / RAY_MARCHING_STEPS;
	//debugValues.y = result.color.y;
	//debugValues.z = result.color.z;

	// raytracing debug values
	if (result.objectId._value && result.objectId.type != ObjectType::Voxel)
	{
		debugValues.y = (real)result.objectId.index / scene->GetObjects().counts[result.objectId.type];
		debugValues.z = (real)result.objectId.type / ObjectType::Count;
	}
	vectors::Clamp(0, 1, debugValues);
	
	// debug output
	frame.buffer[FrameBuffer::Debug][frameOffset] = Vector4fToVector4b(debugValues);

	real depth = 0;
	if (parameters.tracingMethod == TracingMethod::BoundingIntervalHierarchy)
		depth = (real)result.testCount /
		(scene->GetBIH()->GetCurrentNodeCount() + scene->GetObjects().everything.currentCount);
	else if (parameters.tracingMethod == TracingMethod::Octree)
		depth = (real)result.testCount / scene->GetOctree()->GetCurrentNodeCount();

	// depth output
	frame.buffer[FrameBuffer::Depth][frameOffset] = Vector3fToVector4b(GetHeatMapColor(depth));
}
