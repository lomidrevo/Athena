#ifndef __z_order_h
#define __z_order_h

// https://fgiesen.wordpress.com/2009/12/13/decoding-morton-codes/

#include "TypeDefs.h"
#include "Vectors.h"


struct ZOrder
{
	// "Insert" a 0 bit after each of the 16 low bits of x
	static __device__ uint32 Part1By1(uint32 x)
	{
		x &= 0x0000ffff;					// x = ---- ---- ---- ---- fedc ba98 7654 3210
		x = (x ^ (x << 8)) & 0x00ff00ff;	// x = ---- ---- fedc ba98 ---- ---- 7654 3210
		x = (x ^ (x << 4)) & 0x0f0f0f0f;	// x = ---- fedc ---- ba98 ---- 7654 ---- 3210
		x = (x ^ (x << 2)) & 0x33333333;	// x = --fe --dc --ba --98 --76 --54 --32 --10
		x = (x ^ (x << 1)) & 0x55555555;	// x = -f-e -d-c -b-a -9-8 -7-6 -5-4 -3-2 -1-0
		return x;
	}

	// "Insert" two 0 bits after each of the 10 low bits of x
	static __device__ uint32 Part1By2(uint32 x)
	{
		x &= 0x000003ff;					// x = ---- ---- ---- ---- ---- --98 7654 3210
		x = (x ^ (x << 16)) & 0xff0000ff;	// x = ---- --98 ---- ---- ---- ---- 7654 3210
		x = (x ^ (x << 8)) & 0x0300f00f;	// x = ---- --98 ---- ---- 7654 ---- ---- 3210
		x = (x ^ (x << 4)) & 0x030c30c3;	// x = ---- --98 ---- 76-- --54 ---- 32-- --10
		x = (x ^ (x << 2)) & 0x09249249;	// x = ---- 9--8 --7- -6-- 5--4 --3- -2-- 1--0
		return x;
	}

	// Inverse of Part1By1 - "delete" all odd-indexed bits
	static __device__ uint32 Compact1By1(uint32 x)
	{
		x &= 0x55555555;					// x = -f-e -d-c -b-a -9-8 -7-6 -5-4 -3-2 -1-0
		x = (x ^ (x >> 1)) & 0x33333333;	// x = --fe --dc --ba --98 --76 --54 --32 --10
		x = (x ^ (x >> 2)) & 0x0f0f0f0f;	// x = ---- fedc ---- ba98 ---- 7654 ---- 3210
		x = (x ^ (x >> 4)) & 0x00ff00ff;	// x = ---- ---- fedc ba98 ---- ---- 7654 3210
		x = (x ^ (x >> 8)) & 0x0000ffff;	// x = ---- ---- ---- ---- fedc ba98 7654 3210
		return x;
	}

	// Inverse of Part1By2 - "delete" all bits not at positions divisible by 3
	static __device__ uint32 Compact1By2(uint32 x)
	{
		x &= 0x09249249;					// x = ---- 9--8 --7- -6-- 5--4 --3- -2-- 1--0
		x = (x ^ (x >> 2)) & 0x030c30c3;	// x = ---- --98 ---- 76-- --54 ---- 32-- --10
		x = (x ^ (x >> 4)) & 0x0300f00f;	// x = ---- --98 ---- ---- 7654 ---- ---- 3210
		x = (x ^ (x >> 8)) & 0xff0000ff;	// x = ---- --98 ---- ---- ---- ---- 7654 3210
		x = (x ^ (x >> 16)) & 0x000003ff;	// x = ---- ---- ---- ---- ---- --98 7654 3210
		return x;
	}

	static __device__ uint32 Encode2ui(v2ui v)
	{
		return (Part1By1(v.y) << 1) + Part1By1(v.x);
	}

	static __device__ uint32 Encode3ui(v3ui v)
	{
		return (Part1By2(v.z) << 2) + (Part1By2(v.y) << 1) + Part1By2(v.x);
	}

	static __device__ v2ui Decode2ui(uint32 code)
	{
		return v2ui{ Compact1By1(code >> 0), Compact1By1(code >> 1) };
	}

	static __device__ v3ui Decode3ui(uint32 code)
	{
		return v3ui{ Compact1By2(code >> 0), Compact1By2(code >> 1), Compact1By2(code >> 2) };
	}

	static __device__ uint32 Decode2x(uint32 code)
	{
		return Compact1By1(code >> 0);
	}

	static __device__ uint32 Decode2y(uint32 code)
	{
		return Compact1By1(code >> 1);
	}

	static __device__ uint32 Decode3x(uint32 code)
	{
		return Compact1By2(code >> 0);
	}

	static __device__ uint32 Decode3y(uint32 code)
	{
		return Compact1By2(code >> 1);
	}

	static __device__ uint32 Decode3z(uint32 code)
	{
		return Compact1By2(code >> 2);
	}
};

#endif __z_order_h
