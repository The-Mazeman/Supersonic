#include "include.hpp"
#include "contextMenuWindow.hpp"

START_SCOPE(contextMenuWindow)

LRESULT CALLBACK windowCallback(HWND window, UINT message, WPARAM wParam, LPARAM lParam);

void getMaxCharacterCount(String* stringArray, uint stringCount, uint* maxCharacterCount)
{
    uint max = 0;
    for(uint i = 0; i != stringCount; ++i)
    {
        uint characterCount = stringArray[i].characterCount;
        if(characterCount >= max)
        {
            max = characterCount;
        }
    }
    *maxCharacterCount = max;
}
void create(HWND ownerWindow, String* menuOptionArray, uint menuOptionCount, HWND* window)
{
    State* state = {};
    allocateMemory(sizeof(State), (void**)&state);
    state->menuOptionArray = menuOptionArray;
    state->menuOptionCount = menuOptionCount;
    state->ownerWindow = ownerWindow;

    createWindowClass(L"contextMenuWindowClass", windowCallback);
    createWindow(L"contextMenuWindowClass", state, window);
    SetWindowTheme(*window, L"", L"");

    uint maxCharacterCount;
    getMaxCharacterCount(menuOptionArray, menuOptionCount, &maxCharacterCount);
    int width = (int)maxCharacterCount * 8; 
    int height = (int)menuOptionCount * 16 + 8;
    placeWindow(*window, 0, 0, width, height);
    ShowWindow(*window, SW_HIDE);
}
void paint(State* state, HWND window)
{
	PAINTSTRUCT paintStruct;
	HDC deviceContext = BeginPaint(window, &paintStruct);

	RECT* invalidRectangle = &paintStruct.rcPaint;
	rectangleFill(deviceContext, invalidRectangle, COLOR_BLACK);
	rectangleFrame(deviceContext, invalidRectangle, COLOR_WHITE);

	HFONT font = CreateFont(15, 0, 0, 0, 0, 0, 0, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
	SelectObject(deviceContext, font);

	SetTextAlign(deviceContext, TA_LEFT);
	SetBkMode(deviceContext, TRANSPARENT);
	SetTextColor(deviceContext, COLOR_WHITE);

    String* menuOptionArray = state->menuOptionArray;
    uint menuOptionCount = state->menuOptionCount;
    for(uint i = 0; i != menuOptionCount; ++i)
    {
		int stringLength = (int)menuOptionArray[i].characterCount;
        WCHAR* string = menuOptionArray[i].string;
		TextOut(deviceContext, 8, ((int)i * 16) + 4, string, stringLength);
    }

	EndPaint(window, &paintStruct);
}
void handleSizeLimit(LPARAM lParam)
{
    MINMAXINFO* sizeInfo = (MINMAXINFO*)lParam;
    sizeInfo->ptMinTrackSize.x = 0;
    sizeInfo->ptMinTrackSize.y = 0;
}
void closeMenu(State* state, HWND window)
{
    ShowWindow(window, SW_HIDE);
}
void pickOption(State* state, LPARAM lParam)
{
    int y = GET_Y_LPARAM(lParam);
    int optionId = y / 16;
    HWND ownerWindow = state->ownerWindow;
    SendMessage(ownerWindow, WM_CONTEXTMENUOPTION, (WPARAM)optionId, 0);
}
LRESULT CALLBACK windowCallback(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
    State* state = (State*)GetProp(window, L"state");
    switch(message)
    {
        case WM_PAINT:
        {
            paint(state, window);
            break;
        }
        case WM_GETMINMAXINFO:
        {
            handleSizeLimit(lParam);
            break;
        }
        case WM_LBUTTONUP:
        {
            pickOption(state, lParam);
            closeMenu(state, window);
            break;
        }
        case WM_CAPTURECHANGED:
        {
            closeMenu(state, window);
            break;
        }
		case WM_NCACTIVATE:
		{
			lParam = -1;
            return 0;
		}
        case WM_NCCALCSIZE:
		case WM_NCPAINT:
		case WM_SETICON:
		case WM_SETTEXT:
		case WM_NCUAHDRAWCAPTION:
		case WM_NCUAHDRAWFRAME:
		{
			return 0;
		}
    }
    return defaultWindowCallback(window, message, wParam, lParam);
}
END_SCOPE
