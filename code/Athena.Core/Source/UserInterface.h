#ifndef __user_interface_h
#define __user_interface_h

#include "Array.h"
#include "Input.h"
#include "List.h"
#include "TypeDefs.h"
#include "Vectors.h"

#define GUI_ELEMENT_TYPE_VALUES(_) \
    _(GuiElement,=0) \
    _(GuiMenuItem,) \
    _(GuiButton,)
DECLARE_ENUM(GuiElementType, GUI_ELEMENT_TYPE_VALUES)
#undef GUI_ELEMENT_TYPE_VALUES

#define GUI_ELEMENT_ID_VALUES(_) \
    _(Undefined,=0) \
    _(CurrentFrameBufferMenuItem,) \
    _(ColorFrameBufferBtn,) \
    _(NormalFrameBufferBtn,) \
    _(DepthFrameBufferBtn,) \
    _(DebugFrameBufferBtn,)
DECLARE_ENUM(GuiElementId, GUI_ELEMENT_ID_VALUES)
#undef GUI_ELEMENT_ID_VALUES


struct AthenaStorage;

#define GUI_BUTTON_CLICKED_PARAMETERS GuiElementId::Enum buttonId, AthenaStorage* storage
typedef void(*ButtonClicked)(GUI_BUTTON_CLICKED_PARAMETERS);

#define SCENE_CLICKED_PARAMETERS MouseButton::Enum mouseButton, v2ui mousePosition, AthenaStorage* storage
typedef void(*SceneClicked)(SCENE_CLICKED_PARAMETERS);

class MemoryManager;

struct GuiElement
{
	GuiElement* parent;
	array_of<GuiElement> childrenItems;

	v2ui position;	// absolute, not relative to parent
	v2ui size;

	GuiElementId::Enum id;
	GuiElementType::Enum type;

	String text;

	ButtonClicked buttonClickedFunction;

	bool enabled;
	bool visible;
	bool highlighted;

	void Set(GuiElementType::Enum type, v2ui size, GuiElementId::Enum id)
	{
		this->id = id;
		this->size = size;
		this->type = type;

		enabled = true;
		visible = false;
		highlighted = false;

		parent = null;
		buttonClickedFunction = null;

		childrenItems.count = 0;
		childrenItems.ptr = null;
	}

	void Destroy(MemoryManager* memoryManagerInstance);
};

struct rgba_as_uint32;

struct Bitmap
{
	v2ui size;
	array_of<rgba_as_uint32> data;
};

struct Glyph
{
	v2ui offset;
	Bitmap bitmap;
};

struct Font
{
	v2ui glyphSize;
	array_of<Glyph> glyphs;
	v2ui spacing;
};


struct Frame;
struct Timer;
struct Parameter;

class UserInterface
{
public:

	void Initialize(MemoryManager* memoryManagerInstance, AthenaStorage* athenaStorage);
	void Destroy(MemoryManager* memoryManagerInstance);

	void SetContextMenu(array_of<GuiElement> newContextMenuItems);

	void Hide();
	void Show();

	void Draw(AthenaStorage* athenaStorage, array_of<ui32>& output) const;

	void Update(AthenaStorage* athenaStorage, const v2ui& mousePosition);
	void MouseButtonClicked(MouseButton::Enum mouseButton, v2ui mousePosition, AthenaStorage* storage);
	void MouseButtonState(MouseButton::Enum mouseButton, b32 isDown, v2ui mousePosition, AthenaStorage* storage);

	SceneClicked sceneClickedFunction;

private:
	
	void DrawDebugUi(const list_of<Parameter>& parameters, const array_of<array_of<Timer>>& timersLog,
		array_of<uint32>& output, const v2ui& outputSize) const;
	void DrawDebugGraph(const array_of<array_of<Timer>>& timersLog,
		array_of<uint32>& output, const v2ui& outputSize) const;
	void DrawContextMenu(const v2ui& position,
		array_of<uint32>& output, const v2ui& outputSize) const;
	void DrawElement(const GuiElement& element, const v2ui& position,
		array_of<uint32>& output, const v2ui& outputSize) const;
	void DrawRectangle(v2ui position, v2ui size, v4b color, bool fill,
		array_of<uint32>& output, const v2ui& outputSize) const;
	void DrawBitmap(const v2ui& position, const Bitmap& bitmap,
		array_of<uint32>& output, const v2ui& outputSize) const;
	void DrawString(const v2ui& leftCenteredPosition, const String& text, const v4b color,
		array_of<uint32>& output, const v2ui& outputSize) const;
	void DrawString(const v2ui& leftCenteredPosition, char* text, const v4b color,
		array_of<uint32>& output, const v2ui& outputSize) const;

	void UpdateDebugUi(list_of<Parameter>& parameters);
	void UpdateContextMenu();
	void UpdateContextMenu(GuiElement& element);

	void UpdateElement(GuiElement& element, v2ui position, GuiElement* parent);
	void ResetElement(GuiElement& element);

	void ReadFont(const char* fontFileName, MemoryManager* memoryManagerInstance, Font& font);

	bool PointInRectangle(const v2ui& start, const v2ui size, const v2ui& point) const;
	inline bool PointInChildren(const array_of<GuiElement>& elementArray, const v2ui& point) const;

	void ProcessMouseClick(MouseButton::Enum mouseButton, v2ui mousePosition, AthenaStorage* storage);
	void ProcessMouseMovement(MouseButton::Enum mouseButton, v2ui mousePosition, AthenaStorage* storage);

	array_of<GuiElement> contextMenuItems;
	v2ui currentContextMenuPosition;
	v2ui debugUiPosition;

	v2ui currentMousePosition;
	v2ui previousMousePosition;

	GuiElement* hilightedElement;
	
	uint hilightedParameterId;
	uint currentParameterId;

	v4b hilightedBackgroundColor;
	v4b backgroundColor;
	v4b textColor;
	v4b hilightedTextColor;

	Font font;
	
	b32 visible;
	b32 debugUiGraphVisible;

	static list_of<v3b> colors;
};

#endif __user_interface_h
