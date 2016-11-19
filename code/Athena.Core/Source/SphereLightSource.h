#ifndef __sphere_sphere_light_h
#define __sphere_sphere_light_h

#include "AACell.h"
#include "Light.h"
#include <math.h>
#include "Ray.h"
#include "TypeDefs.h"
#include "Vectors.h"


struct SphereLightSource : Light
{
	AACell cell;
	real radius;
	ui32 lightPointCount;
	
	__device__ real Hit(const Ray& ray) const
	{
		v3f distVector = position - ray.origin;
		const real distance2 = vectors::Dot(distVector, distVector);
		const real dot = vectors::Dot(distVector, ray.direction);

		const real radius2 = radius * radius;
		if (distance2 < radius2)
			return dot + (real)sqrt(radius2 - distance2 + dot * dot);

		if (dot < EPSILON)
			return -_INFINITY;

		const real l2hc = radius2 - distance2 + dot * dot;
		if (l2hc < EPSILON)
			return -_INFINITY;
		
		return dot - (real)sqrt(l2hc);
	}

	__device__ bool Collide(const Ray& ray, real from = EPSILON, real to = _INFINITY) const
	{
		v3f distVector = position - ray.origin;
		const real distance = vectors::Dot(distVector, distVector);

		if (distance < radius * radius) 
		{
			const real dot = vectors::Dot(distVector, ray.direction);

			const real t = (real)sqrt(radius * radius - distance + dot*dot) + dot;
			if (t > to - EPSILON || t < from + EPSILON)
				return false;
		} 
		else
		{
			const real dot = vectors::Dot(distVector, ray.direction);
			if (dot < EPSILON)
				return false;

			const real l2hc = radius * radius - distance + dot * dot;
			if (l2hc < EPSILON)
				return false;

			const real t = dot - (real)sqrt(l2hc);
			if (t > to - EPSILON || t < from + EPSILON)
				return false;
		}

		return true;
	}

	__device__ void UpdateCell()
	{
		cell.minCorner.Set(-radius, -radius, -radius);
		cell.maxCorner.Set(radius, radius, radius);
	}
};

#endif __sphere_sphere_light_h
