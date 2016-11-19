#ifndef __camera_h
#define __camera_h

#include "TypeDefs.h"
#include "Vectors.h"

#define CAMERA_PARAMETER_VALUES(_) \
    _(Position,=0) \
    _(Target,) \
    _(View,) \
    _(ImageOrigin,) \
    _(ImageWidthIterator,) \
    _(ImageHeightIterator,)
DECLARE_ENUM(CameraParameter, CAMERA_PARAMETER_VALUES)
#undef CAMERA_PARAMETER_VALUES


struct AthenaStorage;

class DLL_EXPORT Camera
{
public:

	inline const v3f* GetParameters() const { return parameters; }
	inline const v3f& GetParameter(CameraParameter::Enum parameter) const { return parameters[(int)parameter]; }

	Camera();
	Camera(const v3f& position, const v3f& target, real movementSpeed = 5, real mouseSensitivity = 1e-2);
	
	void Set(const v3f& position, const v3f& target, real movementSpeed = 5, real mouseSensitivity = 1e-2);

	bool Update(AthenaStorage* athenaStorage);
	bool Update(v2ui outputSize);

	void RotateView(int dx, int dy);
	void RotateAroundTarget(int dx, int dy);
	void ZoomToTarget(real f);

	void Move(real step);
	void Strafe(real step);

private:

	v3f parameters[CameraParameter::Count];
	real mouseSpeed, movementSpeed;

	bool changed;
	bool upSideDown;
};

#endif __camera_h
