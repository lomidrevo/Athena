#include "Athena.h"
#include "Debug.h"
#include "Convert.h"
#include "Frame.h"
#include "Log.h"
#include "MemoryManager.h"
#include "Ray.h"
#include "Rendering.h"
#include "StringHelpers.h"
#include "Timer.h"
#include "Timers.h"
#include "UserInterface.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "..\3rd\stb\include\stb_truetype.h"
#include "Win32.h"


list_of<v3b> UserInterface::colors;


void GuiElement::Destroy(MemoryManager* memoryManagerInstance)
{
	for (uint i = 0; i < childrenItems.count; ++i)
		childrenItems[i].Destroy(memoryManagerInstance);

	if (childrenItems.count)
		_MEM_FREE_ARRAY(memoryManagerInstance, GuiElement, &childrenItems);
	
	_MEM_FREE_STRING(memoryManagerInstance, &text);
}

void UserInterface::Initialize(MemoryManager* memoryManagerInstance, AthenaStorage* athenaStorage)
{
	backgroundColor.Set(0x1E, 0x1E, 0x1E, 0xAA);
	hilightedBackgroundColor.Set(0x99, 0x99, 0x99, 0xEE);
	textColor.Set(0xFF, 0xFF, 0xFF, 0xFF);
	hilightedTextColor.Set(0xFF, 0xFF, 0x00, 0xFF);

	UserInterface::colors.Initialize(memoryManagerInstance, "v3b");
	colors.Add(v3b(0, 0, 0));
	colors.Add(v3b(255, 255, 255));
	colors.Add(v3b(255, 0, 0));
	colors.Add(v3b(0, 255, 0));
	colors.Add(v3b(0, 0, 255));
	colors.Add(v3b(255, 255, 0));
	colors.Add(v3b(0, 255, 255));
	colors.Add(v3b(255, 0, 255));
	colors.Add(v3b(127, 0, 0));
	colors.Add(v3b(0, 127, 0));
	colors.Add(v3b(0, 0, 127));
	colors.Add(v3b(127, 127, 0));
	colors.Add(v3b(0, 127, 127));
	colors.Add(v3b(127, 0, 127));
	
#ifdef BUCHANICA
	ReadFont("c:\\filip\\programming\\_projects\\Athena\\data\\fonts\\consola.ttf", memoryManagerInstance, font);
#else
	ReadFont("d:\\stuff\\Projects\\Athena\\data\\fonts\\consola.ttf", memoryManagerInstance, font);
#endif

	visible = false;
	sceneClickedFunction = null;
	currentParameterId = hilightedParameterId = 0;
	
	// reset context menu
	contextMenuItems.ptr = null;
	contextMenuItems.count = 0;

	debugUiPosition.Set(10, 15);

	debugUiGraphVisible = true;
	DEBUG_PARAMETER(memoryManagerInstance, athenaStorage, "debugUiGraphVisible", Type::b32, &debugUiGraphVisible, null, 0);

	LOG_TL(LogLevel::Info, "UserInterface::Initialize");
}

void UserInterface::Destroy(MemoryManager* memoryManagerInstance)
{
	if (contextMenuItems.ptr)
	{
		for (uint i = 0; i < contextMenuItems.count; ++i)
			contextMenuItems[i].Destroy(memoryManagerInstance);

		_MEM_FREE_ARRAY(memoryManagerInstance, GuiElement, &contextMenuItems);
	}

	if (font.glyphs.ptr)
	{
		for (int i = 0; i < font.glyphs.count; ++i)
		{
			if (font.glyphs.ptr[i].bitmap.data.ptr)
				_MEM_FREE_ARRAY(memoryManagerInstance, rgba_as_uint32, &font.glyphs.ptr[i].bitmap.data);
		}

		_MEM_FREE_ARRAY(memoryManagerInstance, Glyph, &font.glyphs);
	}

	UserInterface::colors.Destroy();
}

