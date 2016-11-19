#ifndef __frame_h
#define __frame_h

#include "Array.h"
#include "TypeDefs.h"
#include "Vectors.h"

#define FRAME_BUFFER_TYPE_VALUES(_) \
    _(Color,=0) \
    _(Depth,) \
    _(Normal,) \
    _(Debug,)
DECLARE_ENUM(FrameBuffer, FRAME_BUFFER_TYPE_VALUES)
#undef FRAME_BUFFER_TYPE_VALUES


struct ObjectId;

struct Frame
{
	array_of<v4b> buffer[FrameBuffer::Count];
	array_of<ObjectId> objectIdBuffer;
	array_of<v3f> colorAccBuffer;

	v2ui size;

	ui64 count;
	ui64 countSinceChange;
	ui64 countSinceChangeMax;
	FrameBuffer::Enum current;
};

#endif __frame_h
