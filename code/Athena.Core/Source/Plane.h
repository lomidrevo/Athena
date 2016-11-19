#ifndef __plane_h
#define __plane_h

#include "Object.h"
#include "Ray.h"
#include "TypeDefs.h"
#include "Vectors.h"


struct Plane : Object
{
	v3f normal;
	ui32 materialIndex;

	__device__ real DistanceFrom(const v3f& point) const
	{
		v3f localPoint = point - position;
		
		// TODO nemal by tu byt localPoint ?
		return vectors::Dot(point, normal);
	}
	
	__device__ real Hit(const Ray& ray) const
	{
		const real denom = vectors::Dot(normal, ray.direction);
		if (ABS(denom) > EPSILON)
			return vectors::Dot(position - ray.origin, normal) / denom;

		return _INFINITY;
	}

	__device__ bool Collide(const Ray& ray, real from = EPSILON, real to = _INFINITY) const
	{
		//v3f localOrigin;
		//xyz_SUB_VECTORS(localOrigin, ray.origin, position);

		//if (xyz_DOT(normal, localOrigin) < 0)
		//	// ray origin is bellow plane
		//	return xyz_DOT(ray.direction, normal) > 0;
		//else
		//	// ray origin is above plane
		//	return xyz_DOT(ray.direction, normal) < 0;
	
		real t = Hit(ray);
		return (t > from && t < to);
	}

	__device__ bool IsInside(const v3f& point) const
	{
		return (vectors::Dot(normal, point - position) < 0);
	}

	__device__ void GetNormalAt(const v3f& point, v3f& normal) const
	{
		normal = this->normal;
	}

};

#endif __plane_h
