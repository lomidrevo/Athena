#include "Athena.h"
#include "BoundingIntervalHierarchy.h"
#include "HitResult.h"
#include "Octree.h"
#include "Objects.h"
#include "Random.h"
#include "Ray.h"
#include "RayTracing.h"
#include "Rendering.h"


__device__ real AmbientOcclusion(const Objects& objects, const RenderingParameters& parameters, const v3f& point, 
	const v3f& normal, const array_of<v3f>& randomDirections, const BIH* bih, const Octree* octree);
__device__ v3f EvaluatePointLightSources(const Objects& objects, const RenderingParameters& parameters, 
	const HitResult& hit, const Ray& ray, const BIH* bih, const Octree* octree);
__device__ v3f EvaluateAreaLightSources(const Objects& objects, const RenderingParameters& parameters, 
	const HitResult& hit, const Ray& ray, const array_of<v3f>& randomDirections, const BIH* bih, const Octree* octree);
__device__ HitResult RayTraceObjects(const Objects& objects, const RenderingParameters& parameters, const Ray& ray,
	const BIH* bih, const Octree* octree);
__device__ bool CollideWithObjects(const Objects& objects, const RenderingParameters& parameters, const Ray& ray,
	const BIH* bih, const Octree* octree, real from = 0, real to = _INFINITY, const ObjectId* objectIdToSkip = null);
__device__ void FillObjectHitResult(const Objects& objects, const Ray& ray, HitResult& hit);
template <typename T> 
__device__ void FillObjectHitResult(const T& object, const Ray& ray, HitResult& hit);
template <> 
__device__ void FillObjectHitResult<Mesh>(const Mesh& object, const Ray& ray, HitResult& hit);


__device__ RayTraceResult RayTrace(const Objects& objects, const RenderingParameters& parameters, const Ray& ray, 
	const array_of<v3f>& randomDirections, const BIH* bih, const Octree* octree, ui32 depth)
{
	RayTraceResult result;
	if (depth < parameters.maxRayTracingDepth)
	{
		HitResult hit = RayTraceObjects(objects, parameters, ray, bih, octree);

		result.objectId = hit.objectId;
		result.distance = hit.distance;
		result.testCount = hit.nodeTestCount + hit.intersectionCount;

		if (hit.objectId.Type() == ObjectType::Unknown)
		{
			result.normal = ray.direction;
			vectors::Abs(result.normal);
		}
		else if (hit.objectId.IsLight())
		{
			switch (hit.objectId.Type())
			{
				case ObjectType::SphereLightSource:
					result.color = objects.sphereLights[hit.objectId.index].color;
				break;

				case ObjectType::BoxLightSource:
					result.color = objects.boxLights[hit.objectId.index].color;
					break;

				case ObjectType::PointLightSource:
				default:
					break;
			}
		}
		else
		{
			FillObjectHitResult(objects, ray, hit);
			result.normal = hit.normal;

			const Material& material = objects.materials[hit.materialIndex];

			// ambient occlusion
			const real ambientOcclusion = AmbientOcclusion(
				objects, 
				parameters, 
				hit.point, 
				hit.normal, 
				randomDirections,
				bih,
				octree);
			result.color = material.diffuseColor * v3f(1, 1, 1) * ambientOcclusion;

			// evalute point light sources
			result.color += EvaluatePointLightSources(objects, parameters, hit, ray, bih, octree);
			// evaluate area light sources
			result.color += EvaluateAreaLightSources(objects, parameters, hit, ray, randomDirections, bih, octree);

			// reflected ray
			if (material.reflection > EPSILON)
			{
				// compute reflected ray
				const Ray reflection(hit.point + hit.normal * EPSILON, vectors::GetReflection(hit.normal, ray.direction));
				
				const RayTraceResult reflectionResult = RayTrace(
					objects, 
					parameters, 
					reflection, 
					randomDirections, 
					bih,
					octree,
					++depth);

				result.color += reflectionResult.color * material.reflection;
				//result.bihNodeCount += reflectionResult.bihNodeCount;
				result.distance += reflectionResult.distance;
			}

			// refracted ray
			if (material.refraction > EPSILON)
			{
				// compute refracted ray
				Ray refraction;
				refraction.direction = ray.direction;
				const real n1n2 = (hit.fromInside) ? material.refractionIndex : (real)1 / material.refractionIndex;
				if (vectors::GetRefraction(hit.normal, refraction.direction, n1n2))
					refraction.origin = hit.point + refraction.direction * EPSILON;
				else
					refraction.origin = ray.origin + ray.direction * EPSILON;
				refraction.Prepare();

				const RayTraceResult refractionResult = RayTrace(
					objects, 
					parameters, 
					refraction, 
					randomDirections,
					bih,
					octree,
					++depth);

				result.color += refractionResult.color * material.refraction;
				//result.bihNodeCount += refractionResult.bihNodeCount;
				result.distance += refractionResult.distance;
			}

			vectors::Clamp(0, 1, result.color);
		}
	}

	return result;
}

