#ifndef __random_h
#define __random_h

#include <stdlib.h>
#include "TypeDefs.h"

#define fRND(from, to)	(((real)rand()/(real(RAND_MAX) + 1)) * ((to) - (from)) + (from))
#define iRND(from, to)	(rand() % (((to) + 1) - (from)) + (from))

#endif __random_h
