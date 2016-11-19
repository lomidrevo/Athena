#ifndef __box_h
#define __box_h

#include "AACell.h"
#include "Object.h"
#include "Ray.h"
#include "TypeDefs.h"
#include "Vectors.h"


struct Box : Object
{
	AACell cell;
	ui32 materialIndex;

	__device__ Box()
	{
		// v BIH sa pouziva toto + priame nastavenie cell
	}

	__device__ Box(AACell cell)
	{
		// v Octree sa pouziva toto

		position = (cell.maxCorner + cell.minCorner) * .5;
		this->cell.minCorner = (cell.maxCorner - cell.minCorner) * -.5;
		this->cell.maxCorner = (cell.maxCorner - cell.minCorner) * .5;
	}

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

	__device__ void GetNormalAt(const v3f& point, v3f& normal) const
	{
		v3f localPoint;
		localPoint = point - position;

		return cell.GetNormalAt(localPoint, normal);
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

#endif __box_h
