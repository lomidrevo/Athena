#ifndef __vectors_h
#define __vectors_h

#define VECTORS_H_HEADER_ONLY

#ifdef VECTORS_H_HEADER_ONLY
#include <math.h>
#include "Random.h"
#endif
#include "TypeDefs.h"

template <typename T> struct vector1
{
	T x;

	__device__ vector1() { x = 0; }
	__device__ vector1(T x) { this->x = x; }

	__device__ T& operator[](int index) { ASSERT(index < 1); return ((&x)[index]); }
};

template <typename T> struct vector2
{
	T x, y;

	__device__ vector2() { x = y = 0; }
	__device__ vector2(T x, T y) { Set(x, y); }

	inline __device__ void Set(T x, T y) { this->x = x; this->y = y; }
	inline __device__ T Get(uint32 index) const { ASSERT(index < 2); return ((&x)[index]); }
	inline __device__ T& operator[](uint32 index) { ASSERT(index < 2); return ((&x)[index]); }

	inline __device__ vector2<T>& operator+=(const vector2<T>& v) { x += v.x; y += v.y; return *this; }
	inline __device__ vector2<T>& operator-=(const vector2<T>& v) { x -= v.x; y -= v.y; return *this; }
	inline __device__ vector2<T>& operator*=(const vector2<T>& v) { x *= v.x; y *= v.y; return *this; }
	inline __device__ vector2<T>& operator/=(const vector2<T>& v) { x /= v.x; y /= v.y; return *this; }

	inline __device__ vector2<T>& operator+=(T k) { x += k; y += k; return *this; }
	inline __device__ vector2<T>& operator-=(T k) { x -= k; y -= k; return *this; }
	inline __device__ vector2<T>& operator*=(T k) { x *= k; y *= k; return *this; }
	inline __device__ vector2<T>& operator/=(T k) { x /= k; y /= k; return *this; }

	inline __device__ vector2<T> operator+(const vector2<T>& v) const { vector2<T> tmp(*this); tmp += v; return tmp; }
	inline __device__ vector2<T> operator-(const vector2<T>& v) const { vector2<T> tmp(*this); tmp -= v; return tmp; }
	inline __device__ vector2<T> operator*(const vector2<T>& v) const { vector2<T> tmp(*this); tmp *= v; return tmp; }
	inline __device__ vector2<T> operator/(const vector2<T>& v) const { vector2<T> tmp(*this); tmp /= v; return tmp; }
	inline __device__ vector2<T> operator*(T k) const { vector2<T> tmp(*this); tmp *= k; return tmp; }
	inline __device__ vector2<T> operator/(T k) const { vector2<T> tmp(*this); tmp /= k; return tmp; }
};

template <typename T> struct vector3
{
	T x, y, z;

	// TODO default constructor musi byt prazdny pre __device__ :/
	__device__ vector3() { x = y = z = 0; }
	__device__ vector3(T x, T y, T z) { Set(x, y, z); }

	inline __device__ void Set(T x, T y, T z) { this->x = x; this->y = y; this->z = z; }
	inline __device__ T Get(uint32 index) const { ASSERT(index < 3); return ((&x)[index]); }
	inline __device__ T& operator[](uint32 index) { ASSERT(index < 3); return ((&x)[index]); }

	inline __device__ vector3<T>& operator+=(const vector3<T>& v) { x += v.x; y += v.y; z += v.z; return *this; }
	inline __device__ vector3<T>& operator-=(const vector3<T>& v) { x -= v.x; y -= v.y; z -= v.z; return *this; }
	inline __device__ vector3<T>& operator*=(const vector3<T>& v) { x *= v.x; y *= v.y; z *= v.z; return *this; }
	inline __device__ vector3<T>& operator/=(const vector3<T>& v) { x /= v.x; y /= v.y; z /= v.z; return *this; }

	inline __device__ vector3<T>& operator+=(T k) { x += k; y += k; z += k; return *this; }
	inline __device__ vector3<T>& operator-=(T k) { x -= k; y -= k; z -= k; return *this; }
	inline __device__ vector3<T>& operator*=(T k) { x *= k; y *= k; z *= k; return *this; }
	inline __device__ vector3<T>& operator/=(T k) { x /= k; y /= k; z /= k; return *this; }

	inline __device__ vector3<T> operator+(const vector3<T>& v) const { vector3<T> tmp(*this); tmp += v; return tmp; }
	inline __device__ vector3<T> operator-(const vector3<T>& v) const { vector3<T> tmp(*this); tmp -= v; return tmp; }
	inline __device__ vector3<T> operator*(const vector3<T>& v) const { vector3<T> tmp(*this); tmp *= v; return tmp; }
	inline __device__ vector3<T> operator/(const vector3<T>& v) const { vector3<T> tmp(*this); tmp /= v; return tmp; }
	inline __device__ vector3<T> operator*(T k) const { vector3<T> tmp(*this); tmp *= k; return tmp; }
	inline __device__ vector3<T> operator/(T k) const { vector3<T> tmp(*this); tmp /= k; return tmp; }
};

