#ifndef __objects_h
#define __objects_h

#include "AACell.h"
#include "Array.h"
#include "Box.h"
#include "BoxLightSource.h"
#include "Light.h"
#include "List.h"
#include "Mesh.h"
#include "Object.h"
#include "Plane.h"
#include "PointLightSource.h"
#include "Sphere.h"
#include "SphereLightSource.h"
#include "Triangle.h"


struct Objects
{
	ui32 counts[ObjectType::Count];

	list_of<ObjectId> everything;

	list_of<Box> boxes;
	list_of<Plane> planes;
	list_of<Sphere> spheres;
	list_of<Mesh> meshes;
	list_of<PointLightSource> pointLights;
	list_of<SphereLightSource> sphereLights;
	list_of<BoxLightSource> boxLights;
	list_of<Material> materials;
};

#endif __objects_h