void UserInterface::ReadFont(const char* fontFileName, MemoryManager* memoryManagerInstance, Font& font)
{
	using namespace Common::Strings;

	auto ttfFile = Win32ReadFile(fontFileName);
	if (!ttfFile.memory || !ttfFile.memorySize)
	{
		LOG_TL(LogLevel::Error, "UserInterface::ReadFont failed ['%s']", fontFileName);
		return;
	}

	stbtt_fontinfo ttfFont;
	stbtt_InitFont(
		&ttfFont, (unsigned char*)ttfFile.memory, stbtt_GetFontOffsetForIndex((unsigned char*)ttfFile.memory, 0));
	int glyphsToRead = MIN2(ttfFont.numGlyphs, 256);

	font.glyphs = _MEM_ALLOC_ARRAY(memoryManagerInstance, Glyph, glyphsToRead);
	font.glyphSize.Set(0, 0);
	font.spacing.Set(1, 2);

	uint memoryUsed = sizeof(Glyph) * glyphsToRead;

	for (int i = 0; i < glyphsToRead; ++i)
	{
		int width, height, xOffset, yOffset;
		uint8* charBitmap = (uint8*)stbtt_GetCodepointBitmap(&ttfFont, 0, stbtt_ScaleForPixelHeight(&ttfFont, 14), i,
			&width, &height, &xOffset, &yOffset);

		//if (width > 0 && height > 0)
		if (i >= '!' && i <= '~')
		{
			font.glyphs.ptr[i].bitmap.size.Set(width, height);
			font.glyphs.ptr[i].offset.Set(xOffset, yOffset);
			font.glyphs.ptr[i].bitmap.data = _MEM_ALLOC_ARRAY(memoryManagerInstance, rgba_as_uint32, width * height);
			memoryUsed += font.glyphs.ptr[i].bitmap.data.count * sizeof(rgba_as_uint32);

			uint8* source = charBitmap;
			rgba_as_uint32* destination = font.glyphs.ptr[i].bitmap.data.ptr;
			for (int y = 0; y < height; y++)
			{
				for (int x = 0; x < width; x++)
				{
					uint8 fontAlpha = *source++;
					*destination++ = rgba_as_uint32(fontAlpha, fontAlpha, fontAlpha, fontAlpha);
				}
			}

			font.glyphSize.x = MAX2(font.glyphSize.x, (ui32)width);
			font.glyphSize.y = MAX2(font.glyphSize.y, (ui32)height);
		}
		else
		{
			font.glyphs.ptr[i].bitmap.data.ptr = null;
			font.glyphs.ptr[i].bitmap.data.count = null;
		}

		stbtt_FreeBitmap(charBitmap, null);
	}

	char tmpBuffer[256] = {};
	LOG_TL(LogLevel::Info, "UserInterface::ReadFont ['%s' %s]", fontFileName, GetMemSizeString(tmpBuffer, memoryUsed));
}

void UserInterface::SetContextMenu(array_of<GuiElement> newContextMenuItems)
{
	contextMenuItems = newContextMenuItems;

	v2ui itemPosition(2, 2);
	for (uint i = 0; i < contextMenuItems.count; ++i)
	{
		ResetElement(contextMenuItems[i]);
		UpdateElement(contextMenuItems[i], itemPosition, null);

		itemPosition.y += contextMenuItems[i].size.y;
	}
}

void UserInterface::UpdateElement(GuiElement& element, v2ui position, GuiElement* parent)
{
	element.parent = parent;
	element.position = position;

	if (element.type == GuiElementType::GuiMenuItem)
		position.x += element.size.x;

	for (uint i = 0; i < element.childrenItems.count; ++i)
	{
		UpdateElement(element.childrenItems[i], position, &element);
		position.y += element.childrenItems[i].size.y;
	}
}

void UserInterface::Hide()
{
	if (!visible)
		return;
	
	visible = false;

	for (uint i = 0; i < contextMenuItems.count; ++i)
		ResetElement(contextMenuItems[i]);

	hilightedElement = null;
	currentParameterId = hilightedParameterId = 0;
}

void UserInterface::ResetElement(GuiElement& element)
{
	element.visible = (element.parent == null);
	element.highlighted = false;

	for (uint i = 0; i < element.childrenItems.count; ++i)
		ResetElement(element.childrenItems[i]);
}

void UserInterface::Show()
{
	if (!visible)
		currentContextMenuPosition = currentMousePosition;

	visible = true;
}

void UserInterface::Draw(AthenaStorage* athenaStorage, array_of<uint32>& output) const
{
	TIMED_BLOCK(&athenaStorage->timers[TimerId::GuiDraw]);

	if (!visible)
		return;
	
	//DrawContextMenu(currentContextMenuPosition, output, outputSize);
	DrawDebugUi(athenaStorage->debugParameters, athenaStorage->timersLog, output, athenaStorage->frame.size);
}

