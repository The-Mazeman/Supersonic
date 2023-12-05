#include "include.hpp"
#include "mainWindow.hpp"

#define WINDOW_WIDTH  1280
#define WINDOW_HEIGHT  720 

START_SCOPE(mainWindow)
    
LRESULT CALLBACK windowCallback(HWND window, UINT message, WPARAM wParam, LPARAM lParam);

void setAttribute(HWND window)
{
    SetWindowTheme(window, L"", L"");
    DragAcceptFiles(window, 1);
}
void create(GlobalState* globalState)
{
    int width = WINDOW_WIDTH;
    int height = WINDOW_HEIGHT;
    State* state = {};
    allocateMemory(sizeof(State), (void**)&state);
    state->globalState = globalState;
    state->trackCount = 0;
    state->playing = 0;

    createDynamicArray(&state->audioClipContainerArrayHandle, sizeof(void*));

    HWND window;
    createWindowClass(L"mainWindowClass", windowCallback);
    createWindow(L"mainwindowClass", state, &window);

	setAttribute(window);
    SendMessage(window, WM_CREATECHILD, 0, 0);
    placeWindow(window, 0, 0, width, height);
}
void paint(HWND window)
{
	PAINTSTRUCT paintStruct;
	HDC deviceContext = BeginPaint(window, &paintStruct);

	RECT* invalidRectangle = &paintStruct.rcPaint;
	rectangleFill(deviceContext, invalidRectangle, COLOR_BLACK);

	EndPaint(window, &paintStruct);
}
LRESULT handleHitTesting(HWND window, LPARAM lParam)
{
	int cursorX = GET_X_LPARAM(lParam);
	int cursorY = GET_Y_LPARAM(lParam);

	int a, b, c, d;
	getWindowRectangle(window, &a, &b, &c, &d);

	if(cursorY < b + 16)
	{
		return HTCAPTION;
	}
	if(cursorX < a + 8)
	{
		return HTLEFT;
	}
	if(cursorX > c - 8)
	{
		return HTRIGHT;
	}
	if(cursorY > d - 8)
	{
		return HTBOTTOM;
	}
	return HTCLIENT;
}
void createChild(State* state, HWND window)
{
    GlobalState* globalState = state->globalState;
    HWND* titleBar = &state->titleBar; 
    titleBarWindow::create(globalState, window, titleBar);

    rulerWindow::create(globalState, window, &state->ruler);
    clipAreaWindow::create(globalState, window, &state->clipArea);
    sidebarWindow::create(globalState, window, &state->sidebar, state->clipArea);
}
void handleSizeChange(State* state, LPARAM lParam)
{
    HWND* childWindow = &state->titleBar;
    for(uint i = 0; i != 4; ++i)
    {
        SendMessage(childWindow[i], WM_RESIZE, 0, lParam);
    }
}
BOOL CALLBACK resizeAudioClipWindow(HWND window, LPARAM lParam)
{
    SendMessage(window, WM_RESIZE, 0, lParam);
    return 1;
}
void horizontalScroll(State* state, int wheelDelta)
{
    int offsetX = (int)state->globalState->offsetX;
    if(offsetX + wheelDelta < 0)
    {
        wheelDelta = offsetX * -1; 
    }
    HWND ruler = state->ruler;
    ScrollWindowEx(ruler, -wheelDelta, 0, 0, 0, 0, 0, SW_INVALIDATE);

    HWND clipArea = state->clipArea;
    ScrollWindowEx(clipArea, -wheelDelta, 0, 0, 0, 0, 0, SW_INVALIDATE | SW_SCROLLCHILDREN);

    offsetX += wheelDelta;
    state->globalState->offsetX = (uint)offsetX;
}
void handleHorizontalScroll(State* state, WPARAM wParam)
{
    int wheelDelta = GET_WHEEL_DELTA_WPARAM(wParam);
    horizontalScroll(state, wheelDelta);
}
void handleZoom(State* state, int wheelDelta, LPARAM lParam)
{
    int mouseX = GET_X_LPARAM(lParam);
    POINT mouse = {mouseX, 0};
    HWND ruler = state->ruler;
    ScreenToClient(ruler, &mouse);

    int offsetX = (int)state->globalState->offsetX;
    mouseX = mouse.x + offsetX;

    wheelDelta /= WHEEL_DELTA;
    int framesPerPixel = (int)state->globalState->framesPerPixel;
    int scaler = (framesPerPixel / 4) + 2;
    scaler *= wheelDelta;
    int newFramesPerPixel = framesPerPixel - scaler;
    if(newFramesPerPixel < 0)
    {
        return;

    }
    state->globalState->framesPerPixel = (uint)newFramesPerPixel;

    float changeRatio = (float)framesPerPixel / (float)newFramesPerPixel;
    float newMouseX = changeRatio * (float)mouseX;
    int deltaOffsetX = (int)newMouseX -  mouseX;
    horizontalScroll(state, deltaOffsetX);

    offsetX = (int)state->globalState->offsetX;

    HWND clipArea = state->clipArea;
    POINT value = {newFramesPerPixel, offsetX};
    EnumChildWindows(clipArea, resizeAudioClipWindow, (LPARAM)&value);
    SendMessage(clipArea, WM_MOVECURSOR, 0, 0);

    InvalidateRect(ruler, 0, 0);
    InvalidateRect(clipArea, 0, 0);
}
void handleVerticalScroll(State* state, int wheelDelta)
{
    int offsetX = (int)state->globalState->offsetX;
    int offsetY = (int)state->globalState->offsetY;

    if(offsetY - wheelDelta < 0)
    {
        wheelDelta  = -offsetY;
    }

    HWND sidebar = state->sidebar;
    ScrollWindowEx(sidebar, 0, wheelDelta, 0, 0, 0, 0, SW_INVALIDATE | SW_SCROLLCHILDREN);

    HWND clipArea = state->clipArea;
    ScrollWindowEx(clipArea, 0, wheelDelta, 0, 0, 0, 0, SW_INVALIDATE | SW_SCROLLCHILDREN);
    state->globalState->offsetY = (uint)(offsetY - wheelDelta);
}
void trackHeightChange(State* state, int wheelDelta)
{
    int trackHeight = (int)state->globalState->trackHeight;
    trackHeight += wheelDelta;
    state->globalState->trackHeight = (uint)trackHeight;

    HWND sidebarWindow = state->sidebar;
    SendMessage(sidebarWindow, WM_RESIZETRACKHEIGHT, 0, 0);

    HWND clipAreaWindow = state->clipArea;
    SendMessage(clipAreaWindow, WM_RESIZETRACKHEIGHT, 0, 0);
}
void handleMouseWheel(State* state, WPARAM wParam, LPARAM lParam)
{
    int keyState = GET_KEYSTATE_WPARAM(wParam);
    int wheelDelta = GET_WHEEL_DELTA_WPARAM(wParam);

    switch(keyState)
    {
        case 0:
        {
            handleVerticalScroll(state, wheelDelta);
            break;
        }
        case MK_CONTROL:
        {
            handleZoom(state, wheelDelta, lParam);
            break;
        }
        case MK_SHIFT:
        {
            trackHeightChange(state, wheelDelta);
            break;
        }
    }
}
void getTrackUnderMouse(HWND window, int offsetY, uint trackHeight, uint* trackNumber)
{
    POINT mouse;
    GetCursorPos(&mouse);
    ScreenToClient(window, &mouse);
    int x = mouse.x;
    SendMessage(window, WM_SETDROPLOCATION, (WPARAM)x, 0);

    int y = mouse.y;
    y += (offsetY * -1);
    *trackNumber = (int)y / trackHeight;
}
void createNewAudioTrack(State* state, uint newTrackCount)
{
    HWND sidebar = state->sidebar;
    SendMessage(sidebar, WM_CREATEAUDIOTRACK, newTrackCount, 0);

    HWND clipArea = state->clipArea;
    SendMessage(clipArea, WM_CREATEAUDIOTRACK, newTrackCount, 0);

    void* audioClipContainerArrayHandle = state->audioClipContainerArrayHandle;
    resizeArray(audioClipContainerArrayHandle, newTrackCount);
    for(uint i = 0; i != newTrackCount; ++i)
    {
        void* audioClipContainer = {};
        createDynamicArray(&audioClipContainer, sizeof(AudioClip*));
        appendElement(audioClipContainerArrayHandle, &audioClipContainer);
    }
    state->trackCount += newTrackCount;
}
void checkExtension(String* filePath, String* extension)
{
	WCHAR* pathString = filePath->string;
	uint64 characterCount = filePath->characterCount;

	pathString += characterCount - 1;
	const WCHAR* dot = L".";
	uint extensionCharacterCount = {};

	for(uint64 i = 0; i != characterCount; --i)
	{
		if(*pathString == *dot)
		{
			break;
		}
		--pathString;
		++extensionCharacterCount;
	}

	++pathString;
	WCHAR* extensionString = extension->string;

	for(uint i = 0; i != extensionCharacterCount; ++i)
	{
		if(i > extension->characterCount)
		{
			break;
		}
		if(pathString[i] != extensionString[i])
		{
			filePath->characterCount = 0;
			break;
		}
	}
}
void getFilePathArray(WPARAM wParam, String** filePathArray, uint* fileCount)
{
	uint filePathCount;
	getFileCount(wParam, &filePathCount);

	String* filePath;
	allocateMemory(sizeof(String) * filePathCount, (void**)&filePath);
	*filePathArray = filePath;
	const WCHAR* extensionString = L"wav";

	String extension = {};
	extension.string = (WCHAR*)extensionString;
	extension.characterCount = 3;
	for(uint i = 0; i != filePathCount; ++i)
	{
		WCHAR* pathString = {};
		allocateMemory(sizeof(WCHAR) * 256, (void**)&pathString);

        uint characterCount = {};
		getFilePath(wParam, pathString, &characterCount, i);

		filePath->string = pathString;
        filePath->characterCount = characterCount; 

		checkExtension(filePath, &extension);
		if(filePath->characterCount == 0)
		{
            freeMemory(pathString);
			continue;
		}

		++filePath;
		++(*fileCount);
	}
}
void handleFileDrop(State* state, WPARAM wParam)
{

    POINT mouse;
    GetCursorPos(&mouse);

    HWND clipArea = state->clipArea;
    ScreenToClient(clipArea, &mouse);

    int offsetX = (int)state->globalState->offsetX;
    int offsetY = (int)state->globalState->offsetY;

    int y = mouse.y;
    y += offsetY;

    uint trackHeight = state->globalState->trackHeight;
    uint trackNumber = (int)y / trackHeight;

	uint fileCount = {};
    String* filePathArray = {};
	getFilePathArray(wParam, &filePathArray, &fileCount);

    uint framesPerPixel = state->globalState->framesPerPixel;
    int x = mouse.x;
    uint64 startFrame = (x + offsetX) * framesPerPixel; 
    AudioClip* audioClipArray = {};
    audioClip::create(filePathArray, startFrame, fileCount, &audioClipArray);

    uint trackCount = state->trackCount;
    if(trackNumber > trackCount)
    {
        trackNumber = trackCount;
    }
    uint newTrackCount = {};
    if(trackNumber + fileCount > trackCount)
    {
        newTrackCount = (trackNumber + fileCount) - trackCount;
        createNewAudioTrack(state, newTrackCount);
    }

    void* audioClipContainerArrayHandle = state->audioClipContainerArrayHandle;
    void** audioClipContainerArray = {};
    getArrayStart(audioClipContainerArrayHandle, (void**)&audioClipContainerArray);
    for(uint i = 0; i != fileCount; ++i)
    {
        SendMessage(clipArea, WM_FILEDROP, (WPARAM)&audioClipArray[i], trackNumber + i);
        void* audioClipContainer = audioClipContainerArray[trackNumber + i];
        AudioClip* audioClip = &audioClipArray[i];
        appendElement(audioClipContainer, &audioClip);
    }
    freeMemory(filePathArray);
}
void togglePlayback(State* state)
{
    HWND sidebar = state->sidebar;
    if(state->playing)
    {
        state->playing = 0;
        SendMessage(sidebar, WM_PAUSE, 0, 0);
    }
    else
    {
        void* audioClipContainerArrayHandle = state->audioClipContainerArrayHandle;
        SendMessage(sidebar, WM_PLAY, (WPARAM)audioClipContainerArrayHandle, 0);
        state->playing = 1;
    }
}
void handleKeyboard(State* state, WPARAM wParam)
{
    if(wParam == VK_SPACE)
    {
        togglePlayback(state);
    }
}
void createNewBusTrack(State* state, WPARAM wParam)
{
    uint newTrackCount = (uint)wParam;
    HWND sidebar = state->sidebar;
    SendMessage(sidebar, WM_CREATEBUSTRACK, newTrackCount, 0);
}
LRESULT CALLBACK windowCallback(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
    State* state = (State*)GetProp(window, L"state");
    switch(message)
    {
        case WM_CREATECHILD:
        {
            createChild(state, window);
            break;
        }
        case WM_PAINT:
        {
            paint(window);
            break;
        }
        case WM_SIZE:
        {
            handleSizeChange(state, lParam);
            break;
        }
        case WM_MOUSEHWHEEL:
        {
            handleHorizontalScroll(state, wParam);
            break;
        }
        case WM_MOUSEWHEEL:
        {
            handleMouseWheel(state, wParam, lParam);
            break;
        }
        case WM_KEYDOWN:
        {
            handleKeyboard(state, wParam);
            break;
        }
        case WM_DROPFILES:
        {
            handleFileDrop(state, wParam);
            break;
        }
        case WM_CREATEBUSTRACK:
        {
            createNewBusTrack(state, wParam);
            break;
        }
        case WM_CREATEAUDIOTRACK:
        {
            createNewAudioTrack(state, (uint)wParam);
            break;
        }
        case WM_NCCALCSIZE:
		{
			return 0;
		}
		case WM_NCACTIVATE:
		{
			lParam = -1;
            return 0;
		}
		case WM_NCPAINT:
		case WM_SETICON:
		case WM_SETTEXT:
		case WM_NCUAHDRAWCAPTION:
		case WM_NCUAHDRAWFRAME:
		{
			return 0;
		}
		case WM_NCHITTEST:
		{
			return handleHitTesting(window, lParam);
		}
    }
    return defaultWindowCallback(window, message, wParam, lParam);
}

END_SCOPE
