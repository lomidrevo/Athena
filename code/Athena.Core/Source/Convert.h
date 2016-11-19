#ifndef __convert_h
#define __convert_h

#include "TypeDefs.h"
#include "Vectors.h"


struct rgba_as_uint32
{
	ui8 b, g, r, a;

	__device__ rgba_as_uint32()
	{

	}

	__device__ rgba_as_uint32(const v4b& v)
	{
		Set(v);
	}

	__device__ rgba_as_uint32(const ui32 ui)
	{
		*((uint32*)&b) = ui;
	}

	__device__ rgba_as_uint32(const v4b* v)
	{
		r = v->x;
		g = v->y;
		b = v->z;
		a = v->w;
	}

	__device__ rgba_as_uint32(ui8 r, ui8 g, ui8 b, ui8 a)
	{
		this->r = r;
		this->g = g;
		this->b = b;
		this->a = a;
	}

	__device__ void Set(const v4b& v)
	{
		r = v.x;
		g = v.y;
		b = v.z;
		a = v.w;
	}

	__device__ void Add(const rgba_as_uint32* oldColor)
	{
		const real invAlpha = (real)a / 0xff;
		r = (ui8)(invAlpha * r) + (ui8)(((real)1 - invAlpha) * oldColor->r);
		g = (ui8)(invAlpha * g) + (ui8)(((real)1 - invAlpha) * oldColor->g);
		b = (ui8)(invAlpha * b) + (ui8)(((real)1 - invAlpha) * oldColor->b);
	}

	__device__ void Add(const rgba_as_uint32& oldColor)
	{
		const real invAlpha = (real)a / 0xff;
		r = (ui8)(invAlpha * r) + (ui8)(((real)1 - invAlpha) * oldColor.r);
		g = (ui8)(invAlpha * g) + (ui8)(((real)1 - invAlpha) * oldColor.g);
		b = (ui8)(invAlpha * b) + (ui8)(((real)1 - invAlpha) * oldColor.b);
	}

	inline __device__ rgba_as_uint32& operator*=(const v4b& c)
	{
		if (this->r == 0 && this->g == 0 && this->b == 0)
			return *this;

		vector4f tmp(
			((real)c.x / 255 * (real)r / 255) * 255,
			((real)c.y / 255 * (real)g / 255) * 255,
			((real)c.z / 255 * (real)b / 255) * 255,
			((real)c.w / 255 * (real)a / 255) * 255
			);

		this->r = MIN2(255, (ui8)tmp.x);
		this->g = MIN2(255, (ui8)tmp.y);
		this->b = MIN2(255, (ui8)tmp.z);
		this->a = MIN2(255, (ui8)tmp.w);

		return *this;
	}

	inline __device__ uint32 GetUi32() const { return *(uint32*)&b; }
};

inline __device__ v4b Vector4fToVector4b(const v4f& v)
{
	return v4b(REAL_TO_BYTE(v.x), REAL_TO_BYTE(v.y), REAL_TO_BYTE(v.z), REAL_TO_BYTE(v.w));
}

inline __device__ v4b Vector3fToVector4b(const v3f& v)
{
	return v4b(REAL_TO_BYTE(v.x), REAL_TO_BYTE(v.y), REAL_TO_BYTE(v.z), 255);
}

inline __device__ v3f Vector4bToVector3f(const v4b& v)
{
	return v3f((real)v.x / 255, (real)v.y / 255, (real)v.z / 255);
}

#endif __convert_h