template <typename T>
void PrintEnumParameter(char* tmpBuffer, const Parameter& p)
{
	sprintf(tmpBuffer, "%s: %s", p.name.ptr,
		(p.readOnly) ? T::GetString(*(const T::Enum*)p.readOnlyPtr) : T::GetString(*(T::Enum*)p.ptr));
}

void UserInterface::DrawDebugUi(const list_of<Parameter>& parameters, const array_of<array_of<Timer>>& timersLog,
	array_of<uint32>& output, const v2ui& outputSize) const
{
	if (debugUiGraphVisible)
		DrawDebugGraph(timersLog, output, outputSize);

	const v2ui debugUiSize(outputSize.x / 2, outputSize.y - outputSize.y / 8);
	DrawRectangle(v2ui(0, 0), debugUiSize,
		backgroundColor, true, output, outputSize);

	char tmpBuffer[256] = {};

	v2ui linePosition = debugUiPosition;

	for (uint i = 1; i < parameters.currentCount; ++i)
	{
		const Parameter& p = parameters[i];
		if (p.IsRegion())
		{
			sprintf(tmpBuffer, "%s%s", p.collapsed ? "+" : "-", p.name.ptr);
		}
		else
		{
			int indent = 0;
			if (p.parentId)
			{
				if (parameters[p.parentId].collapsed)
					continue;

				// TODO viac-urovnove odsadzovanie
				tmpBuffer[0] = '\t';
				indent = 1;
			}

			//if (!p.readOnly)
			//{
			//	tmpBuffer[indent] = '*';
			//	indent++;
			//}

			switch (p.type)
			{
				case Type::b32:
				{
					bool value = (p.readOnly) ? *(const bool*)p.readOnlyPtr : *(bool*)p.ptr;
					sprintf(tmpBuffer + indent, "%s: %s", p.name.ptr, 
						value ? "true" : "false");
				}
				break;

				case Type::ui8:
				{
					sprintf(tmpBuffer + indent, "%s: %X", p.name.ptr, 
						(p.readOnly) ? *(const ui8*)p.readOnlyPtr : *(ui8*)p.ptr);
				}
				break;

				case Type::uint:
				case Type::ui64:
				{
					sprintf(tmpBuffer + indent, "%s: %I64d", p.name.ptr, 
						(p.readOnly) ? *(const ui64*)p.readOnlyPtr : *(ui64*)p.ptr);
				}
				break;

				case Type::ui32:
				{
					if (!p.maxValuePtr)
						sprintf(tmpBuffer + indent, "%s: %u", p.name.ptr, 
							(p.readOnly) ? *(const ui32*)p.readOnlyPtr : *(ui32*)p.ptr);
					else
						sprintf(tmpBuffer + indent, "%s: %u/%u", p.name.ptr,
							(p.readOnly) ? *(const ui32*)p.readOnlyPtr : *(ui32*)p.ptr, *(const ui32*)p.maxValuePtr - 1);
				}
				break;

				case Type::f32:
				{
					sprintf(tmpBuffer + indent, "%s: %.3f", p.name.ptr, 
						(p.readOnly) ? *(const f32*)p.readOnlyPtr : *(f32*)p.ptr);
				}
				break;

				case Type::timer:
				{
					// TODO timer formating min/max/avg
				}
				break;

				case Type::f64:
				case Type::real:
				{
					sprintf(tmpBuffer + indent, "%s: %.3f", p.name.ptr, 
						(p.readOnly) ? *(const real*)p.readOnlyPtr : *(real*)p.ptr);
				}
				break;

				case Type::v3i:
				{
					const v3i* v = (p.readOnly) ? (const v3i*)p.readOnlyPtr : (v3i*)p.ptr;
					sprintf(tmpBuffer + indent, "%s: [%d, %d, %d]", p.name.ptr, 
						v->x, v->y, v->z);
				}
				break;

				case Type::v3f:
				{
					const v3f* v = (p.readOnly) ? (const v3f*)p.readOnlyPtr : (v3f*)p.ptr;
					sprintf(tmpBuffer + indent, "%s: [%.03f, %.03f, %.03f]", p.name.ptr, 
						v->x, v->y, v->z);
				}
				break;

				case Type::frameBufferEnum:
					PrintEnumParameter<FrameBuffer>(tmpBuffer + indent, p);
					break;

				case Type::rendererEnum:
					PrintEnumParameter<Renderer>(tmpBuffer + indent, p);
					break;

				case Type::renderingMethodEnum:
					PrintEnumParameter<RenderingMethod>(tmpBuffer + indent, p);
					break;

				case Type::tracingMethodEnum:
					PrintEnumParameter<TracingMethod>(tmpBuffer + indent, p);
					break;

				case Type::renderingModeEnum:
					PrintEnumParameter<RenderingMode>(tmpBuffer + indent, p);
					break;

				default:
					sprintf(tmpBuffer + indent, "%s: ?unknownType?", p.name.ptr);
					break;
			}
		}

		DrawString(linePosition, tmpBuffer, (i == hilightedParameterId && !p.readOnly) ? hilightedTextColor : textColor, 
			output, outputSize);
		linePosition.y += font.glyphSize.y + font.spacing.y;

		// if no more lines fit to debugUi
		if (linePosition.y + font.glyphSize.y > debugUiSize.y)
			break;
	}
}

