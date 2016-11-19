#ifndef __scene_h
#define __scene_h

#include "Array.h"
#include "Animations.h"
#include "BoundingIntervalHierarchy.h"
#include "List.h"
#include "Objects.h"
#include "Octree.h"
#include "Materials.h"
#include <thread>
#include "TypeDefs.h"
#include "Vectors.h"

#define OCTREE_DEFAULT_MAX_DEPTH			4
#define BIH_DEFAULT_MAX_OBJECTS_PER_LEAF	4
#define BIH_DEFAULT_MAX_DEPTH				16

DLL_EXPORT_ARRAY_OF(v3f);
DLL_EXPORT_ARRAY_OF(char);
DLL_EXPORT_ARRAY_OF(RotateAround);
DLL_EXPORT_LIST_OF(RotateAround);

class BIH;
class MemoryManager;
struct Objects;
class Octree;
struct AthenaStorage;

class DLL_EXPORT Scene
{
public:

	Scene();
	Scene(const char* name, MemoryManager* memoryManagerInstance);
	~Scene();

	void Create(const char* name, MemoryManager* memoryManagerInstance);
	void Destroy();
	bool Update(real timeElapsed, AthenaStorage* athenaStorage,
		ui32 octreeDepth = OCTREE_DEFAULT_MAX_DEPTH,
		ui32 bihDepth = BIH_DEFAULT_MAX_DEPTH,
		ui32 bihMaxObjects = BIH_DEFAULT_MAX_OBJECTS_PER_LEAF);

	uint AddBox(const v3f& position, const v3f& size, ui32 materialId = 0);
	uint AddPlane(const v3f& position, const v3f& normal, ui32 materialId = 0);
	uint AddSphere(const v3f& position, real radius, ui32 materialId = 0);
	uint AddPointLightSource(const v3f& position, real intensity, const v3f& color);
	uint AddSphereLightSource(const v3f& position, real radius, real intensity, const v3f& color);
	uint AddBoxLightSource(const v3f& position, const v3f& size, real intensity, const v3f& color);
	uint AddMesh(Mesh& mesh);
	uint AddMesh(v3f position, const char* meshFileName);
	ui32 AddMaterial(const v3f& diffuseColor, const v3f& specularColor,
		real shininess = 0, real reflection = 0, real refraction = 0, real refractionIndex = 0);
	uint AddRotationAroundAnimation(v3f* point, v3f center, v3f axis, real speed);

	inline const BIH* GetBIH() const { return &bih; }
	inline const Octree* GetOctree() const { return &octree; }
	inline const Objects& GetObjects() const { return sceneObjects; }
	inline const array_of<v3f>& GetRandomDirections() const { return randomDirections; }

private:

	void AddObjectId(ObjectId objectId);
	uint AddBox(real x, real y, real z, real width, real height, real depth, ui32 materialId = 0);
	uint AddPlane(real x, real y, real z, real normalx, real normaly, real normalz, ui32 materialId = 0);
	uint AddSphere(real x, real y, real z, real radius, ui32 materialId = 0);
	uint AddPointLightSource(real x, real y, real z, real intensity, real r, real g, real b);
	uint AddSphereLightSource(real x, real y, real z, real radius, real intensity, real r, real g, real b);
	uint AddBoxLightSource(real x, real y, real z, real width, real height, real depth, real intensity, 
		real r, real g, real b);
	ui32 AddMaterial(real diffuseR, real diffuseG, real diffuseB,
		real specularR = 0, real specularG = 0, real specularB = 0,
		real shininess = 0, real reflection = 0, real refraction = 0, real refractionIndex = 0);
	
private:

	String name;
	MemoryManager* memoryManagerInstance;

	Objects sceneObjects;
	list_of<RotateAround> rotateAroundAnimations;

	BIH bih;
	Octree octree;

	array_of<v3f> randomDirections;
};

#endif __scene_h