__device__ v3f EvaluatePointLightSources(const Objects& objects, const RenderingParameters& parameters, 
	const HitResult& hit, const Ray& ray, const BIH* bih, const Octree* octree)
{
	const Material& material = objects.materials[hit.materialIndex];

	// create light ray above surface
	Ray lightRay(hit.point + hit.normal * EPSILON);

	v3f resultColor;

	const ui32 lightCount = (ui32)objects.pointLights.currentCount;
	for (ui32 lightIndex = 0; lightIndex < lightCount; ++lightIndex)
	{
		const PointLightSource& light = objects.pointLights[lightIndex];

		lightRay.direction = vectors::Normalize(light.position - lightRay.origin);

		// if light is bellow surface
		const real lightAngle = vectors::Dot(hit.normal, lightRay.direction);
		if (lightAngle < EPSILON)
			continue;

		lightRay.Prepare();

		const real lightDistance = vectors::Distance(lightRay.origin, light.position);

		if (CollideWithObjects(objects, parameters, lightRay, bih, octree, 0, lightDistance, &hit.objectId))
			continue;

		// diffuse
		const real lightShading = light.intensity / (lightDistance * lightDistance);
		resultColor += material.diffuseColor * light.color * lightShading * lightAngle;

		if (material.shininess > EPSILON)
		{
			// specular
			const v3f reflection = vectors::GetReflection(hit.normal, lightRay.direction);
			const real reflectionEyeAngle = vectors::Dot(reflection, ray.direction);
			if (reflectionEyeAngle > EPSILON)
				resultColor += material.specularColor * light.color * pow(reflectionEyeAngle, material.shininess);
		}
	}

	return resultColor;
}

__device__ v3f EvaluateAreaLightSources(const Objects& objects, const RenderingParameters& parameters, 
	const HitResult& hit, const Ray& ray, const array_of<v3f>& randomDirections, const BIH* bih, const Octree* octree)
{
	const Material& material = objects.materials[hit.materialIndex];

	// create light ray above surface
	Ray lightRay(hit.point + hit.normal * EPSILON);

	v3f resultColor;

	const ui32 sphereLightCount = (ui32)objects.sphereLights.currentCount;
	for (ui32 lightIndex = 0; lightIndex < sphereLightCount; ++lightIndex)
	{
		const SphereLightSource& light = objects.sphereLights[lightIndex];
		const ui32 seed = randomDirections.count > (light.lightPointCount + 1) ? 
			iRND(0, randomDirections.count - light.lightPointCount - 1) : 0;
	
		const ui32 lightPointCount = MIN2(light.lightPointCount, MAX2((ui32)randomDirections.count, 1));
		for (ui32 lightPointIndex = 0; lightPointIndex < lightPointCount; lightPointIndex++)
		{
			const v3f lightPointPosition = light.position + (randomDirections.count ?
				v3f(randomDirections[lightPointIndex + seed]) * light.radius : v3f());

			// TODO spojit zvysok jednej funkcie PhongShading(hit, light, lightPosition)

			lightRay.direction = vectors::Normalize(lightPointPosition - lightRay.origin);

			// if light is bellow surface
			const real lightAngle = vectors::Dot(hit.normal, lightRay.direction);
			if (lightAngle < EPSILON)
				continue;

			lightRay.Prepare();

			const real lightDistance = vectors::Distance(lightRay.origin, lightPointPosition);

			if (CollideWithObjects(objects, parameters, lightRay, bih, octree, 0, lightDistance, &hit.objectId))
				continue;

			// diffuse
			const real lightShading = (light.intensity / light.lightPointCount) / (lightDistance * lightDistance);
			resultColor += material.diffuseColor * light.color * lightShading * lightAngle;

			if (material.shininess > EPSILON)
			{
				// specular
				const v3f reflection = vectors::GetReflection(hit.normal, lightRay.direction);
				const real reflectionEyeAngle = vectors::Dot(reflection, ray.direction);
				if (reflectionEyeAngle > EPSILON)
					resultColor += material.specularColor * (light.color / (real)light.lightPointCount) * 
						pow(reflectionEyeAngle, material.shininess);
			}
		}
	}

	const ui32 boxLightCount = (ui32)objects.boxLights.currentCount;
	for (ui32 lightIndex = 0; lightIndex < boxLightCount; ++lightIndex)
	{
		const BoxLightSource& light = objects.boxLights[lightIndex];

		// TODO EvaluateAreaLightSources, BoxLightSource

		//const int seed = iRND(0, randomDirections.count - light.lightPointCount - 1);

		//for (ui32 lightPointIndex = 0; lightPointIndex < light.lightPointCount; lightPointIndex++)
		//{
		//	const v3f lightPointPosition = light.position + v3f(randomDirections[lightPointIndex + seed]) * light.radius;

		//	lightRay.direction = vectors::Normalize(lightPointPosition - lightRay.origin);

		//	// if light is bellow surface
		//	const real lightAngle = vectors::Dot(hit.normal, lightRay.direction);
		//	if (lightAngle < EPSILON)
		//		continue;

		//	lightRay.Prepare();

		//	const real lightDistance = vectors::Distance(lightRay.origin, lightPointPosition);

		//	if (CollideWithScene(scene, parameters, lightRay, 0, lightDistance, &hit.objectId))
		//		continue;

		//	// diffuse
		//	const real lightShading = (light.intensity / light.lightPointCount) / (lightDistance * lightDistance);
		//	resultColor += material.diffuseColor * light.color * lightShading * lightAngle;

		//	if (material.shininess > EPSILON)
		//	{
		//		// specular
		//		const v3f reflection = vectors::GetReflection(hit.normal, lightRay.direction);
		//		const real reflectionEyeAngle = vectors::Dot(reflection, ray.direction);
		//		if (reflectionEyeAngle > EPSILON)
		//			resultColor += material.specularColor * (light.color / (real)light.lightPointCount) *
		//			pow(reflectionEyeAngle, material.shininess);
		//	}
		//}
	}
	
	return resultColor;
}

