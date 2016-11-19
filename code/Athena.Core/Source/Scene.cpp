#include "Athena.h"
#include "Log.h"
#include "MemoryManager.h"
#include "Scene.h"


Scene::Scene()
{
}

Scene::Scene(const char* name, MemoryManager* memoryManagerInstance)
{
	Create(name, memoryManagerInstance);
}

Scene::~Scene()
{
	Destroy();
}

void Scene::Destroy()
{
	if (memoryManagerInstance)
	{
		bih.Destroy();
		octree.Destroy(memoryManagerInstance);

		sceneObjects.everything.Destroy();
		sceneObjects.boxes.Destroy();
		sceneObjects.planes.Destroy();
		sceneObjects.spheres.Destroy();
		sceneObjects.pointLights.Destroy();
		sceneObjects.sphereLights.Destroy();
		sceneObjects.boxLights.Destroy();
		sceneObjects.materials.Destroy();

		for (uint i = 0; i < sceneObjects.meshes.currentCount; ++i)
		{
			_MEM_FREE_ARRAY(memoryManagerInstance, v3f, &sceneObjects.meshes[i].vertices);
			_MEM_FREE_ARRAY(memoryManagerInstance, v3f, &sceneObjects.meshes[i].vertexNormals);
			_MEM_FREE_ARRAY(memoryManagerInstance, v2f, &sceneObjects.meshes[i].textureCoords);
			_MEM_FREE_ARRAY(memoryManagerInstance, Triangle, &sceneObjects.meshes[i].triangles);
			_MEM_FREE_ARRAY(memoryManagerInstance, Material, &sceneObjects.meshes[i].materials);
		}
		sceneObjects.meshes.Destroy();

		rotateAroundAnimations.Destroy();

		_MEM_FREE_ARRAY(memoryManagerInstance, v3f, &randomDirections);

		LOG_DEBUG("Scene::Destroy [%s]", name.ptr);

		_MEM_FREE_ARRAY(memoryManagerInstance, char, &name);
		memoryManagerInstance = null;
	}
}

void Scene::Create(const char* name, MemoryManager* memoryManagerInstance)
{
	this->memoryManagerInstance = memoryManagerInstance;

	sceneObjects.everything.Initialize(memoryManagerInstance, "ObjectId");
	sceneObjects.boxes.Initialize(memoryManagerInstance, "Box");
	sceneObjects.planes.Initialize(memoryManagerInstance, "Plane");
	sceneObjects.spheres.Initialize(memoryManagerInstance, "Sphere");
	sceneObjects.pointLights.Initialize(memoryManagerInstance, "PointLightSource");
	sceneObjects.sphereLights.Initialize(memoryManagerInstance, "SphereLightSource");
	sceneObjects.boxLights.Initialize(memoryManagerInstance, "BoxLightSource");
	sceneObjects.meshes.Initialize(memoryManagerInstance, "Mesh");
	sceneObjects.materials.Initialize(memoryManagerInstance, "Material");
	rotateAroundAnimations.Initialize(memoryManagerInstance, "RotateAround");

	memset(sceneObjects.counts, 0, sizeof(*sceneObjects.counts) * ObjectType::Count);

	this->name = _MEM_ALLOC_STRING(memoryManagerInstance, name);

	bih.Initialize(&sceneObjects, memoryManagerInstance);
	octree.Initialize(&sceneObjects, memoryManagerInstance);

	randomDirections = _MEM_ALLOC_ARRAY(memoryManagerInstance, v3f, 1024);
	for (uint i = 0; i < randomDirections.count; ++i)
		vectors::RandomSpherePoint3f(randomDirections[i]);
}

