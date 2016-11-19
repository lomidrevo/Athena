#ifndef __light_h
#define __light_h

#include "Object.h"
#include "TypeDefs.h"
#include "Vectors.h"


struct Light : Object
{
	v3f color;
	real intensity;
};

#endif __light_h