#define DRAW_DEBUG_UI_BAR(timerId) \
{ \
	value = (ui32)(timersLog[i][timerId].lastDurationMs * barSize.y); \
	barPosition.y -= value; \
	DrawRectangle(barPosition, v2ui(barSize.x, value), v4b(colors[nextColor], 0xAA), fill, output, outputSize); \
	nextColor++; \
	if (barPosition.y > outputSize.y) \
		continue; \
}

void UserInterface::DrawDebugGraph(const array_of<array_of<Timer>>& timersLog,
	array_of<uint32>& output, const v2ui& outputSize) const
{
	// barBaseSize.y is num of pixels for 1ms
	const v2ui barSize((outputSize.x / 2) / ((ui32)timersLog.count + 1),
		(ui32)(((real)outputSize.y / 8) / ((real)1000 / 60)));

	const bool fill = true;
	ui32 value;
	v2ui barPosition(0, outputSize.y);
	for (uint i = 0; i < timersLog.count; i++, barPosition.x += barSize.x + 1, barPosition.y = outputSize.y)
	{
		ui32 nextColor = 2;

		DRAW_DEBUG_UI_BAR(TimerId::Update);
		DRAW_DEBUG_UI_BAR(TimerId::Render);
		DRAW_DEBUG_UI_BAR(TimerId::GuiDraw);
		DRAW_DEBUG_UI_BAR(TimerId::PostProcess);
		DRAW_DEBUG_UI_BAR(TimerId::Display);
	}

	// 16ms (60fps) line
	DrawRectangle(v2ui(0, outputSize.y - outputSize.y / 8), v2ui(outputSize.x/2, 1),
		v4b(0xff, 0xff, 0xff, 0xff), true, output, outputSize);
}

void UserInterface::DrawContextMenu(const v2ui& position, 
	array_of<uint32>& output, const v2ui& outputSize) const
{
	for (uint i = 0; i < contextMenuItems.count; ++i)
		DrawElement(contextMenuItems[i], position, output, outputSize);
}

void UserInterface::DrawElement(const GuiElement& element, const v2ui& position, 
	array_of<uint32>& output, const v2ui& outputSize) const
{
	switch (element.type)
	{
		case GuiElementType::GuiButton:
		case GuiElementType::GuiMenuItem:
		{
			v2ui elementPosition = element.position + position;

			DrawRectangle(elementPosition, element.size, 
				(element.highlighted) ? hilightedBackgroundColor : backgroundColor, true, output, outputSize);

			elementPosition.x += 10;
			elementPosition.y += element.size.y / 2;
			DrawString(elementPosition, element.text, textColor, output, outputSize);
		}
		break;
	}

	for (uint i = 0; i < element.childrenItems.count; ++i)
		if (element.childrenItems[i].visible)
			DrawElement(element.childrenItems[i], position, output, outputSize);
}

