#pragma once
#include "dataType.hpp"
#include "define.hpp"
#include "dataStructure.hpp"
#include "platformWindows.hpp"
#include "graphicsGDI.hpp"
#include "audioUtilities.hpp"

START_SCOPE(labelWindow)

struct State
{
    Parameter parameter;
    String formatString;
};

void create(Parameter* parameter, String* formatString, RECT* boundingBox, HWND parent, HWND* window);

END_SCOPE
