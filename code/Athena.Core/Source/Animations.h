#ifndef __animations_h
#define __animations_h

#include "TypeDefs.h"
#include "Vectors.h"

struct Animation
{
	real speed;
};

struct RotateAround : Animation
{
	v3f localCenter, axis;
	v3f* center;
	v3f* point;
};

#endif __animations_h
