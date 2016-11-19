#include "Athena.h"
#include "Objects.h"
#include "Ray.h"
#include "RayMarching.h"
#include "Rendering.h"
#include "Scene.h"


RayMarchResult RayMarch(const Scene* scene, const Ray& ray, const RenderingParameters& parameters);
v3f GetNormalAt(const Scene* scene, v3f point);
real GetDistanceFromObjects(const Scene* scene, v3f p);
real GetAmbienOcclusionAt(const Scene* scene, v3f point, v3f normal);
real GetSoftShadowAt(const Scene* scene, v3f point, v3f lightDirection, real lightDistance,
	real strength = 8);


RayMarchResult RayMarch(const Scene* scene, const Ray& ray, const RenderingParameters& parameters)
{
	RayMarchResult result;

	Ray lightRay;

	v3f point;
	v3f colorWithLight(0, 0, 0);

	real t = 0;
	real prevDistance = _INFINITY;
	uint32 lightCount = (uint32)scene->GetObjects().pointLights.currentCount;

	auto rayMarchingStep = 0;
	//if (scene->GetBIH()->Collide(ray))
	{
		for (result.rayMarchingStep = 0; rayMarchingStep < RAY_MARCHING_STEPS; ++rayMarchingStep)
		{
			point = ray.origin + ray.direction * t;

			auto distance = GetDistanceFromObjects(scene, point);
			if (distance < EPSILON)
			{
				v3f color(1, 1, 1);

				// compute normal (as gradient)
				result.normal = GetNormalAt(scene, point);

				// create shadow ray above surface
				lightRay.origin = point + result.normal * EPSILON;

				colorWithLight.Set(0, 0, 0);
				for (uint lightIndex = 0; lightIndex < lightCount; ++lightIndex)
				{
					auto light = scene->GetObjects().pointLights[lightIndex];

					// set light ray direction
					lightRay.direction = light.position - lightRay.origin;
					vectors::Normalize(lightRay.direction);

					// if light is bellow surface
					const real angle = vectors::Dot(result.normal, lightRay.direction);
					if (angle < EPSILON)
						continue;

					const real lightDistance = vectors::Distance(lightRay.origin, light.position);

					// calculate soft shadow
					const real softShadow = GetSoftShadowAt(scene,
						lightRay.origin, lightRay.direction, lightDistance);

					// calculate ambient occlusion
					const real ambientOcclusion = GetAmbienOcclusionAt(scene, lightRay.origin, result.normal);

					// occluder is between light source and hit point
					if (softShadow < EPSILON)
						continue;

					// zatial takto
					const real lightShading = 
						light.intensity 
						* angle 
						* softShadow 
						* ambientOcclusion /
						(lightDistance/* * lightDistance*/);

					colorWithLight += color * light.color * lightShading;
				}

				break;
			}

			if (distance == _INFINITY)
				break;

			prevDistance = distance;
			t += distance;
		}
	}
	//else
	//{
	//	result.normal = ray.direction;
	//	result.normal.Abs();
	//}

	return result;
}

v3f GetNormalAt(const Scene* scene, v3f point)
{
	v3f normal;

	const real epsilon = 0.01f; // small offset

	v3f p1, p2;
	p1.Set(point.x + epsilon, point.y, point.z);
	p2.Set(point.x - epsilon, point.y, point.z);
	normal.x = GetDistanceFromObjects(scene, p1) - GetDistanceFromObjects(scene, p2);

	p1.Set(point.x, point.y + epsilon, point.z);
	p2.Set(point.x, point.y - epsilon, point.z);
	normal.y = GetDistanceFromObjects(scene, p1) - GetDistanceFromObjects(scene, p2);

	p1.Set(point.x, point.y, point.z + epsilon);
	p2.Set(point.x, point.y, point.z - epsilon);
	normal.z = GetDistanceFromObjects(scene, p1) - GetDistanceFromObjects(scene, p2);

	vectors::Normalize(normal);

	return normal;
}

real GetSoftShadowAt(const Scene* scene, v3f point, v3f lightDirection, real lightDistance, real strength)
{
	real t = 0;
	real softShadow = 1;
	v3f p1;

	while (t < lightDistance)
	{
		p1 = point + lightDirection * t;

		const real occluderDistance = GetDistanceFromObjects(scene, p1);
		if (occluderDistance < EPSILON)
			break;

		const real shadowStrength = strength * occluderDistance / t;
		softShadow = MIN2(softShadow, shadowStrength);

		t += occluderDistance;
	}

	return softShadow;
}

real GetAmbienOcclusionAt(const Scene* scene, v3f point, v3f normal)
{
	// TODO ambient occlusion
	return 1;
}

real GetDistanceFromObjects(const Scene* scene, v3f point)
{
	real minDistance = _INFINITY;
	auto objects = scene->GetObjects();

	auto sphereCount = objects.spheres.currentCount;
	for (int i = 0; i < sphereCount; ++i)
	{
		auto distance = objects.spheres[i].DistanceFrom(point);
		if (distance < minDistance)
			minDistance = distance;
	}

	auto boxCount = objects.boxes.currentCount;
	for (int i = 0; i < boxCount; ++i)
	{
		auto distance = objects.boxes[i].DistanceFrom(point);
		if (distance < minDistance)
			minDistance = distance;
	}

	auto planeCount = objects.planes.currentCount;
	for (int i = 0; i < planeCount; ++i)
	{
		auto distance = objects.planes[i].DistanceFrom(point);
		if (distance < minDistance)
			minDistance = distance;
	}

	return minDistance;
}