void UserInterface::DrawRectangle(v2ui position, v2ui size, v4b color, bool fill,
	array_of<uint32>& output, const v2ui& outputSize) const
{
	if (position.x > outputSize.x || position.y > outputSize.y)
		return;

	size.x = MIN2(outputSize.x, position.x + size.x);
	size.y = MIN2(outputSize.y, position.y + size.y);

	if (!fill)
	{
		rgba_as_uint32 newColor;
		for (uint x = position.x; x < size.x; x++)
		{
			uint outputIndex = (outputSize.y - position.y) * outputSize.x + x;
			newColor.Set(color);
			newColor.Add((rgba_as_uint32*)&output[outputIndex]);
			output[outputIndex] = newColor.GetUi32();

			outputIndex = (outputSize.y - (size.y - 1)) * outputSize.x + x;
			newColor.Set(color);
			newColor.Add((rgba_as_uint32*)&output[outputIndex]);
			output[outputIndex] = newColor.GetUi32();
		}

		for (uint y = position.y; y < size.y; y++)
		{
			uint outputIndex = (outputSize.y - y) * outputSize.x + position.x;
			newColor.Set(color);
			newColor.Add((rgba_as_uint32*)&output[outputIndex]);
			output[outputIndex] = newColor.GetUi32();

			outputIndex = (outputSize.y - y) * outputSize.x + size.x - 1;
			newColor.Set(color);
			newColor.Add((rgba_as_uint32*)&output[outputIndex]);
			output[outputIndex] = newColor.GetUi32();
		}
	}
	else
	{
		for (uint y = position.y; y < size.y; y++)
		{
			for (uint x = position.x; x < size.x; x++)
			{
				const uint outputIndex = (outputSize.y - y) * outputSize.x + x;

				rgba_as_uint32 newColor(color);
				newColor.Add((rgba_as_uint32*)&output[outputIndex]);

				output[outputIndex] = newColor.GetUi32();
			}
		}
	}
}

void UserInterface::DrawBitmap(const v2ui& position, const Bitmap& bitmap,
	array_of<uint32>& output, const v2ui& outputSize) const
{
	rgba_as_uint32* bitmapData = bitmap.data.ptr;
	for (uint y = 0; y < bitmap.size.y; y++)
	{
		if (position.y + y >= outputSize.y)
			break;

		for (uint x = 0; x < bitmap.size.x; x++)
		{
			if (position.x + x >= outputSize.x)
				break;

			const uint outputIndex = (outputSize.y - position.y - y) * outputSize.x + position.x + x;

			rgba_as_uint32 newColor = *bitmapData++;
			newColor.Add((rgba_as_uint32*)&output[outputIndex]);

			output[outputIndex] = newColor.GetUi32();
		}
	}
}

void UserInterface::DrawString(const v2ui& leftCenteredPosition, const String& text, const v4b color,
	array_of<uint32>& output, const v2ui& outputSize) const
{
	if (!font.glyphs.ptr)
		return;
	
	rgba_as_uint32* bitmapData = null;

	v2ui stringSize(font.glyphSize.x * (ui32)text.count, font.glyphSize.y);
	v2ui glyphPosition(leftCenteredPosition.x, leftCenteredPosition.y + stringSize.y / 2);
	for (uint i = 0; i < text.count; ++i)
	{
		if (text[i] == '\t')
		{
			glyphPosition.x += 4 * (font.glyphSize.x + font.spacing.x);
			continue;
		}

		glyphPosition.x += font.glyphs[text[i]].offset.x;
		glyphPosition.y += font.glyphs[text[i]].offset.y;
		
		bitmapData = font.glyphs[text[i]].bitmap.data.ptr;
		const v2ui& glyphSize = font.glyphs[text[i]].bitmap.size;
		for (uint y = 0; y < glyphSize.y; y++)
		{
			if (glyphPosition.y + y >= outputSize.y)
				break;

			for (uint x = 0; x < glyphSize.x; x++)
			{
				if (glyphPosition.x + x >= outputSize.x)
					break;

				const uint outputIndex = (outputSize.y - glyphPosition.y - y) * outputSize.x + glyphPosition.x + x;

				rgba_as_uint32 newColor = *bitmapData++;
				newColor *= color;
				newColor.Add((rgba_as_uint32*)&output[outputIndex]);

				output[outputIndex] = newColor.GetUi32();
			}
		}
		glyphPosition.x += font.glyphSize.x + font.spacing.x;

		glyphPosition.x -= font.glyphs[text[i]].offset.x;
		glyphPosition.y -= font.glyphs[text[i]].offset.y;
	}
}