bool Scene::Update(real timeElapsed, AthenaStorage* athenaStorage, ui32 octreeDepth, ui32 bihDepth, ui32 bihMaxObjects)
{
	bool changed = false;

	// update animations
	for (uint i = 0; i < rotateAroundAnimations.currentCount; i++)
	{
		RotateAround& animation = rotateAroundAnimations[i];
		vectors::RotatePointAround(*animation.center, animation.axis, timeElapsed * animation.speed, *animation.point);
	}
	if (rotateAroundAnimations.currentCount)
		changed = true;

	// update dynamic meshes
	if (changed)
		for (uint i = 0; i < sceneObjects.meshes.currentCount; ++i)
			sceneObjects.meshes[i].Update();

	//if (changed || !athenaStorage->frame.count)
	{
		// update scene acceleration structures
		
		octree.Update(
			athenaStorage->renderingParameters.multiThreadedOctreeUpdate ?
			athenaStorage->threads : array_of<std::thread>(),
			octreeDepth, athenaStorage);
	
		bih.Update(bihMaxObjects, bihDepth, athenaStorage);
	}

	// generate new random directions each frame
	for (uint i = 0; i < randomDirections.count; ++i)
		vectors::RandomSpherePoint3f(randomDirections[i]);

	return changed;
}

void Scene::AddObjectId(ObjectId objectId)
{
	sceneObjects.everything.Add(objectId);
}

uint Scene::AddBox(const v3f& position, const v3f& size, ui32 materialId)
{
	return AddBox(position.x, position.y, position.z, size.x, size.y, size.z, materialId);
}

uint Scene::AddBox(real x, real y, real z, real width, real height, real depth, ui32 materialId)
{
	Box newBox;
	newBox.position.Set(x, y, z);
	newBox.cell.minCorner.Set(-width/2, -height/2, -depth/2);
	newBox.cell.maxCorner.Set(width/2, height/2, depth/2);
	newBox.materialIndex = materialId;

	auto index = sceneObjects.boxes.Add(newBox);
	AddObjectId(sceneObjects.boxes[index].id.Set(ObjectType::Box, index));
	
	sceneObjects.counts[ObjectType::Box] = (uint32)sceneObjects.boxes.currentCount;
	return index;
}

uint Scene::AddPlane(const v3f& position, const v3f& normal, ui32 materialId)
{
	return AddPlane(position.x, position.y, position.z, normal.x, normal.y, normal.z, materialId);
}

uint Scene::AddPlane(real x, real y, real z, real normalx, real normaly, real normalz, ui32 materialId)
{
	Plane newPlane;
	newPlane.position.Set(x, y, z);
	newPlane.normal.Set(normalx, normaly, normalz);
	vectors::Normalize(newPlane.normal);
	newPlane.materialIndex = materialId;

	auto index = sceneObjects.planes.Add(newPlane);
	AddObjectId(sceneObjects.planes[index].id.Set(ObjectType::Plane, index));

	sceneObjects.counts[ObjectType::Plane] = (uint32)sceneObjects.planes.currentCount;
	return index;
}

uint Scene::AddSphere(const v3f& position, real radius, ui32 materialId)
{
	return AddSphere(position.x, position.y, position.z, radius, materialId);
}

uint Scene::AddSphere(real x, real y, real z, real radius, ui32 materialId)
{
	Sphere newSphere;
	newSphere.position.Set(x, y, z);
	newSphere.radius = radius;
	newSphere.materialIndex = materialId;
	newSphere.UpdateCell();

	auto index = sceneObjects.spheres.Add(newSphere);
	AddObjectId(sceneObjects.spheres[index].id.Set(ObjectType::Sphere, index));

	sceneObjects.counts[ObjectType::Sphere] = (uint32)sceneObjects.spheres.currentCount;
	return index;
}

uint Scene::AddPointLightSource(const v3f& position, real intensity, const v3f& color)
{
	return AddPointLightSource(position.x, position.y, position.z, intensity, color.x, color.y, color.z);
}

uint Scene::AddPointLightSource(real x, real y, real z, real intensity,
	real r, real g, real b)
{
	PointLightSource newLight;
	newLight.position.Set(x, y, z);
	newLight.intensity = intensity;
	newLight.color.Set(r, g, b);

	auto index = sceneObjects.pointLights.Add(newLight);
	AddObjectId(sceneObjects.pointLights[index].id.Set(ObjectType::PointLightSource, index));

	sceneObjects.counts[ObjectType::PointLightSource] = (uint32)sceneObjects.pointLights.currentCount;
	return index;
}

uint Scene::AddSphereLightSource(const v3f& position, real radius, real intensity, const v3f& color)
{
	return AddSphereLightSource(position.x, position.y, position.z, radius, intensity, color.x, color.y, color.z);
}

