#ifndef __input_h
#define __input_h

#include "TypeDefs.h"
#include "Vectors.h"

#define MAX_CONTROLLERS				1
#define NUM_OF_CONTROLLER_BUTTONS	9
#define NUM_OF_MOUSE_BUTTONS		5

#define MOUSE_BUTTON_VALUES(_) \
	_(Left,=0) \
	_(Middle,) \
	_(Right,)
DECLARE_ENUM(MouseButton, MOUSE_BUTTON_VALUES)
#undef MOUSE_BUTTON_VALUES


struct button_state
{
	int32 halfTransitionCount;
	bool32 isDown;
};

struct mouse_button_state
{
	b32 isDown;
	b32 pressed;
	b32 released;
};

struct controller_input
{
	b32 isConnected;
	b32 isAnalog;

	f32 stickAverageX;
	f32 stickAverageY;

	union
	{
		button_state buttons[NUM_OF_CONTROLLER_BUTTONS];
		struct
		{
			button_state moveUp;
			button_state moveDown;
			button_state moveLeft;
			button_state moveRight;

			button_state actionUp;
			button_state actionDown;
			button_state actionLeft;
			button_state actionRight;

			button_state menu;
		};
	};
};

struct application_input
{
	mouse_button_state mouseButtons[NUM_OF_MOUSE_BUTTONS];
	v3i mousePosition;

	controller_input controllers[MAX_CONTROLLERS];

	application_input()
	{
		Clear();
	}

	void Clear()
	{
		for (int i = 0; i < MAX_CONTROLLERS; ++i)
		{
			for (int i2 = 0; i2 < NUM_OF_CONTROLLER_BUTTONS; ++i2)
			{
				controllers[i].buttons[i2].halfTransitionCount = 0;
				controllers[i].buttons[i2].isDown = 0;
			}
		}
	}

	inline static const controller_input* GetController(const application_input* input, int index)
	{
		ASSERT(index < ARRAY_COUNT(input->controllers));

		return &input->controllers[index];
	}
};

#endif __input_h
