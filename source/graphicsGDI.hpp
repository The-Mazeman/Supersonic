#pragma once
#include "define.hpp"
#include "dataType.hpp"
#include "dataStructure.hpp"

void rectangleFrame(HDC deviceContext, RECT* invalidRectangle, uint color);
void rectangleFill(HDC deviceContext, RECT* invalidRectangle, uint color);
void drawGrid(HDC deviceContext, RECT* invalidRectangle, float spacing, int offsetX);
void drawMarking(HDC deviceContext, RECT* invalidRectangle, float spacing, int offsetX, int multiplier);
void drawLine(HDC deviceContext, POINT* start, POINT* end);
void drawText(HDC deviceContext, String* text, RECT* boundingBox, uint color);