void UserInterface::DrawString(const v2ui& leftCenteredPosition, char* text, const v4b color,
	array_of<uint32>& output, const v2ui& outputSize) const
{
	array_of<char> string;
	string.ptr = text;
	string.count = strlen(text);

	DrawString(leftCenteredPosition, string, color, output, outputSize);
}

void UserInterface::Update(AthenaStorage* athenaStorage, const v2ui& mousePosition)
{
	this->currentMousePosition = mousePosition;
	hilightedElement = null;

	if (visible)
	{
		UpdateDebugUi(athenaStorage->debugParameters);
		//UpdateContextMenu();
	}
}

void UserInterface::UpdateDebugUi(list_of<Parameter>& parameters)
{
	if (currentParameterId)
		return;

	hilightedParameterId = 0;

	v2ui linePosition = debugUiPosition;
	for (uint i = 1; i < parameters.currentCount; ++i)
	{
		const Parameter& p = parameters[i];
		v2ui parameterLineSize(
			((ui32)p.name.count + (p.IsRegion() ? 1 : 0)) * (font.glyphSize.x + font.spacing.x), 
			font.glyphSize.y);

		if (p.parentId)
		{
			if (parameters[p.parentId].collapsed)
				continue;
		
			linePosition.x += 4 * (font.glyphSize.x + font.spacing.x);
		}

		if (PointInRectangle(linePosition, parameterLineSize, currentMousePosition))
		{
			hilightedParameterId = i;
			break;
		}

		if (p.parentId)
			linePosition.x -= 4 * (font.glyphSize.x + font.spacing.x);

		linePosition.y += font.glyphSize.y + font.spacing.y;
	}
}

void UserInterface::UpdateContextMenu()
{
	for (uint i = 0; i < contextMenuItems.count; ++i)
	{
		contextMenuItems[i].visible = true;
		UpdateContextMenu(contextMenuItems[i]);
	}
}

void UserInterface::UpdateContextMenu(GuiElement& element)
{
	const v2ui localMousePosition = currentMousePosition - currentContextMenuPosition;

	if (element.type == GuiElementType::GuiMenuItem)
	{
		element.highlighted = PointInRectangle(element.position, element.size, localMousePosition) ||
			(element.childrenItems[0].visible && PointInChildren(element.childrenItems, localMousePosition));

		for (uint i = 0; i < element.childrenItems.count; ++i)
			element.childrenItems[i].visible = element.highlighted;
	}
	else
	{
		element.highlighted = element.visible && PointInRectangle(element.position, element.size, localMousePosition);
		if (element.highlighted)
			hilightedElement = &element;
	}

	for (uint i = 0; i < element.childrenItems.count; ++i)
		UpdateContextMenu(element.childrenItems[i]);
}

inline bool UserInterface::PointInChildren(const array_of<GuiElement>& elementArray, 
	const v2ui& cursorPosition) const
{
	return PointInRectangle(elementArray[0].position, 
		v2ui(elementArray[0].size.x, elementArray[0].size.y * (uint32)elementArray.count), cursorPosition);
}

bool UserInterface::PointInRectangle(const v2ui& start, const v2ui size, const v2ui& point) const
{
	if (point.x < start.x || point.y < start.y)
		return false;

	if (point.x > start.x + size.x || point.y > start.y + size.y)
		return false;

	return true;
}

void UserInterface::MouseButtonClicked(MouseButton::Enum mouseButton, v2ui mousePosition, AthenaStorage* storage)
{
	switch (mouseButton)
	{
		case MouseButton::Right:
		{
			if (visible && hilightedParameterId)
				ProcessMouseClick(mouseButton, mousePosition, storage);

			else if (!visible && sceneClickedFunction)
				sceneClickedFunction(mouseButton, mousePosition, storage);
		}
		break;

		case MouseButton::Left:
		{
			if (visible && hilightedElement != null)
			{
				LOG_DEBUG("%s-click on %s", MouseButton::GetString(mouseButton), 
					GuiElementId::GetString(hilightedElement->id));

				if (hilightedElement->buttonClickedFunction)
					hilightedElement->buttonClickedFunction(hilightedElement->id, storage);
			}
			else if (visible && hilightedParameterId)
				ProcessMouseClick(mouseButton, mousePosition, storage);

			else if (!visible && sceneClickedFunction)
				sceneClickedFunction(mouseButton, mousePosition, storage);
		}
		break;
	}
}

