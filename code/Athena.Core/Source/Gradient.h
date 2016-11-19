#ifndef __gradient_h
#define __gradient_h

#include "List.h"
#include "MemoryManager.h"
#include "TypeDefs.h"
#include "Vectors.h"


//
// http://www.andrewnoske.com/wiki/Code_-_heatmaps_and_color_gradients
//
v3f GetHeatMapColor(real value)
{
	const int NUM_COLORS = 5;
	static real color[NUM_COLORS][3] = { { 0, 0, 1 }, { 0, 1, 1 }, { 0, 1, 0 }, { 1, 1, 0 }, { 1, 0, 0 } };

	int idx1;        // |-- Our desired color will be between these two indexes in "color".
	int idx2;        // |
	real fractBetween = 0;  // Fraction between "idx1" and "idx2" where our value is.

	if (value <= 0)      { idx1 = idx2 = 0; }    // accounts for an input <=0
	else if (value >= 1)  { idx1 = idx2 = NUM_COLORS - 1; }    // accounts for an input >=0
	else
	{
		value = value * (NUM_COLORS - 1);        // Will multiply value by 3.
		idx1 = int(value);                  // Our desired color will be after this index.
		idx2 = idx1 + 1;                        // ... and before this index (inclusive).
		fractBetween = value - real(idx1);    // Distance between the two indexes (0-1).
	}

	return v3f(
		(color[idx2][0] - color[idx1][0]) * fractBetween + color[idx1][0],
		(color[idx2][1] - color[idx1][1]) * fractBetween + color[idx1][1],
		(color[idx2][2] - color[idx1][2]) * fractBetween + color[idx1][2]);
}

struct Gradient
{
	struct GradientPoint
	{
		v3f color;
		real value;

		GradientPoint()
		{
		}

		GradientPoint(const v3f& color, real value) : color(color), value(value)
		{
		}
	};

	list_of<GradientPoint> gradientMap;

	void Initialize(MemoryManager* memoryManagerInstance)
	{
		gradientMap.Initialize(memoryManagerInstance);

		// heat map
		//gradientMap.Add(GradientPoint(v3f(0, 0, 1), 0));
		//gradientMap.Add(GradientPoint(v3f(0, 1, 1), .25));
		//gradientMap.Add(GradientPoint(v3f(0, 1, 0), .50));
		//gradientMap.Add(GradientPoint(v3f(1, 1, 0), .75));
		//gradientMap.Add(GradientPoint(v3f(1, 0, 0), 1));
	}

	void Destroy(MemoryManager* memoryManagerInstance)
	{
		gradientMap.Destroy();
	}

	__device__ v3f GetColor(real value)
	{
		v3f result;
		if (value < 0)
			return result;

		if (value > 1)
			return gradientMap[gradientMap.currentCount - 1].color;

		for (int i = 0; i < gradientMap.currentCount; i++)
		{
			const GradientPoint& current = gradientMap[i];
			if (value < current.value)
			{
				GradientPoint& previous = gradientMap[MAX2(0, i - 1)];

				const real fractBetween = (previous.value == current.value) ? 0 : 
					(value - current.value) / (previous.value - current.value);
				result = current.color + (previous.color - current.color) * fractBetween;

				break;
			}
		}

		return result;
	}
};

#endif __gradient_h