__device__ real AmbientOcclusion(const Objects& objects, const RenderingParameters& parameters, const v3f& point,
	const v3f& normal, const array_of<v3f>& randomDirections, const BIH* bih, const Octree* octree)
{
	if (!parameters.ambientOcclusionSamples || randomDirections.count == 0)
		return .1;

	Ray sampleRay(point + normal * EPSILON);

	//const int offset = 0;
	const int seed = iRND(0, randomDirections.count - parameters.ambientOcclusionSamples - 1);

	int result = 0;
	for (ui32 i = 0; i < parameters.ambientOcclusionSamples; ++i)
	{
		sampleRay.direction = randomDirections[i + seed];
		if (vectors::Dot(sampleRay.direction, normal) < 0)
			vectors::Inv(sampleRay.direction);
		sampleRay.Prepare();

		if (!CollideWithObjects(objects, parameters, sampleRay, bih, octree, 0, _INFINITY, null))
			result++;
	}

	return (real)result / parameters.ambientOcclusionSamples * parameters.ambientOcclusionModifier;
}

__device__ HitResult RayTraceObjects(const Objects& objects, const RenderingParameters& parameters, const Ray& ray,
	const BIH* bih, const Octree* octree)
{
	HitResult hit;

	// first trace planes, since they go to infinity (and are not part of any acceleration structure)
	const list_of<Plane>& planes = objects.planes;
	for (int i = 0; i < planes.currentCount; ++i)
	{
		auto t = planes[i].Hit(ray);
		if (t > EPSILON && t < hit.distance)
		{
			hit.distance = t;
			hit.objectId = planes[i].id;
		}
	}

	switch (parameters.tracingMethod)
	{
		case TracingMethod::StraightForward:
		{
			ObjectId innerObjectId;

			const uint objectCount = objects.everything.currentCount;
			for (uint i = 0; i < objectCount; ++i)
			{
				real t = _INFINITY;
				const ObjectId& objectId = objects.everything[i];
				switch (objectId.Type())
				{
					case ObjectType::Sphere: t = objects.spheres[objectId.index].Hit(ray); break;
					case ObjectType::Box: t = objects.boxes[objectId.index].Hit(ray); break;
					case ObjectType::SphereLightSource: t = objects.sphereLights[objectId.index].Hit(ray); break;
					case ObjectType::BoxLightSource: t = objects.boxLights[objectId.index].Hit(ray); break;
					case ObjectType::Mesh: t = objects.meshes[objectId.index].Hit(ray, innerObjectId); break;

					case ObjectType::Plane:
					case ObjectType::PointLightSource:
					default:
						t = _INFINITY;
				}

				if (t > EPSILON && t < hit.distance)
				{
					hit.distance = t;
					hit.objectId = objectId;
					if (objectId.Type() == ObjectType::Mesh)
						hit.innerObjectId = innerObjectId;
				}
			}

			hit.intersectionCount = objectCount;
		}
		break;

		case TracingMethod::BoundingIntervalHierarchy:
			if (bih) 
				bih->Hit(ray, hit);
			break;

		case TracingMethod::Octree:
			if (octree)
				octree->Hit(ray, hit);
			break;
	}

	return hit;
}

