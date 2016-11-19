#ifndef __ray_marching_h
#define __ray_marching_h

#include "TypeDefs.h"
#include "Vectors.h"

#define RAY_MARCHING_STEPS 32


struct RayMarchResult
{
	v3f color;
	v3f normal;
	real distance;
	ui32 rayMarchingStep;
};

struct Ray;
struct RenderingParameters;
class Scene;

RayMarchResult RayMarch(const Scene* scene, const Ray& ray, const RenderingParameters& parameters);

#endif __ray_marching_h
