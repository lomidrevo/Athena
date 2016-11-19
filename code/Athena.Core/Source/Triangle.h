#ifndef __triangle_h
#define __triangle_h

#include "AACell.h"
#include "Object.h"
#include "Ray.h"
#include "TypeDefs.h"
#include "Vectors.h"

#define BACKFACE_CULLING_ENABLED


struct Triangle : Object
{
	v3i v;	// vertex indices
	v3i vn;	// vertex normal indices
	v3i tc;	// texture coords indices
	v3f normal;

	int materialIndex;

	__device__ Triangle()
	{

	}

	__device__ Triangle(v3i& vertexIndices) : v(vertexIndices)
	{

	}

	__device__ real Hit(const Ray& ray, const array_of<v3f>& vertices) const
	{
		// Fast, minimum storage, ray triangle intersection
		// Tomas Moller, Ben Trumbore

		v3f v0 = vertices[v.x] + position;
		v3f v1 = vertices[v.y] + position;
		v3f v2 = vertices[v.z] + position;

		v3f edge1 = v1 - v0;
		v3f edge2 = v2 - v0;

		// begin calculating determinant - also used to calculate U parameter
		v3f pvec = vectors::Cross(ray.direction, edge2);

		// if determinant is near zero, ray lies in plane of triangle
		const real det = vectors::Dot(edge1, pvec);

#ifdef BACKFACE_CULLING_ENABLED
		if (det < EPSILON)
			return _INFINITY;

		// calculate distance from vert0 to ray origin
		v3f tvec = ray.origin - v0;

		// calculate U parameter and test bounds
		const real u = vectors::Dot(tvec, pvec);
		if (u < 0 || u > det)
			return _INFINITY;

		// prepare to test V parameter
		v3f qvec = vectors::Cross(tvec, edge1);

		// calculate V parameter and test bounds
		const real v = vectors::Dot(ray.direction, qvec);
		if (v < 0 || (u + v) > det)
			return _INFINITY;

		const real inv_det = 1 / det;

		//barycentric.u = u * inv_det;
		//barycentric.v = v * inv_det;

		// calculate t, scale parameters, ray intersects triangle
		return vectors::Dot(edge2, qvec) * inv_det;
#else
		if (det > -EPSILON && det < EPSILON)
			return _INFINITY;

		const real inv_det = 1 / det;

		// calculate distance from vert0 to ray origin
		v3f tvec = ray.origin - v0;

		// calculate U parameter and test bounds
		const real u = vectors::Dot(tvec, pvec) * inv_det;
		if (u < 0 || u > 1)
			return _INFINITY;

		// prepare to test V parameter
		v3f qvec = vectors::Cross(tvec, edge1);

		// calculate V parameter and test bounds
		const real v = vectors::Dot(ray.direction, qvec) * inv_det;
		if (v < 0 || (u + v) > 1)
			return _INFINITY;

		//barycentric.u = u;
		//barycentric.v = v;

		// calculate t, ray intersects triangle
		return vectors::Dot(edge2, qvec) * inv_det;
#endif
	}

	__device__ bool Collide(const Ray& ray, const array_of<v3f>& vertices, real from = EPSILON, real to = _INFINITY) const
	{
		v3f v0 = vertices[v.x] + position;
		v3f v1 = vertices[v.y] + position;
		v3f v2 = vertices[v.z] + position;

		v3f edge1 = v1 - v0;
		v3f edge2 = v2 - v0;

		// begin calculating determinant - also used to calculate U parameter
		v3f pvec = vectors::Cross(ray.direction, edge2);

		// if determinant is near zero, ray lies in plane of triangle
		const real det = vectors::Dot(edge1, pvec);
		if (ABS(det) < EPSILON)
			return false;

		// calculate distance from vert0 to ray origin
		v3f tvec = ray.origin - v0;

		// calculate U parameter and test bounds
		const real u = vectors::Dot(tvec, pvec);
		if (u < 0 || u > det)
			return false;

		// prepare to test V parameter
		v3f qvec = vectors::Cross(tvec, edge1);

		// calculate V parameter and test bounds
		const real v = vectors::Dot(ray.direction, qvec);
		if (v < 0 || (u + v) > det)
			return false;

		// calculate t, scale parameters, ray intersects triangle
		const real t = vectors::Dot(edge2, qvec) / det;
		if (t < from || t > to)
			return false;

		return true;
	}

	__device__ bool IsInside(const v3f& point, const array_of<v3f>& vertices) const
	{
		v3f tmpPoint = vertices[v.x] + position;
		return vectors::Dot(normal, point) - vectors::Dot(normal, tmpPoint) < 0;
	}

	__device__ void GetNormalAt(const v3f& point, const array_of<v3f>& vertices, v3f& normal) const
	{
		// TODO compute vertex normal at hit point
		normal = this->normal;
	}

	__device__ void UpdateNormal(const array_of<v3f>& vertices)
	{
		normal = vectors::Cross(vertices[v.x] - vertices[v.y], vertices[v.z] - vertices[v.y]);
		vectors::Normalize(normal);
	}
};

#endif __triangle_h
