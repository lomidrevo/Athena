#pragma once

#include <Vectors.h>


struct Material
{
	v3f diffuseColor;
	v3f specularColor;
		
	real reflection;
	real refraction;
	real refractionIndex;
	real shininess;

	__device__ inline void Clear()
	{
		reflection = refraction = refractionIndex = shininess = 0;

		diffuseColor.Set(1, 1, 1);
		specularColor.Set(0, 0, 0);
	}
};


//struct BSDF
//{
//
//};
//
//struct Lambertian : BSDF
//{
//	v3f lambertian;
//
//	__device__ v3f Evaluate(const v3f& wi, const v3f& wo, const v3f& n)
//	{
//		const real dot = xyz_DOT(wi, n);
//
//		v3f result = lambertian;
//		xyz_MUL(result, dot);
//
//		return result;
//	}
//};
//
//struct BlinnPhong : Lambertian
//{
//	v3f glossy;
//	real glossySharpness;
//
//	__device__ v3f Evaluate(const v3f& wi, const v3f& wo, const v3f& n)
//	{
//
//
//
//		return lambertian;
//	}
//};
