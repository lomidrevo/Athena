#ifndef __ray_tracing_h
#define __ray_tracing_h

#include "Object.h"
#include "TypeDefs.h"
#include "Vectors.h"


struct RayTraceResult
{
	v3f color;
	v3f normal;
	real distance;
	ObjectId objectId;

	uint testCount;
};

class BIH;
struct Objects;
class Octree;
struct RenderingParameters;

__device__ RayTraceResult RayTrace(const Objects& objects, const RenderingParameters& parameters, const Ray& ray,
	const array_of<v3f>& randomDirections, const BIH* bih, const Octree* octree, ui32 depth);

#endif __ray_tracing_h
