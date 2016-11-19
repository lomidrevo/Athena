#ifndef __aa_cell_h
#define __aa_cell_h

#include "Ray.h"
#include "TypeDefs.h"
#include "Vectors.h"


struct AACell
{
	v3f minCorner;
	v3f maxCorner;

	inline __device__ AACell& operator-=(const v3f& v) { minCorner -= v; maxCorner -= v; return *this; }
	inline __device__ AACell operator-(const v3f& v) const { AACell tmp(*this); tmp -= v; return tmp; }
	
	__device__ real DistanceFrom(const v3f& point) const
	{
		v3f tmpPoint(point);
		vectors::Abs(tmpPoint);
		tmpPoint -= maxCorner;

		return MAX3(tmpPoint.x, tmpPoint.y, tmpPoint.z);
	}

	__device__ real Hit(const Ray& ray) const
	{
		real tMin = _INFINITY;

		if (!IsInside(ray.origin))
		{
			if (ray.origin.x <= minCorner.x)
			{
				const real t = (ray.origin.x + ABS(minCorner.x)) / -ray.direction.x;
				if (t < tMin)
				{
					real hit = ray.origin.y + ray.direction.y * t;
					if (hit >= minCorner.y && hit <= maxCorner.y)
					{
						hit = ray.origin.z + ray.direction.z * t;
						if (hit >= minCorner.z && hit <= maxCorner.z)
							tMin = t;
					}
				}
			}
			else if (ray.origin.x >= maxCorner.x)
			{
				const real t = (-ray.origin.x + ABS(maxCorner.x)) / ray.direction.x;
				if (t < tMin)
				{
					real hit = ray.origin.y + ray.direction.y * t;
					if (hit >= minCorner.y && hit <= maxCorner.y)
					{
						hit = ray.origin.z + ray.direction.z * t;
						if (hit >= minCorner.z && hit <= maxCorner.z)
							tMin = t;
					}
				}
			}

			if (ray.origin.y <= minCorner.y)
			{
				const real t = (ray.origin.y + ABS(minCorner.y)) / -ray.direction.y;
				if (t < tMin)
				{
					real hit = ray.origin.x + ray.direction.x * t;
					if (hit > minCorner.x && hit < maxCorner.x)
					{
						hit = ray.origin.z + ray.direction.z * t;
						if (hit > minCorner.z && hit < maxCorner.z)
							tMin = t;
					}
				}
			}
			else if (ray.origin.y >= maxCorner.y)
			{
				const real t = (-ray.origin.y + ABS(maxCorner.y)) / ray.direction.y;
				if (t < tMin)
				{
					real  hit = ray.origin.x + ray.direction.x * t;
					if (hit > minCorner.x && hit < maxCorner.x)
					{
						hit = ray.origin.z + ray.direction.z * t;
						if (hit > minCorner.z && hit < maxCorner.z)
							tMin = t;
					}
				}
			}

			if (ray.origin.z <= minCorner.z)
			{
				const real t = (ray.origin.z + ABS(minCorner.z)) / -ray.direction.z;
				if (t < tMin)
				{
					real hit = ray.origin.x + ray.direction.x * t;
					if (hit > minCorner.x && hit < maxCorner.x)
					{
						hit = ray.origin.y + ray.direction.y * t;
						if (hit > minCorner.y && hit < maxCorner.y)
							tMin = t;
					}
				}
			}
			else if (ray.origin.z >= maxCorner.z)
			{
				const real t = (-ray.origin.z + ABS(maxCorner.z)) / ray.direction.z;
				if (t < tMin)
				{
					real hit = ray.origin.x + ray.direction.x * t;
					if (hit > minCorner.x && hit < maxCorner.x)
					{
						hit = ray.origin.y + ray.direction.y * t;
						if (hit > minCorner.y && hit < maxCorner.y)
							tMin = t;
					}
				}
			}
		}
		else
		{
			if (ray.direction.x > 0)
			{
				const real t = (-ray.origin.x + maxCorner.x) / ray.direction.x;
				if (t < tMin)
				{
					real hit = ray.origin.y + ray.direction.y * t;
					if (hit > minCorner.y && hit < maxCorner.y)
					{
						hit = ray.origin.z + ray.direction.z * t;
						if (hit > minCorner.z && hit < maxCorner.z)
							tMin = t;
					}
				}
			}
			else
			{
				const real t = (ray.origin.x + minCorner.x) / -ray.direction.x;
				if (t < tMin)
				{
					real hit = ray.origin.y + ray.direction.y * t;
					if (hit > minCorner.y && hit < maxCorner.y)
					{
						hit = ray.origin.z + ray.direction.z * t;
						if (hit > minCorner.z && hit < maxCorner.z)
							tMin = t;
					}
				}
			}

			if (ray.direction.y > 0)
			{
				const real t = (-ray.origin.y + maxCorner.y) / ray.direction.y;
				if (t < tMin)
				{
					real hit = ray.origin.x + ray.direction.x * t;
					if (hit > minCorner.x && hit < maxCorner.x)
					{
						hit = ray.origin.z + ray.direction.z * t;
						if (hit > minCorner.z && hit < maxCorner.z)
							tMin = t;
					}
				}
			}
			else
			{
				const real t = (ray.origin.y + minCorner.y) / -ray.direction.y;
				if (t < tMin)
				{
					real hit = ray.origin.x + ray.direction.x * t;
					if (hit > minCorner.x && hit < maxCorner.x)
					{
						hit = ray.origin.z + ray.direction.z * t;
						if (hit > minCorner.z && hit < maxCorner.z)
							tMin = t;
					}
				}
			}

			if (ray.direction.z > 0)
			{
				const real t = (-ray.origin.z + maxCorner.z) / ray.direction.z;
				if (t < tMin)
				{
					real hit = ray.origin.x + ray.direction.x * t;
					if (hit > minCorner.x && hit < maxCorner.x)
					{
						hit = ray.origin.y + ray.direction.y * t;
						if (hit > minCorner.y && hit < maxCorner.y)
							tMin = t;
					}
				}
			}
			else
			{
				const real t = (ray.origin.z + minCorner.z) / -ray.direction.z;
				if (t < tMin)
				{
					real hit = ray.origin.x + ray.direction.x * t;
					if (hit > minCorner.x && hit < maxCorner.x)
					{
						hit = ray.origin.y + ray.direction.y * t;
						if (hit > minCorner.y && hit < maxCorner.y)
							tMin = t;
					}
				}
			}
		}

		return tMin;
	}

