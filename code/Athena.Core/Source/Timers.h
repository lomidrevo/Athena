#ifndef __timers_h
#define __timers_h

#include "TypeDefs.h"


#define TIMER_ID_VALUES(_) \
	_(Application,) \
	_(Frame,) \
		_(SystemProcessing,) \
		_(Update,) \
			_(ProcessInput,) \
			_(CameraUpdate,) \
			_(OctreeConstruction,) \
			_(BihConstruction,) \
		_(Draw,) \
			_(Render,) \
			_(PostProcess,) \
			_(GuiDraw,) \
		_(Display,)
DECLARE_ENUM(TimerId, TIMER_ID_VALUES)
#undef TIMER_ID_VALUES

#endif __timers_h