uint Scene::AddSphereLightSource(real x, real y, real z, real radius, real intensity,
	real r, real g, real b)
{
	SphereLightSource newLight;
	newLight.position.Set(x, y, z);
	newLight.radius = radius;
	newLight.intensity = intensity;
	newLight.color.Set(r, g, b);
	newLight.lightPointCount = 8;
	newLight.UpdateCell();

	auto index = sceneObjects.sphereLights.Add(newLight);
	AddObjectId(sceneObjects.sphereLights[index].id.Set(ObjectType::SphereLightSource, index));

	sceneObjects.counts[ObjectType::SphereLightSource] = (uint32)sceneObjects.sphereLights.currentCount;
	return index;
}

uint Scene::AddBoxLightSource(const v3f& position, const v3f& size, real intensity, const v3f& color)
{
	return AddBoxLightSource(position.x, position.y, position.z, size.x, size.y, size.z, intensity, 
		color.x, color.y, color.z);
}

uint Scene::AddBoxLightSource(real x, real y, real z, real width, real height, real depth, real intensity,
	real r, real g, real b)
{
	BoxLightSource newLight;
	newLight.position.Set(x, y, z);
	newLight.cell.minCorner.Set(-width/2, -height/2, -depth/2);
	newLight.cell.maxCorner.Set(width/2, height/2, depth/2);
	newLight.intensity = intensity;
	newLight.color.Set(r, g, b);
	
	auto index = sceneObjects.boxLights.Add(newLight);
	AddObjectId(sceneObjects.boxLights[index].id.Set(ObjectType::BoxLightSource, index));
	
	sceneObjects.counts[ObjectType::BoxLightSource] = (uint32)sceneObjects.boxLights.currentCount;
	return index;
}

uint Scene::AddRotationAroundAnimation(v3f* point, v3f center, v3f axis, real speed)
{
	RotateAround animation;
	animation.point = point;
	animation.localCenter = center;
	animation.center = &animation.localCenter;
	animation.axis = axis;
	animation.speed = speed;
	animation.point = point;

	return rotateAroundAnimations.Add(animation);
}

uint Scene::AddMesh(Mesh& mesh)
{
	mesh.Update(true);

	auto index = sceneObjects.meshes.Add(mesh);
	mesh.vertices.ptr = null;
	mesh.vertexNormals.ptr = null;
	mesh.textureCoords.ptr = null;
	mesh.triangles.ptr = null;
	mesh.materials.ptr = null;

	// TODO add mesh triangles to scene triangle list ?

	AddObjectId(sceneObjects.meshes[index].id.Set(ObjectType::Mesh, index));
	sceneObjects.counts[ObjectType::Mesh] = (uint32)sceneObjects.meshes.currentCount;
	return index;
}

uint Scene::AddMesh(v3f position, const char* meshFileName)
{
	Mesh newMesh;
	if (LoadWavefrontObjectFromFile(meshFileName, memoryManagerInstance, newMesh))
	{
		newMesh.position = position;
		newMesh.UpdateCell();

		auto index = sceneObjects.meshes.Add(newMesh);
		AddObjectId(sceneObjects.meshes[index].id.Set(ObjectType::Mesh, index));

		sceneObjects.counts[ObjectType::Mesh] = (uint32)sceneObjects.meshes.currentCount;
		return index;
	}
	
	return 0;
}

ui32 Scene::AddMaterial(const v3f& diffuseColor, const v3f& specularColor,
	real shininess, real reflection, real refraction, real refractionIndex)
{
	return AddMaterial(
		diffuseColor.x, diffuseColor.y, diffuseColor.z, 
		specularColor.x, specularColor.y, specularColor.z,
		shininess, reflection, refraction, refractionIndex);
}

ui32 Scene::AddMaterial(real diffuseR, real diffuseG, real diffuseB,
	real specularR, real specularG, real specularB,
	real shininess, real reflection, real refraction, real refractionIndex)
{
	Material newMaterial;
	newMaterial.diffuseColor.Set(diffuseR, diffuseG, diffuseB);
	newMaterial.specularColor.Set(specularR, specularG, specularB);
	newMaterial.reflection = reflection;
	newMaterial.refraction = refraction;
	newMaterial.refractionIndex = refractionIndex;
	newMaterial.shininess = shininess;
	
	return (ui32)sceneObjects.materials.Add(newMaterial);
}
