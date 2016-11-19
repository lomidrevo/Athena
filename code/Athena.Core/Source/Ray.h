#ifndef __ray_h
#define __ray_h

#include "Camera.h"
#include "TypeDefs.h"
#include "Vectors.h"


struct Ray
{
	v3f origin;
	v3f direction;
	v3f invDirection;
	v3ui sign;

	__device__ Ray()
	{

	}

	__device__ Ray(const v3f& _origin)
	{
		origin = _origin;
	}

	__device__ Ray(const v3f& _origin, const v3f& _direction)
	{
		Prepare(_origin, _direction);
	}

	__device__ void Prepare(const v3f& _origin, const v3f& _direction)
	{
		origin = _origin;
		direction = _direction;

		Prepare();
	}

	// preparations for AACell.Collide(Ray)
	__device__ void Prepare()
	{
		invDirection.Set((real)1 / direction.x, (real)1 / direction.y, (real)1 / direction.z);
		sign.Set(invDirection.x < 0 ? 1 : 0, invDirection.y < 0 ? 1 : 0, invDirection.z < 0 ? 1 : 0);
	}

	static __device__ Ray GetPrimary(const v3f* cameraParams, const v2ui& pixel, const v2ui& pixelSize, real noise = .5)
	{
		const v2f k(noise * pixelSize.x + pixel.x, noise * pixelSize.y + pixel.y);

		// primary ray
		Ray ray(cameraParams[CameraParameter::Position]);
		ray.direction =
			cameraParams[CameraParameter::ImageOrigin] +
			(cameraParams[CameraParameter::ImageHeightIterator] * k.y) +
			(cameraParams[CameraParameter::ImageWidthIterator] * k.x) -
			ray.origin;

		vectors::Normalize(ray.direction);
		ray.Prepare();

		return ray;
	}
};

#endif __ray_h
