#include "Vectors.h"

#ifndef VECTORS_H_HEADER_ONLY
#include <math.h>
#include "Random.h"

v3f& vectors::Normalize(v3f& v)
{
	v *= ((real)1 / (real)sqrt(Dot(v, v)));
	return v;
}

real vectors::Length(const v3f& v)
{
	return (real)sqrt(Length2(v));
}

real vectors::Distance(const v3f& v1, const v3f& v2)
{
	return (real)sqrt(Distance2(v1, v2));
}

void vectors::RandomSpherePoint3f(v3f& result)
{
	// nonuniform! too many points on cube edges
	//xyz_SET(result, RANDOM(-1, 1), RANDOM(-1, 1), RANDOM(-1, 1));
	//xyz_NORMALIZE(result);

	// uniform - fastest!
	//real x1 = fRND(-1, 1);
	//real x2 = fRND(-1, 1);
	//const real sqrtx1x2 = (real)sqrt(1 - x1*x1 - x2*x2);
	//result.x = 2 * x1 * sqrtx1x2;
	//result.y = 2 * x2 * sqrtx1x2;
	//result.z = 1 - 2 * (x1*x1 + x2*x2);

	// uniform
	real u1 = fRND(0, PI64 * 2);
	real u2 = fRND(-1, 1);
	const real sqrtu2 = sqrt(1 - u2*u2);
	result.x = sqrtu2 * cos(u1);
	result.y = sqrtu2 * sin(u1);
	result.z = u2;

	// nonuniform! (too many points on poles)
	//const real r1 = RANDOMf(0, PI * 2);
	//const real r2 = RANDOMf(0, PI);
	//const real cosr1 = cos(r1);
	//result.x = cosr1 * cos(r2);
	//result.y = cosr1 * sin(r2);
	//result.z = sin(r1);
}

void vectors::RandomSpherePoint2f(v2f& result)
{
	result.x = fRND(0, _PI * 2);
	result.y = fRND(0, _PI);
}

v3f vectors::RandomHemiSpherePoint(const v3f& normal)
{
	v3f result;
	RandomSpherePoint3f(result);

	if (vectors::Dot(result, normal) < 0)
		vectors::Inv(result);

	return result;
}

void vectors::RotatePointAround(const v3f& center, const v3f& axis, real angle, v3f& point)
{
	point -= center;

	const real cosTheta = (real)cos(angle);
	v3f sinAxis = axis * (real)sin(angle);

	const real xzAxis = axis.x * axis.z;
	const real xyAxis = axis.x * axis.y;
	const real yzAxis = axis.y * axis.z;

	v3f result(
		(cosTheta + (1 - cosTheta) * axis.x * axis.x) * point.x + ((1 - cosTheta) * xyAxis - sinAxis.z) * point.y + ((1 - cosTheta) * xzAxis + sinAxis.y) * point.z,
		((1 - cosTheta) * xyAxis + sinAxis.z) * point.x + (cosTheta + (1 - cosTheta) * axis.y * axis.y) * point.y + ((1 - cosTheta) * yzAxis - sinAxis.x) * point.z,
		((1 - cosTheta) * xzAxis - sinAxis.y) * point.x + ((1 - cosTheta) * yzAxis + sinAxis.x) * point.y + (cosTheta + (1 - cosTheta) * axis.z * axis.z) * point.z);

	point = result + center;
}

bool vectors::GetRefraction(const v3f& normal, v3f& refraction, real n1n2)
{
	const real c1 = -vectors::Dot(normal, refraction);
	const real d = 1 - n1n2*n1n2*(1 - c1*c1);
	if (d < 0)
	{
		vectors::Inv(refraction);
		return false;
	}

	refraction *= n1n2;
	refraction += normal * (n1n2 * c1 - (real)sqrt(d));
	vectors::Normalize(refraction);

	return true;
}

real vectors::GetReflectance(const v3f& normal, const v3f& incident, real n1, real n2)
{
	real r0 = (n1 - n2) / (n1 + n2);
	r0 *= r0;

	real cosX = -vectors::Dot(normal, incident);
	if (n1 > n2)
	{
		const real n = n1 / n2;
		const real sinT2 = n * n * (1 - cosX * cosX);

		if (sinT2 > 1)
			return 1; // total internal reflection

		cosX = (real)sqrt(1 - sinT2);
	}

	const real x = 1 - cosX;
	return r0 + (1 - r0) * x * x * x * x * x;
}
#endif