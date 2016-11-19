#ifndef __box_sphere_light_h
#define __box_sphere_light_h

#include "AACell.h"
#include "Light.h"
#include <math.h>
#include "Ray.h"
#include "TypeDefs.h"
#include "Vectors.h"


struct BoxLightSource : Light
{
	AACell cell;
	
	__device__ real DistanceFrom(const v3f& point) const
	{
		v3f localPoint = point - position;

		return cell.DistanceFrom(localPoint);
	}

	__device__ real Hit(const Ray& ray) const
	{
		Ray localRay(ray);
		localRay.origin = ray.origin - position;

		return cell.Hit(localRay);
	}
	
	__device__ bool Collide(const Ray& ray, real from = EPSILON, real to = _INFINITY) const
	{
		Ray localRay(ray);
		localRay.origin = position - ray.origin;

		return cell.Collide(localRay, from, to);
	}

	__device__ bool IsInside(const v3f& point) const
	{
		v3f localPoint;
		localPoint = point - position;

		return cell.IsInside(localPoint);
	}

	__device__ bool AndAACell(const AACell& cell) const
	{
		if (!this->cell.AndAACell(cell - position))
			return false;

		// is cell inside of object ? - used for voxelization, object will be empty inside
		if (this->cell.minCorner.x < cell.minCorner.x && cell.maxCorner.x < this->cell.maxCorner.x &&
			this->cell.minCorner.y < cell.minCorner.y && cell.maxCorner.y < this->cell.maxCorner.y &&
			this->cell.minCorner.z < cell.minCorner.z && cell.maxCorner.z < this->cell.maxCorner.z)
			return false;
		
		return true;
	}
};

#endif __box_sphere_light_h