template <typename T> struct vector4
{
	T x, y, z, w;

	__device__ vector4() { x = y = z = w = 0; }
	__device__ vector4(T x, T y, T z, T w) { Set(x, y, z, w); }
	__device__ vector4(vector3<T> v, T w) { Set(v.x, v.y, v.z, w); }

	inline __device__ void Set(T x, T y, T z, T w) { this->x = x; this->y = y; this->z = z; this->w = w; }
	inline __device__ T Get(uint32 index) const { ASSERT(index < 4); return ((&x)[index]); }
	inline __device__  T& operator[](int index) { ASSERT(index < 4); return ((&x)[index]); }

	inline __device__ vector4<T>& operator+=(const vector4<T>& v) { x += v.x; y += v.y; z += v.z; w += v.w; return *this; }
	inline __device__ vector4<T>& operator-=(const vector4<T>& v) { x -= v.x; y -= v.y; z -= v.z; w -= v.w; return *this; }
	inline __device__ vector4<T>& operator*=(const vector4<T>& v) { x *= v.x; y *= v.y; z *= v.z; w *= v.w; return *this; }
	inline __device__ vector4<T>& operator/=(const vector4<T>& v) { x /= v.x; y /= v.y; z /= v.z; w /= v.w; return *this; }

	inline __device__ vector4<T>& operator+=(T k) { x += k; y += k; z += k; w += k; return *this; }
	inline __device__ vector4<T>& operator-=(T k) { x -= k; y -= k; z -= k; w -= k; return *this; }
	inline __device__ vector4<T>& operator*=(T k) { x *= k; y *= k; z *= k; w *= k; return *this; }
	inline __device__ vector4<T>& operator/=(T k) { x /= k; y /= k; z /= k; w /= k; return *this; }

	inline __device__ vector4<T> operator+(const vector4<T>& v) const { vector4<T> tmp(*this); tmp += v; return tmp; }
	inline __device__ vector4<T> operator-(const vector4<T>& v) const { vector4<T> tmp(*this); tmp -= v; return tmp; }
	inline __device__ vector4<T> operator*(const vector4<T>& v) const { vector4<T> tmp(*this); tmp *= v; return tmp; }
	inline __device__ vector4<T> operator/(const vector4<T>& v) const { vector4<T> tmp(*this); tmp /= v; return tmp; }
	inline __device__ vector4<T> operator*(T k) const { vector4<T> tmp(*this); tmp *= k; return tmp; }
	inline __device__ vector4<T> operator/(T k) const { vector4<T> tmp(*this); tmp /= k; return tmp; }
};

typedef vector2<real>		vector2f;
typedef vector3<real>		vector3f;
typedef vector4<real>		vector4f;
typedef vector2<uint8>		vector2b;
typedef vector3<uint8>		vector3b;
typedef vector4<uint8>		vector4b;
typedef vector2<int>		vector2i;
typedef vector3<int>		vector3i;
typedef vector4<int>		vector4i;
typedef vector2<uint32>		vector2ui;
typedef vector3<uint32>		vector3ui;
typedef vector4<uint32>		vector4ui;
typedef vector1<uint64>		vector1ui64;

typedef vector2f			v2f;
typedef vector3f			v3f;
typedef vector4f			v4f;
typedef vector2b			v2b;
typedef vector3b			v3b;
typedef vector4b			v4b;
typedef vector2i			v2i;
typedef vector3i			v3i;
typedef vector4i			v4i;
typedef vector2ui			v2ui;
typedef vector3ui			v3ui;
typedef vector4ui			v4ui;
typedef vector1ui64			v1ui64;

