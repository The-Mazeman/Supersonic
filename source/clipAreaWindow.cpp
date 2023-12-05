#include "include.hpp"
#include "clipAreaWindow.hpp"

START_SCOPE(clipAreaWindow)

LRESULT CALLBACK windowCallback(HWND window, UINT message, WPARAM wParam, LPARAM lParam);

void create(GlobalState* globalState, HWND parent, HWND* window)
{
    State* state = {};
    allocateMemory(sizeof(State), (void**)&state);
    state->globalState = globalState;
	state->x = (int)globalState->sidebarWidth;
	state->y = (int)(globalState->titleBarHeight + globalState->rulerHeight);
    state->scalar = 1;
    state->selectedChild = 0;

    createDynamicArray(&state->audioClipWindowContainerArrayHandle, sizeof(void*));

    createWindowClass(L"clipAreaWindowClass", windowCallback);
    createChildWindow(L"clipAreaWindowClass", parent, state, window);

    timelineCursorWindow::create(*window, &state->timelineCursorWindow);
}
void paint(State* state, HWND window)
{
	PAINTSTRUCT paintStruct;
	HDC deviceContext = BeginPaint(window, &paintStruct);

	RECT* invalidRectangle = &paintStruct.rcPaint;
	rectangleFill(deviceContext, invalidRectangle, COLOR_BLACK);

    int bottom = invalidRectangle->bottom - 1;
	int left = invalidRectangle->left;
	int right = invalidRectangle->right;

    POINT start = {left, bottom};
    POINT end = {right, bottom};

    sint64 framesPerPixel = state->globalState->framesPerPixel;
    int sampleRate = (int)state->globalState->sampleRate;

    float spacing = (float)sampleRate / (float)framesPerPixel;
    int offsetX = (int)state->globalState->offsetX;
	left += offsetX;
	right += offsetX;

    int scalar = state->scalar;
    spacing *= (float)scalar;
    if(spacing < 64)
    {
        scalar *= 2;
        spacing *= 2;
        state->scalar = scalar;
    }
    if(spacing > 128 && scalar > 1)
    {
        scalar /= 2;
        spacing /= 2;
        state->scalar = scalar;
    }

	int leftMarking = (int)((float)left / spacing);
	int rightMarking = (int)((float)right / spacing);

    invalidRectangle->left = leftMarking;
    invalidRectangle->right = rightMarking;

    SelectObject(deviceContext, GetStockObject(DC_PEN));
	SetDCPenColor(deviceContext, COLOR_GREY);

    drawGrid(deviceContext, invalidRectangle, spacing, offsetX);

	EndPaint(window, &paintStruct);
}
void handleResize(State* state, HWND window, LPARAM lParam)
{
    int x = state->x;
	int y = state->y;
    int width = LOWORD(lParam) - x;
    int height = HIWORD(lParam) - y;
    placeWindow(window, x, y, width, height);

    HWND timelineCursorWindow = state->timelineCursorWindow;
    uint64 readCursor = state->globalState->readCursor;;
    uint framesPerPixel = state->globalState->framesPerPixel;
    x = (int)(readCursor / framesPerPixel);
    placeWindow(timelineCursorWindow, x, 0, 1, height);
}
void addClip(State* state, HWND window, WPARAM wParam, LPARAM lParam)
{
    AudioClip* audioClip = (AudioClip*)wParam;
    uint trackNumber = (uint)lParam;
    int trackHeight = (int)state->globalState->trackHeight;

    int offsetX = (int)state->globalState->offsetX;
    int offsetY = (int)state->globalState->offsetY;
    uint framesPerPixel = (uint)state->globalState->framesPerPixel;

    uint64 startFrame = audioClip->startFrame;

    int x = (int)(startFrame / framesPerPixel) - offsetX;
    int y = ((int)trackNumber * trackHeight) + offsetY;
    uint64 frameCount = audioClip->frameCount;
    int width = (int)(frameCount / framesPerPixel);
    RECT boundingBox = {x, y, width, trackHeight};

    HWND audioClipWindow = {};
    audioClipWindow::create(audioClip, &boundingBox, window, &audioClipWindow);

    void* audioClipWindowContainerArrayHandle = state->audioClipWindowContainerArrayHandle;
    void** audioClipWindowContainerArray = {};
    getArrayStart(audioClipWindowContainerArrayHandle, (void**)&audioClipWindowContainerArray);

    void* audioClipContainer = audioClipWindowContainerArray[trackNumber];
    appendElement(audioClipContainer, (char*)&audioClipWindow);
}
void handleMouseMove(State* state, WPARAM wParam, LPARAM lParam)
{
    if(wParam == MK_LBUTTON)
    {
        int x = GET_X_LPARAM(lParam);
        int y = GET_Y_LPARAM(lParam);

        HWND selectedChild = state->selectedChild;

        int trackHeight = (int)state->globalState->trackHeight;
        int trackNumber = y / trackHeight;
        y = trackHeight * trackNumber;

        int width, height;
        getWindowDimension(selectedChild, &width, &height);

        x -= (width / 2);
        placeWindow(selectedChild, x, y, width, height); 
    }
}
void handleMouseLeftClick(State* state, HWND window, LPARAM lParam)
{
    int x = GET_X_LPARAM(lParam);
    int y = GET_Y_LPARAM(lParam);
    POINT point = {x, y};
    HWND selectedChild = ChildWindowFromPoint(window, point);
    if(selectedChild != window)
    {
        state->selectedChild = selectedChild;
    }
    else
    {
        HWND timelineCursorWindow = state->timelineCursorWindow;
        int width, height;
        getWindowDimension(window, &width, &height);
        placeWindow(timelineCursorWindow, x, 0, 1, height);

        uint offsetX = state->globalState->offsetX;
        uint framesPerPixel = state->globalState->framesPerPixel;
        uint64 readCursor = (uint64)(x + offsetX) * framesPerPixel;
        state->globalState->readCursor = readCursor;
    }
}
void resizeTrackHeight(State* state, HWND window)
{
    void* audioClipWindowContainerArrayHandle = state->audioClipWindowContainerArrayHandle; 
    void** audioClipWindowContainerArray = {};
    uint audioTrackCount = {};
    getArray(audioClipWindowContainerArrayHandle, (void**)&audioClipWindowContainerArray, &audioTrackCount);
    int trackHeight = (int)state->globalState->trackHeight;
    int offsetY = (int)state->globalState->offsetY;
    for(uint i = 0; i != audioTrackCount; ++i)
    {
        void* audioClipWindowContainer = audioClipWindowContainerArray[i];
        HWND* audioClipWindowArray = {};
        uint audioClipWindowCount = {};
        getArray(audioClipWindowContainer, (void**)&audioClipWindowArray, &audioClipWindowCount);
        for(uint j = 0; j != audioClipWindowCount; ++j)
        {
            int x, y;
            getWindowPosition(audioClipWindowArray[j], &x, &y);

            POINT point = {x, y};
            HWND parent = GetAncestor(audioClipWindowArray[j], GA_PARENT);
            ScreenToClient(parent, &point);
            x = point.x;
            y = (int)i * trackHeight; 
            y -= offsetY;

            int width, height;
            getWindowDimension(audioClipWindowArray[j], &width, &height);

            placeWindow(audioClipWindowArray[j], x, y, width, trackHeight);
        }
    }
}
LRESULT handleHitTesting(HWND window, LPARAM lParam)
{
	int cursorX = GET_X_LPARAM(lParam);
	int cursorY = GET_Y_LPARAM(lParam);

	int a, b, c, d;
	getWindowRectangle(window, &a, &b, &c, &d);

	if(cursorX > c - 8)
	{
		return HTTRANSPARENT;
	}
    else if(cursorY > d - 8)
	{
		return HTTRANSPARENT;
	}
	return HTCLIENT;
}
void moveCursor(State* state)
{
    uint64 readCursor = state->globalState->readCursor;
    uint framesPerPixel = state->globalState->framesPerPixel;
    int x = (int)(readCursor / framesPerPixel);
    int offsetX = (int)state->globalState->offsetX;
    x -= offsetX;

    HWND timelineCursorWindow = state->timelineCursorWindow;
    int width, height;
    getWindowDimension(timelineCursorWindow, &width, &height);
    placeWindow(timelineCursorWindow, x, 0, width, height);
}
void handleTimer(State* state, WPARAM wParam)
{
    uint64 timelineCursorBasePosition = state->timelineCursorBasePosition;
    uint64 framesElapsed = wParam;
    uint64 currentPosition = timelineCursorBasePosition + framesElapsed;

    uint framesPerPixel = state->globalState->framesPerPixel;
    int offsetX = (int)state->globalState->offsetX;
    int x = (int)(currentPosition / framesPerPixel);
    x -= offsetX;

    int width, height;
    HWND timelineCursorWindow = state->timelineCursorWindow;
    getWindowDimension(timelineCursorWindow, &width, &height);
    placeWindow(timelineCursorWindow, x, 0, width, height);
}
void prepareTimer(State* state)
{
    state->timelineCursorBasePosition = state->globalState->readCursor;
}
void createAudioTrack(State* state, WPARAM wParam)
{
    uint newTrackCount = (uint)wParam;
    void* audioClipWindowContainerArrayHandle = state->audioClipWindowContainerArrayHandle;
    resizeArray(audioClipWindowContainerArrayHandle, newTrackCount);
    for(uint i = 0; i != newTrackCount; ++i)
    {
        void* audioClipWindowContainer = {};
        createDynamicArray(&audioClipWindowContainer, sizeof(HWND));
        appendElement(audioClipWindowContainerArrayHandle, &audioClipWindowContainer);
    }
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
        case WM_FILEDROP:
        {
            addClip(state, window, wParam, lParam);
            break;
        }
        case WM_CREATEAUDIOTRACK:
        {
            createAudioTrack(state, wParam);
            break;
        }
        case WM_MOVECURSOR:
        {
            moveCursor(state);
            break;
        }
        case WM_RESIZETRACKHEIGHT:
        {
            resizeTrackHeight(state, window);
            break;
        }
        case WM_RESIZE:
        {
            handleResize(state, window, lParam);
            break;
        }
        case WM_LBUTTONDOWN:
        {
            handleMouseLeftClick(state, window, lParam);
            break;
        }
        case WM_MOUSEMOVE:
        {
            handleMouseMove(state, wParam, lParam);
            break;
        }
        case WM_EXITSIZEMOVE:
        {
            int a = 0;
            break;
        }
        case WM_PLAY:
        {
            prepareTimer(state);
            break;
        }
        case WM_TIMER:
        {
            handleTimer(state, wParam);
            break;
        }
        case WM_NCHITTEST:
        {
            return handleHitTesting(window, lParam);
        }
    }
    return defaultWindowCallback(window, message, wParam, lParam);
}

END_SCOPE