	__device__ bool Collide(const Ray& ray, real from = EPSILON, real to = _INFINITY) const
	{
		// "An Efficient and Robust Ray-Box Intersection Algorithm"
		// Journal of graphics tools, 10(1):49-54, 2005
		// Amy Williams, Steve Barrus, R. Keith Morley, and Peter Shirley
		// http://jgt.akpeters.com/papers/WilliamsEtAl05/

		real tmin = ((ray.sign.x == 0 ? minCorner.x : maxCorner.x) + ray.origin.x)
			* ray.invDirection.x;
		real tmax = ((ray.sign.x == 1 ? minCorner.x : maxCorner.x) + ray.origin.x)
			* ray.invDirection.x;
		const real tymin = ((ray.sign.y == 0 ? minCorner.y : maxCorner.y) + ray.origin.y)
			* ray.invDirection.y;
		const real tymax = ((ray.sign.y == 1 ? minCorner.y : maxCorner.y) + ray.origin.y)
			* ray.invDirection.y;

		if ((tmin > tymax) || (tymin > tmax))
			return false;
		else
		{
			if (tymin > tmin)
				tmin = tymin;

			if (tymax < tmax)
				tmax = tymax;

			const real tzmin = ((ray.sign.z == 0 ? minCorner.z : maxCorner.z) + ray.origin.z)
				* ray.invDirection.z;
			const real tzmax = ((ray.sign.z == 1 ? minCorner.z : maxCorner.z) + ray.origin.z)
				* ray.invDirection.z;

			if ((tmin > tzmax) || (tzmin > tmax))
				return false;

			if (tzmin > tmin)
				tmin = tzmin;

			if (tzmax < tmax)
				tmax = tzmax;
		}

		return ((tmin < to) && (tmax > from));

	}

	__device__ bool IsInside(const v3f& point) const
	{
		if (point.x < minCorner.x || point.x > maxCorner.x)
			return false;

		if (point.y < minCorner.y || point.y > maxCorner.y)
			return false;

		if (point.z < minCorner.z || point.z > maxCorner.z)
			return false;

		return true;
	}
	
	__device__ void GetNormalAt(const v3f& point, v3f& normal) const
	{
		if (ABS(point.x - maxCorner.x) < EPSILON)
		{
			normal.Set(1, 0, 0);
		}
		else if (ABS(point.x - minCorner.x) < EPSILON)
		{
			normal.Set(-1, 0, 0);
		}

		else if (ABS(point.y - maxCorner.y) < EPSILON)
		{
			normal.Set(0, 1, 0);
		}
		else if (ABS(point.y - minCorner.y) < EPSILON)
		{
			normal.Set(0, -1, 0);
		}

		else if (ABS(point.z - maxCorner.z) < EPSILON)
		{
			normal.Set(0, 0, 1);
		}
		else if (ABS(point.z - minCorner.z) < EPSILON)
		{
			normal.Set(0, 0, -1);
		}
	}

	__device__ bool AndAACell(const AACell& cell) const
	{
		// are cells away from each other ? (no overlaping)
		if (minCorner.x > cell.maxCorner.x ||
			maxCorner.x < cell.minCorner.x ||
			minCorner.y > cell.maxCorner.y ||
			maxCorner.y < cell.minCorner.y ||
			minCorner.z > cell.maxCorner.z ||
			maxCorner.z < cell.minCorner.z)
			return false;

		// cell overlaps this->cell
		return true;
	}
};

#endif __aa_cell_h
