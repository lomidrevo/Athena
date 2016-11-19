#ifndef __object_h
#define __object_h

#include "AACell.h"
#include "TypeDefs.h"
#include "Vectors.h"

#define OBJECT_TYPE_VALUES(_) \
    _(Unknown,=0) \
    _(Sphere,) \
    _(Box,) \
    _(Mesh,) \
    _(Triangle,) \
	_(Plane,) \
	_(Voxel,) \
    _(Light,) \
    _(PointLightSource,) \
    _(SphereLightSource,) \
    _(BoxLightSource,)
DECLARE_ENUM(ObjectType, OBJECT_TYPE_VALUES)
#undef OBJECT_TYPE_VALUES


struct ObjectId
{
	union
	{
		ui64 _value;
		struct
		{
			ui64 index		: 32;
			ui64 type		: 16;
			ui64 reserved	: 16;
		};
	};

	__device__ void Clear() { _value = 0; }

	__device__ const ObjectId& Set(ObjectType::Enum type, uint index) 
	{ 
		this->type = (ui32)type;
		this->index = (ui32)index;
		return *this;
	}

	__device__ inline ObjectType::Enum Type() const { return (ObjectType::Enum)type; }
	__device__ inline bool IsLight() const { return Type() > ObjectType::Light; }
};

struct Ray;

struct Object
{
	v3f position;
	ObjectId id;
	
	//__device__ real DistanceFrom(const v3f& point) const { return _INFINITY; }
	//__device__ real Hit(const Ray& ray) const { return _INFINITY; }
	//__device__ bool Collide(const Ray& ray, real from = EPSILON, real to = _INFINITY) const { return false; }
	//__device__ bool IsInside(const v3f& point) const { return false; }
	//__device__ void GetNormalAt(const v3f& point, v3f& normal) const { }
	//__device__ bool AndAACell(const AACell& cell) const { return false; }
	//__device__ void UpdateCell() { }
};

#endif __object_h
