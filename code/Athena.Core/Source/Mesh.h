#ifndef __mesh_h
#define __mesh_h

#include "AACell.h"
#include "Array.h"
#include "Materials.h"
#include "Object.h"
#include "Ray.h"
#include "Triangle.h"
#include "TypeDefs.h"
#include "Vectors.h"


struct Mesh : Object
{
	AACell cell;
	array_of<v3f> vertices;
	array_of<v3f> vertexNormals;
	array_of<v2f> textureCoords;
	array_of<Triangle> triangles;
	array_of<Material> materials;
	b32 dynamic;

	Mesh(b32 dynamic = false) : dynamic(dynamic)
	{

	}

	__device__ real Hit(const Ray& ray, ObjectId& triangleId) const
	{
		Ray localOcclusionRay(ray);
		localOcclusionRay.origin = position - ray.origin;
		if (!cell.Collide(localOcclusionRay))
			return _INFINITY;
		
		Ray localRay(ray);
		localRay.origin = ray.origin - position;

		real minDistance = _INFINITY;
		for (uint i = 0; i < triangles.count; i++)
		{
			real distance = triangles[i].Hit(localRay, vertices);
			if (distance < minDistance)
			{
				minDistance = distance;
				triangleId.Set(ObjectType::Triangle, i);
			}
		}

		return minDistance;
	}
	
	__device__ bool IsInside(const v3f& point, const ObjectId* triangleId = null) const
	{
		return triangles[triangleId->index].IsInside(point, vertices);
	}

	__device__ void GetNormalAt(const v3f& point, v3f& normal, const ObjectId* triangleId = null) const
	{
		triangles[triangleId->index].GetNormalAt(point, vertices, normal);
	}

	__device__ bool Collide(const Ray& ray, real from = EPSILON, real to = _INFINITY) const
	{
		Ray localRay(ray);
		localRay.origin = position - ray.origin;

		if (!cell.Collide(localRay, from, to))
			return false;

		Ray localRay2(ray);
		localRay2.origin = ray.origin - position;

		for (uint i = 0; i < triangles.count; i++)
			if (triangles[i].Collide(localRay2, vertices, from, to))
				return true;

		return false;
	}

	__device__ void Update(bool updateStatic = false)
	{
		if (dynamic || updateStatic)
		{
			UpdateCell();
			for (uint i = 0; i < triangles.count; ++i)
				triangles[i].UpdateNormal(vertices);
		}
	}

	__device__ void UpdateCell()
	{
		cell.minCorner.Set(_INFINITY, _INFINITY, _INFINITY);
		cell.maxCorner.Set(-_INFINITY, -_INFINITY, -_INFINITY);
		for (uint i = 0; i < vertices.count; i++)
		{
			if (vertices[i].x < cell.minCorner.x)
				cell.minCorner.x = vertices[i].x;
			if (vertices[i].y < cell.minCorner.y)
				cell.minCorner.y = vertices[i].y;
			if (vertices[i].z < cell.minCorner.z)
				cell.minCorner.z = vertices[i].z;

			if (vertices[i].x > cell.maxCorner.x)
				cell.maxCorner.x = vertices[i].x;
			if (vertices[i].y > cell.maxCorner.y)
				cell.maxCorner.y = vertices[i].y;
			if (vertices[i].z > cell.maxCorner.z)
				cell.maxCorner.z = vertices[i].z;
		}
	}
};

class MemoryManager;

bool LoadWavefrontObjectFromFile(const char* filename, MemoryManager* memoryManagerInstance, Mesh& mesh);

#endif __mesh_h
