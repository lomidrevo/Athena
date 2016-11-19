#ifndef __sphere_h
#define __sphere_h

#include "AACell.h"
#include <math.h>
#include "Object.h"
#include "Ray.h"
#include "TypeDefs.h"
#include "Vectors.h"


struct Sphere : Object
{
	AACell cell;
	real radius;
	ui32 materialIndex;

	__device__ real DistanceFrom(const v3f& point) const
	{
		return vectors::Length(point - position) - radius;
	}

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

	__device__ bool IsInside(const v3f& point) const
	{
		return vectors::Distance2(point, position) < radius * radius;
	}

	__device__ void GetNormalAt(const v3f& point, v3f& normal) const
	{
		normal = point - position;
		vectors::Normalize(normal);
	}

	__device__ bool AndAACell(const AACell& cell) const
	{
		if (!this->cell.AndAACell(cell - position))
			return false;

		// is cell inside of object ? - used for voxelization, object will be empty inside
		const v3f cellSize = cell.maxCorner - cell.minCorner;
		const real radius2 = radius * radius;
		if (vectors::Distance2(position, cell.minCorner) < radius2 &&
			vectors::Distance2(position, cell.minCorner + cellSize * v3f(1, 0, 0)) < radius2 &&
			vectors::Distance2(position, cell.minCorner + cellSize * v3f(0, 0, 1)) < radius2 &&
			vectors::Distance2(position, cell.minCorner + cellSize * v3f(1, 0, 1)) < radius2 &&
			vectors::Distance2(position, cell.maxCorner) < radius2 &&
			vectors::Distance2(position, cell.maxCorner - cellSize * v3f(1, 0, 0)) < radius2 &&
			vectors::Distance2(position, cell.maxCorner - cellSize * v3f(0, 0, 1)) < radius2 &&
			vectors::Distance2(position, cell.maxCorner - cellSize * v3f(1, 0, 1)) < radius2)
			return false;

		// A Simple Method for Box - Sphere Intersection Testing by Jim Arvo from "Graphics Gems"
		// Academic Press, 1990

		real dmin = 0;
		if (position.x < cell.minCorner.x)
			dmin += (position.x - cell.minCorner.x) * (position.x - cell.minCorner.x);
		else if (position.x > cell.maxCorner.x)
			dmin += (position.x - cell.maxCorner.x) * (position.x - cell.maxCorner.x);

		if (position.y < cell.minCorner.y)
			dmin += (position.y - cell.minCorner.y) * (position.y - cell.minCorner.y);
		else if (position.y > cell.maxCorner.y)
			dmin += (position.y - cell.maxCorner.y) * (position.y - cell.maxCorner.y);

		if (position.z < cell.minCorner.z)
			dmin += (position.z - cell.minCorner.z) * (position.z - cell.minCorner.z);
		else if (position.z > cell.maxCorner.z)
			dmin += (position.z - cell.maxCorner.z) * (position.z - cell.maxCorner.z);

		return dmin <= (radius * radius);
	}

	__device__ void UpdateCell()
	{
		cell.minCorner.Set(-radius, -radius, -radius);
		cell.maxCorner.Set(radius, radius, radius);
	}
};

#endif __sphere_h
