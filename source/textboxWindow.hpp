#pragma once
#include "dataType.hpp"
#include "define.hpp"
#include "dataStructure.hpp"
#include "platformWindows.hpp"
#include "graphicsGDI.hpp"

START_SCOPE(textboxWindow)

struct State
{
	String text;
};

void create(String* text, RECT* boundingBox, HWND parent, HWND* window);

END_SCOPE
