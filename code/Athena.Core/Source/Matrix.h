#pragma once

#include <memory.h>
#include <math.h>
#include "TypeDefs.h"


// TODO prerobit komplet
class Matrix4x4
{
public:

	enum 
	{ 
		TX = 3, 
		TY = 7, 
		TZ = 11, 
		D0 = 0, 
		D1 = 5, 
		D2 = 10, 
		D3 = 15, 
		SX = D0, 
		SY = D1, 
		SZ = D2, 
		W = D3 
	};

	real box[16];

	Matrix4x4()
	{
		Identity();
	}

	void Identity()
	{
		box[1] = box[2] = box[TX] = box[4] = box[6] = box[TY] = box[8] = box[9] = box[TZ] = box[12] = box[13] = box[14] = 0;
		box[D0] = box[D1] = box[D2] = box[W] = 1;
	}

	void Rotate(const v3f& pos, const v3f& rot)
	{
		Matrix4x4 t;
		t.RotateX(rot.z);
		RotateY(rot.y);
		Concatenate(t);
		t.RotateZ(rot.x);
		Concatenate(t);
		Translate(pos);
	}

	void Translate(const v3f& pos)
	{
		box[TX] += pos.x; 
		box[TY] += pos.y; 
		box[TZ] += pos.z;
	}

	void Concatenate(const Matrix4x4& m2)
	{
		Matrix4x4 res;

		for (uint c = 0; c < 4; c++)
			for (uint r = 0; r < 4; r++)
			{
				res.box[r * 4 + c] = box[r * 4] * m2.box[c] + 
					box[r * 4 + 1] * m2.box[c + 4] +
					box[r * 4 + 2] * m2.box[c + 8] +
					box[r * 4 + 3] * m2.box[c + 12];
			}

		memcpy(box, res.box, sizeof(box));
	}

	void Transform(const v3f& v, v3f& output) const
	{
		v3f tmp;
		tmp.x = box[0] * v.x + box[1] * v.y + box[2] * v.z + box[3];
		tmp.y = box[4] * v.x + box[5] * v.y + box[6] * v.z + box[7];
		tmp.z = box[8] * v.x + box[9] * v.y + box[10] * v.z + box[11];

		output = tmp;
	}

	void RotateX(real rx)
	{
		Identity();

		const real sx = (real)sin(rx * _PI / 180);
		box[5] = box[10] = (real)cos(rx * _PI / 180);
		box[6] = sx;
		box[9] = -sx;
	}

	void RotateY(real ry)
	{
		Identity();

		const real sy = (real)sin(ry * _PI / 180);
		box[0] = box[10] = (real)cos(ry * _PI / 180);
		box[2] = -sy;
		box[8] = sy;
	}

	void RotateZ(real rz)
	{
		Identity();

		const real sz = (real)sin(rz * _PI / 180);
		box[0] = box[5] = (real)cos(rz * _PI / 180);
		box[1] = sz;
		box[4] = -sz;
	}

	void Invert()
	{
		Matrix4x4 t;

		const real tx = -box[3], ty = -box[7], tz = -box[11];

		for (int h = 0; h < 3; h++) 
			for (int v = 0; v < 3; v++) 
				t.box[h + v * 4] = box[v + h * 4];

		for (int i = 0; i < 11; i++) 
			box[i] = t.box[i];

		box[3]  = tx * box[0] + ty * box[1] + tz * box[2];
		box[7]  = tx * box[4] + ty * box[5] + tz * box[6];
		box[11] = tx * box[8] + ty * box[9] + tz * box[10];
	}	
};
