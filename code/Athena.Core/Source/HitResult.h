#ifndef __hit_result_h
#define __hit_result_h

#include "Object.h"
#include "TypeDefs.h"
#include "Vectors.h"


struct HitResult
{
	v3f point;
	v3f normal;

	ObjectId objectId;
	ObjectId innerObjectId;

	real distance;
	int materialIndex;

	// debugging
	uint nodeTestCount;
	uint intersectionCount;

	bool fromInside;

	__device__ HitResult()
	{
		Clear();
	}

	__device__ inline void Clear()
	{
		point.Set(0, 0, 0);
		normal.Set(0, 0, 0);
		objectId.Clear();
		innerObjectId.Clear();
		distance = _INFINITY;
		materialIndex = 0;
		fromInside = false;

		nodeTestCount = 0;
		intersectionCount = 0;
	}
};

#endif __hit_result_h