__device__ bool CollideWithObjects(const Objects& objects, const RenderingParameters& parameters, const Ray& ray,
	const BIH* bih, const Octree* octree, real from, real to, const ObjectId* objectIdToSkip)
{
	// planes are not part of scene tree
	const list_of<Plane>& planes = objects.planes;
	for (int i = 0; i < planes.currentCount; ++i)
	{
		if (objectIdToSkip && planes[i].id._value == objectIdToSkip->_value)
			continue;

		if (planes[i].Collide(ray, from, to))
			return true;
	}

	switch (parameters.tracingMethod)
	{
		case TracingMethod::Octree:
			if (octree)
				return octree->Collide(ray, from, to);
			break;
		
		case TracingMethod::BoundingIntervalHierarchy:
			if (bih)
				return bih->Collide(ray, from, to, objectIdToSkip);
			break;
	}

	// TracingMethod::StraightForward
	bool collision = false;
	const uint objectCount = objects.everything.currentCount;
	for (uint i = 0; i < objectCount; ++i)
	{
		const ObjectId& objectId = objects.everything[i];
		if ((objectId.Type() != ObjectType::Mesh) && 
			(objectId.IsLight() || (objectIdToSkip != null && objectId._value == objectIdToSkip->_value)))
			continue;

		switch (objectId.Type())
		{
			case ObjectType::Sphere:
				collision = objects.spheres[objectId.index].Collide(ray, from, to);
				break;
			case ObjectType::Box:
				collision = objects.boxes[objectId.index].Collide(ray, from, to);
				break;
			case ObjectType::Mesh:
				collision = objects.meshes[objectId.index].Collide(ray, from, to);
				break;
			case ObjectType::SphereLightSource:
				collision = objects.sphereLights[objectId.index].Collide(ray, from, to);
				break;
			case ObjectType::BoxLightSource:
				collision = objects.boxLights[objectId.index].Collide(ray, from, to);
				break;

			case ObjectType::Plane:
			case ObjectType::PointLightSource:
			default:
				break;
		}

		if (collision)
			return true;
	}

	return false;
}

__device__ void FillObjectHitResult(const Objects& objects, const Ray& ray, HitResult& hit)
{
	switch (hit.objectId.Type())
	{
		case ObjectType::Box:
			FillObjectHitResult<Box>(objects.boxes[hit.objectId.index], ray, hit);
			break;

		case ObjectType::Sphere:
			FillObjectHitResult<Sphere>(objects.spheres[hit.objectId.index], ray, hit);
			break;

		case ObjectType::Mesh:
			FillObjectHitResult<Mesh>(objects.meshes[hit.objectId.index], ray, hit);
			break;

		case ObjectType::Plane:
			FillObjectHitResult<Plane>(objects.planes[hit.objectId.index], ray, hit);
			break;

		case ObjectType::Voxel:
			hit.materialIndex = hit.objectId.index;
			break;
	}
}

template <typename T>
__device__ void FillObjectHitResult(const T& object, const Ray& ray, HitResult& hit)
{
	// compute hit point
	hit.point = ray.origin + ray.direction * hit.distance;
	object.GetNormalAt(hit.point, hit.normal);
	hit.fromInside = object.IsInside(ray.origin);
	hit.materialIndex = object.materialIndex;

	if (hit.fromInside)
		vectors::Inv(hit.normal);
}

template <>
__device__ void FillObjectHitResult<Mesh>(const Mesh& object, const Ray& ray, HitResult& hit)
{
	// compute hit point
	hit.point = ray.origin + ray.direction * hit.distance;
	object.GetNormalAt(hit.point, hit.normal, &hit.innerObjectId);
	hit.fromInside = object.IsInside(ray.origin, &hit.innerObjectId);

	if (hit.fromInside)
		vectors::Inv(hit.normal);
}