void UserInterface::MouseButtonState(MouseButton::Enum mouseButton, b32 isDown, v2ui mousePosition,
	AthenaStorage* storage)
{
	switch (mouseButton)
	{
		case MouseButton::Left:
		{
			if (isDown && visible && currentParameterId)
				ProcessMouseMovement(mouseButton, mousePosition, storage);

			else if (!isDown)
				currentParameterId = 0;
		}
		break;
	}
}

template <typename T>
void ProcessEnumParameter(MouseButton::Enum mouseButton, Parameter& p)
{
	if (mouseButton == MouseButton::Left && (*(int*)p.ptr) == (T::Count - 1))
	(*(int*)p.ptr) = 0;
	else if (mouseButton == MouseButton::Right && (*(int*)p.ptr) == 0)
		(*(int*)p.ptr) = T::Count - 1;
	else if (mouseButton == MouseButton::Left && (*(int*)p.ptr) < (T::Count - 1))
		(*(int*)p.ptr)++;
	else if (mouseButton == MouseButton::Right && (*(int*)p.ptr) > 0)
		(*(int*)p.ptr)--;
}

void UserInterface::ProcessMouseClick(MouseButton::Enum mouseButton, v2ui mousePosition, AthenaStorage* storage)
{
	Parameter& p = storage->debugParameters[hilightedParameterId];

	if (p.readOnly)
		return;

	if (p.IsRegion() && mouseButton == MouseButton::Left)
		p.collapsed = !p.collapsed;
	else
	{
		switch (p.type)
		{
			case Type::b32:
				*(b32*)p.ptr = (*(b32*)p.ptr) ? 0 : 1;
				break;

			case Type::uint:
			case Type::ui64:
				if (mouseButton == MouseButton::Left
					&& (*(uint*)p.ptr) < 0xffffffffffffffff
					&& (!p.maxValuePtr || (*(uint*)p.ptr) < (*(uint*)p.maxValuePtr - 1)))
					(*(uint*)p.ptr)++;
				else if (mouseButton == MouseButton::Right && (*(uint*)p.ptr) > 0)
					(*(uint*)p.ptr)--;
				break;

			case Type::ui32:
				if (mouseButton == MouseButton::Left 
					&& (*(ui32*)p.ptr) < 0xffffffff 
					&& (!p.maxValuePtr || (*(ui32*)p.ptr) < (*(ui32*)p.maxValuePtr - 1)))
					(*(ui32*)p.ptr)++;
				else if (mouseButton == MouseButton::Right && (*(ui32*)p.ptr) > 0)
					(*(ui32*)p.ptr)--;
				break;

			case Type::i32:
				if (mouseButton == MouseButton::Left)
					(*(i32*)p.ptr)++;
				else if (mouseButton == MouseButton::Right)
					(*(i32*)p.ptr)--;
				break;

			case Type::real:
				currentParameterId = hilightedParameterId;
				break;

			case Type::frameBufferEnum:
				ProcessEnumParameter<FrameBuffer>(mouseButton, p);
				break;

			case Type::rendererEnum:
				ProcessEnumParameter<Renderer>(mouseButton, p);
				break;

			case Type::renderingMethodEnum:
				ProcessEnumParameter<RenderingMethod>(mouseButton, p);
				break;

			case Type::tracingMethodEnum:
				ProcessEnumParameter<TracingMethod>(mouseButton, p);
				break;

			case Type::renderingModeEnum:
				ProcessEnumParameter<RenderingMode>(mouseButton, p);
				break;

			default:
				break;
		}
	}

	previousMousePosition = mousePosition;
}

void UserInterface::ProcessMouseMovement(MouseButton::Enum mouseButton, v2ui mousePosition, AthenaStorage* storage)
{
	Parameter& p = storage->debugParameters[currentParameterId];

	if (mouseButton == MouseButton::Left)
	{
		switch (p.type)
		{
			case Type::real:
			{
				const real increment = .01 * ((i32)mousePosition.y - (i32)previousMousePosition.y);
				*(real*)p.ptr += increment;

				previousMousePosition = mousePosition;
			}
			break;

			default:
				break;
		}
	}
}