//warning C4251 : ... needs to have dll - interface to be used by clients of struct ...
#define DLL_EXPORT_VECTOR(type) \
	template struct DLL_EXPORT vector1<type>; \
	template struct DLL_EXPORT vector2<type>; \
	template struct DLL_EXPORT vector3<type>; \
	template struct DLL_EXPORT vector4<type>;

DLL_EXPORT_VECTOR(f32);
DLL_EXPORT_VECTOR(f64);
DLL_EXPORT_VECTOR(int);
DLL_EXPORT_VECTOR(ui8);
DLL_EXPORT_VECTOR(ui16);
DLL_EXPORT_VECTOR(ui32);
DLL_EXPORT_VECTOR(ui64);

struct vectors
{
#ifndef VECTORS_H_HEADER_ONLY
	static v3f& Normalize(v3f& v);
	static real Length(const v3f& v);
	static real Distance(const v3f& v1, const v3f& v2);
	static void RandomSpherePoint3f(v3f& result);
	static void RandomSpherePoint2f(v2f& result);
	static v3f RandomHemiSpherePoint(const v3f& normal);
	static void RotatePointAround(const v3f& center, const v3f& axis, real angle, v3f& point);
	static bool GetRefraction(const v3f& normal, v3f& refraction, real n1n2);
	static real GetReflectance(const v3f& normal, const v3f& incident, real n1, real n2);
#else
	static __device__ v3f& Normalize(v3f& v)
	{
		v *= ((real)1 / (real)sqrt(Dot(v, v)));
		return v;
	}

	static __device__ real vectors::Length(const v3f& v)
	{
		return (real)sqrt(Length2(v));
	}

	static __device__ real vectors::Distance(const v3f& v1, const v3f& v2)
	{
		return (real)sqrt(Distance2(v1, v2));
	}

	static __device__ void vectors::RandomSpherePoint3f(v3f& result)
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

	static __device__ void vectors::RandomSpherePoint2f(v2f& result)
	{
		result.x = fRND(0, _PI * 2);
		result.y = fRND(0, _PI);
	}

	static __device__ v3f vectors::RandomHemiSpherePoint(const v3f& normal)
	{
		v3f result;
		RandomSpherePoint3f(result);

		if (vectors::Dot(result, normal) < 0)
			vectors::Inv(result);

		return result;
	}

	static __device__ void vectors::RotatePointAround(const v3f& center, const v3f& axis, real angle, v3f& point)
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

	static __device__ bool vectors::GetRefraction(const v3f& normal, v3f& refraction, real n1n2)
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

	static __device__ real vectors::GetReflectance(const v3f& normal, const v3f& incident, real n1, real n2)
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

	static __device__ v3f& Clamp(real min, real max, v3f& v)
	{
		CLAMP(v.x, min, max);
		CLAMP(v.y, min, max);
		CLAMP(v.z, min, max);
		return v;
	}

	static __device__ v4f& Clamp(real min, real max, v4f& v)
	{
		CLAMP(v.x, min, max);
		CLAMP(v.y, min, max);
		CLAMP(v.z, min, max);
		CLAMP(v.w, min, max);
		return v;
	}

	static __device__ v3f& Abs(v3f& v)
	{
		if (v.x < 0) v.x = -v.x;
		if (v.y < 0) v.y = -v.y;
		if (v.z < 0) v.z = -v.z;
		return v;
	}

	static __device__ v3f& Inv(v3f& v)
	{
		v.x *= -1; 
		v.y *= -1; 
		v.z *= -1; 
		return v;
	}

	static __device__ real Dot(const v3f& v1, const v3f& v2)
	{
		return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
	}

	static __device__ real Distance2(const v3f& v1, const v3f& v2)
	{
		return (v1.x - v2.x) * (v1.x - v2.x) + (v1.y - v2.y) * (v1.y - v2.y) + (v1.z - v2.z) * (v1.z - v2.z);
	}

	static __device__ real Length2(const v3f& v)
	{
		return Dot(v, v);
	}

	static __device__ v3f Cross(const v3f& v1, const v3f& v2)
	{
		return v3f(v1.y * v2.z - v1.z * v2.y, v1.z * v2.x - v1.x * v2.z, v1.x * v2.y - v1.y * v2.x);
	}

	static __device__ v3f GetReflection(const v3f& normal, const v3f& vector)
	{
		return vector - normal * (2 * vectors::Dot(vector, normal));
	}	
};

#endif __vectors_h
