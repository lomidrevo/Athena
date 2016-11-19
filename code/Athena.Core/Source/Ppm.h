
#ifndef __ppm_h
#define __ppm_h

#include "Array.h"
#include "Convert.h"
#include <stdio.h>
#include "TypeDefs.h"
#include "Vectors.h"


void SavePPMImage(const char* filename, const v2ui& imageSize, const array_of<ui32>& imageData)
{
	FILE *f = fopen(filename, "w");
	fprintf(f, "P3\n%d %d\n%d\n", imageSize.x, imageSize.y, 255);

	for (int i = 0; i < imageData.count; i++)
	{
		const rgba_as_uint32 color(imageData[i]);
		fprintf(f, "%d %d %d ", color.r, color.g, color.b);
	}

	fclose(f);
}

#endif __ppm_h
