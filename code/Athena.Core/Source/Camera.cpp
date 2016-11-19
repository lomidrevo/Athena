
#include "Athena.h"
#include "Camera.h"
#include <math.h>
#include "Matrix.h"
#include "Timer.h"
#include "Timers.h"


Camera::Camera()
{

}

Camera::Camera(const v3f& position, const v3f& target, real movementSpeed, real mouseSensitivity)
{
	Set(position, target, movementSpeed, mouseSensitivity);
}

void Camera::Set(const v3f& position, const v3f& target, real movementSpeed, real mouseSensitivity)
{
	parameters[CameraParameter::Position] = position;
	parameters[CameraParameter::Target] = target;
	changed = true;
	upSideDown = false;

	this->mouseSpeed = mouseSensitivity;
	this->movementSpeed = movementSpeed;
}

bool Camera::Update(AthenaStorage* athenaStorage)
{
	TIMED_BLOCK(&athenaStorage->timers[TimerId::CameraUpdate]);

	return Update(athenaStorage->frame.size);
}

bool Camera::Update(v2ui outputSize)
{
	if (!changed)
		return changed;

	const real fieldOfVision = 45;
	
	parameters[CameraParameter::View] = parameters[CameraParameter::Target];
	parameters[CameraParameter::View] -= parameters[CameraParameter::Position];
	vectors::Normalize(parameters[CameraParameter::View]);

	const real aspectRatio = (real)outputSize.x / (real)outputSize.y;
	const real targetDistance = 1000;

	const real sizex = (real)tan(fieldOfVision * .5 * (_PI / 180)) * targetDistance;
	const real sizey = sizex / aspectRatio;

	v3f viewCorner1(-sizex, sizey, targetDistance); 
	v3f viewCorner2(sizex, sizey, targetDistance);
	v3f viewCorner3(sizex, sizey, targetDistance);
	v3f viewCorner4(-sizex, -sizey, targetDistance);

	v3f zaxis = parameters[CameraParameter::View];

	v3f up(0, 1, 0);

	if (upSideDown)
		vectors::Inv(up);

	v3f xaxis = vectors::Cross(up, zaxis);
	vectors::Normalize(xaxis);

	v3f invZaxis = zaxis;
	vectors::Inv(invZaxis);
	v3f yaxis = vectors::Cross(xaxis, invZaxis);
	vectors::Normalize(yaxis);

	Matrix4x4 m;
	m.box[0] = xaxis.x; m.box[1] = xaxis.y; m.box[2] = xaxis.z;
	m.box[4] = yaxis.x; m.box[5] = yaxis.y; m.box[6] = yaxis.z;
	m.box[8] = zaxis.x; m.box[9] = zaxis.y; m.box[10] = zaxis.z;
	m.Invert();

	m.box[3] = parameters[CameraParameter::Position].x;
	m.box[7] = parameters[CameraParameter::Position].y;
	m.box[11] = parameters[CameraParameter::Position].z;

	v3f zero;

	// move camera
	m.Transform(zero, parameters[CameraParameter::Position]);
	
	// 1--2
	// 4--3
	m.Transform(viewCorner1, viewCorner1);
	m.Transform(viewCorner2, viewCorner2);
	m.Transform(viewCorner3, viewCorner3);
	m.Transform(viewCorner4, viewCorner4);

	parameters[CameraParameter::ImageWidthIterator].Set( 
		(viewCorner2.x - viewCorner1.x) / (real)outputSize.x, 
		(viewCorner2.y - viewCorner1.y) / (real)outputSize.x,
		(viewCorner2.z - viewCorner1.z) / (real)outputSize.x);
	parameters[CameraParameter::ImageHeightIterator].Set(
		(viewCorner4.x - viewCorner1.x) / (real)outputSize.y,
		(viewCorner4.y - viewCorner1.y) / (real)outputSize.y,
		(viewCorner4.z - viewCorner1.z) / (real)outputSize.y);

	const v3f& imageWidthIterator = parameters[CameraParameter::ImageWidthIterator];
	const v3f& imageHeightIterator = parameters[CameraParameter::ImageHeightIterator];

	parameters[CameraParameter::ImageOrigin] = viewCorner1 + imageWidthIterator * .5 + imageHeightIterator * .5;

	vectors::Normalize(parameters[CameraParameter::View]);

	changed = false;
	return true;
}

void Camera::RotateView(int dx, int dy)
{
	v3f up(0, 1, 0);
	if (upSideDown)
		vectors::Inv(up);

	if (dx != 0)
	{
		vectors::RotatePointAround(
			parameters[CameraParameter::Position], up, dx * mouseSpeed, parameters[CameraParameter::Target]);

		changed = true;
	}

	if (dy != 0)
	{
		v3f axis;

		axis = vectors::Cross(parameters[CameraParameter::View], up);
		vectors::Normalize(axis);

		const v3f& cameraTarget = parameters[CameraParameter::Target];
		const v3f& cameraPosition = parameters[CameraParameter::Position];

		const v3f camPosBefore = cameraPosition - cameraTarget;

		vectors::RotatePointAround(
			parameters[CameraParameter::Position], axis, -dy * mouseSpeed, parameters[CameraParameter::Target]);

		const v3f camPosAfter = cameraPosition - cameraTarget;

		if (camPosBefore.x * camPosAfter.x < 0 && camPosBefore.z * camPosAfter.z < 0)
			upSideDown = !upSideDown;

		changed = true;
	}
}

void Camera::RotateAroundTarget(int dx, int dy)
{
	v3f up(0, 1, 0);
	if (upSideDown)
		vectors::Inv(up);

	if (dx != 0)
	{
		vectors::RotatePointAround(
			parameters[CameraParameter::Target], up, dx * mouseSpeed, parameters[CameraParameter::Position]);
	
		changed = true;
	}

	if (dy != 0)
	{
		v3f axis = vectors::Cross(parameters[CameraParameter::View], up);
		vectors::Normalize(axis);

		const v3f& cameraTarget = parameters[CameraParameter::Target];
		const v3f& cameraPosition = parameters[CameraParameter::Position];

		v3f camPosBefore = cameraPosition - cameraTarget;

		vectors::RotatePointAround(
			parameters[CameraParameter::Target], axis, -dy * mouseSpeed, parameters[CameraParameter::Position]);
		
		v3f camPosAfter = cameraPosition - cameraTarget;

		if (camPosBefore.x * camPosAfter.x < 0 && camPosBefore.z * camPosAfter.z < 0)
			upSideDown = !upSideDown;
		
		changed = true;
	}
}

void Camera::ZoomToTarget(real f)
{
	parameters[CameraParameter::Position] += parameters[CameraParameter::View] * f;
	
	changed = true;
}

void Camera::Move(real step)
{
	step *= movementSpeed;

	v3f move = parameters[CameraParameter::View] * step;

	parameters[CameraParameter::Position] += move;
	parameters[CameraParameter::Target] += move;

	changed = true;
}

void Camera::Strafe(real step)
{
	step *= movementSpeed;

	v3f down(0, -1, 0);
	if (upSideDown)
		vectors::Inv(down);

	v3f strafe = vectors::Cross(parameters[CameraParameter::View], down);
	vectors::Normalize(strafe);
	strafe *= step;

	parameters[CameraParameter::Position] += strafe;
	parameters[CameraParameter::Target] += strafe;

	changed = true;
}
