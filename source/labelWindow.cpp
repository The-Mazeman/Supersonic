#include "include.hpp"
#include "labelWindow.hpp"

START_SCOPE(labelWindow)

LRESULT windowCallback(HWND window, UINT message, WPARAM wParam, LPARAM lParam);

void create(Parameter* parameter, String* formatString, RECT* boundingBox, HWND parent, HWND* window)
{
	State* state = {};
	allocateMemory(sizeof(State), (void**)&state);
    state->parameter = *parameter;
    state->formatString = *formatString;

	createWindowClass(L"labelWindowClass", windowCallback);
	createChildWindow(L"labelWindowClass", parent, state, window);

    placeWindow(*window, boundingBox); 
}
void paintWindow(State* state, HWND window)
{
	PAINTSTRUCT paintStruct;
	HDC deviceContext = BeginPaint(window, &paintStruct);

	RECT* invalidRectangle = &paintStruct.rcPaint;
	rectangleFill(deviceContext, invalidRectangle, COLOR_BLACK);

    int width, height;
    getWindowDimension(window, &width, &height);
    *invalidRectangle = {0, 0, width, height};
	rectangleFrame(deviceContext, invalidRectangle, COLOR_WHITE);

    float value = state->parameter.value;
    WCHAR labelText[16] = {};
    WCHAR* format = state->formatString.string;
    swprintf(labelText, 16, format, value);

    uint characterCount = state->formatString.characterCount;
    String text = {labelText, characterCount};
    drawText(deviceContext, &text, invalidRectangle, COLOR_WHITE);

	EndPaint(window, &paintStruct);
}
void handleVerticalScroll(State* state, HWND window, int wheelDelta)
{
    float delta = state->parameter.delta;
    if(wheelDelta > 0)
    {
        delta *= -1;
    }
    float value = state->parameter.value;
    float minimum = state->parameter.minimum;
    float maximum = state->parameter.maximum;
    value += delta;
    if(value > maximum)
    {
        value = maximum;
    }
    if(value < minimum)
    {
        value = minimum;
    }
    InvalidateRect(window, 0, 0);
    HWND parent = GetAncestor(window, GA_PARENT);

    state->parameter.value = value;
    uint parameterId = state->parameter.id;
    SendMessage(parent, WM_PARAMETERCHANGE, parameterId, (LPARAM)&value);
}
void handleMouseWheel(State* state, HWND window, WPARAM wParam, LPARAM lParam)
{
    int keyState = GET_KEYSTATE_WPARAM(wParam);
    int wheelDelta = GET_WHEEL_DELTA_WPARAM(wParam);

    switch(keyState)
    {
        case 0:
        {
            handleVerticalScroll(state, window, wheelDelta);
            break;
        }
    }
}
void getParameterValue(State* state, WPARAM wParam)
{
    float* value = (float*)wParam;
    *value = state->parameter.value;
}
LRESULT windowCallback(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
	State* state = (State*)GetProp(window, L"state");
	switch(message)
	{
        case WM_PAINT:
        {
            paintWindow(state, window);
            break;
        }
        case WM_MOUSEWHEEL:
        {
            handleMouseWheel(state, window, wParam, lParam);
            return 0;
        }
        case WM_GETPARAMETERVALUE:
        {
            getParameterValue(state, wParam);
            break;
        }
        case WM_NCHITTEST:
        {
            return HTCLIENT;
        }
	}
	return defaultWindowCallback(window, message, wParam, lParam);
}

END_SCOPE
