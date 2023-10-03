#include "header.h"
#include "fader.h"

START_SCOPE(fader)

LRESULT windowCallback(HWND window, UINT message, WPARAM wParam, LPARAM lParam);

void create(HWND parent, HWND* window, WindowPosition* position, uint faderId)
{
	State* state = {};
	allocateSmallMemory(sizeof(State), (void**)&state);
	state->position.x = position->x;
	state->position.y = position->y;
	state->position.width = position->width;
	state->position.height = position->height;
	state->faderId = faderId;

	createWindowClass(L"faderWindowClass", windowCallback);
	createChildWindow(L"faderWindowClass", parent, window, state);

	int width = position->width;
	int height = position->height;
	int rectangleWidth = 32;
	int rectangleHeight = 64;
	int rectangleX = (width / 2) - (rectangleWidth / 2);
	int rectangleY = (height / 2) - (rectangleHeight/ 2);

	state->rectanglePosition.x = rectangleX;
	state->rectanglePosition.y = rectangleY;
	state->rectanglePosition.width = rectangleWidth;
	state->rectanglePosition.height = rectangleHeight;
	state->upperLimit = 16 + rectangleHeight / 2;
	state->lowerLimit = height - (rectangleHeight / 2) - 16;
	rectangle::create(*window, &state->rectangle, &state->rectanglePosition);
}
void paintWindow(State* state, HWND window)
{
	PAINTSTRUCT paintStruct;
	HDC deviceContext = BeginPaint(window, &paintStruct);

	RECT* invalidRectangle = &paintStruct.rcPaint;
	rectangleFill(deviceContext, invalidRectangle, COLOR_BLACK);

	int width = state->position.width;
	int height = state->position.height;
	RECT windowRectangle = {0, 0, width, height};
	rectangleFrame(deviceContext, &windowRectangle, COLOR_WHITE);

	SelectObject(deviceContext, GetStockObject(DC_PEN));
	SetDCPenColor(deviceContext, COLOR_WHITE);

	MoveToEx(deviceContext, width / 2, 16, 0);
	LineTo(deviceContext, width / 2, height - 16);

	EndPaint(window, &paintStruct);
}
void handleResize(State* state, HWND window, WPARAM wParam)
{
	int x = state->position.x;
	int y = state->position.y;
	POINT* parent = (POINT*)wParam;
	int width = parent->x;
	int height = state->position.height;

	placeWindow(window, x, y, width, height);
	
	HWND rectangle = state->rectangle;
	SendMessage(rectangle, WM_RESIZE, 0, 0);
}
void handleMouseDrag(State* state, HWND window, WPARAM wParam, LPARAM lParam)
{
	if(wParam == MK_LBUTTON)
	{
		int mouseY = GET_Y_LPARAM(lParam); 
		int width = state->rectanglePosition.width;
		int height = state->rectanglePosition.height;
		int x = state->rectanglePosition.x;
		int upperLimit = state->upperLimit;
		int lowerLimit = state->lowerLimit;
		int range = lowerLimit - upperLimit;
		if(mouseY < upperLimit)
		{
			mouseY = upperLimit;
		}
		if (mouseY > lowerLimit)
		{
			mouseY = lowerLimit;
		}
		int y = mouseY - (height / 2);
		int faderValue =(-1 * (mouseY - upperLimit)) + (range / 2);

		HWND rectangle = state->rectangle;
		placeWindow(rectangle, x, y, width, height);
		HWND parent = GetAncestor(window, GA_PARENT);
		uint faderId = state->faderId;
		SendMessage(parent, WM_FADERMOVE, (WPARAM)faderId, faderValue);
	}
}
void setControl(State* state, WPARAM wParam)
{
	int upperLimit = state->upperLimit;
	int lowerLimit = state->lowerLimit;
	int range = lowerLimit - upperLimit;

	__m256* output = (__m256*)wParam;
	float gain = output->m256_f32[0];
	float dbFloat = (float)log10(gain) * 20.0f;
	int db = (int)(dbFloat * 10.0f);
	int y = (range / 2) - db + 16;

	int width = state->rectanglePosition.width;
	int height = state->rectanglePosition.height;

	int x = state->rectanglePosition.x;
	HWND rectangle = state->rectangle;
	placeWindow(rectangle, x, y, width, height);
	ShowWindow(rectangle, SW_SHOW);
}
LRESULT windowCallback(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
	State* state = (State*)GetProp(window, L"state");
	switch (message)
	{
		case WM_CREATE:
		{
			setState(window, lParam);
			break;
		}
		case WM_PAINT:
		{
			paintWindow(state, window);
			break;
		}
		case WM_SETCONTROL:
		{
			setControl(state, wParam);
			break;
		}
		case WM_MOUSEMOVE:
		{
			handleMouseDrag(state, window, wParam, lParam);
			break;
		}
		case WM_RESIZE:
		{
			handleResize(state, window, wParam);
			break;
		}
	}
	return DefWindowProc(window, message, wParam, lParam);
}

END_SCOPE