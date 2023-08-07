#include "header.h"
#include "mainWindow.h"

START_SCOPE(mainWindow)

LRESULT windowCallback(HWND window, UINT message, WPARAM wParam, LPARAM lParam);

void create()
{
    createWindowClass(L"mainWindowClass", windowCallback);
    createWindow(L"mainWindowClass", WINDOW_WIDTH, WINDOW_HEIGHT, 0);
}
LRESULT handleHitTesting(HWND window, LPARAM lParam)
{
	int cursorX = GET_X_LPARAM(lParam);
	int cursorY = GET_Y_LPARAM(lParam);

	int a, b, c, d;
	getWindowRectangle(window, &a, &b, &c, &d);

	if (cursorY < b + 16)
	{
		return HTCAPTION;
	}
	if (cursorX < a + 8)
	{
		return HTLEFT;
	}
	if (cursorX > c - 8)
	{
		return HTRIGHT;
	}
	if (cursorY > d - 8)
	{
		return HTBOTTOM;
	}
	return HTCLIENT;
}
void handleSizing(WPARAM wParam, LPARAM lParam)
{
	RECT* rectangle = {};
	if (wParam)
	{
		NCCALCSIZE_PARAMS* parameter = (NCCALCSIZE_PARAMS*)lParam;
		rectangle = parameter->rgrc;
	}
	else
	{
		rectangle = (RECT*)lParam;
	}
	rectangle->top -= 1;
}
void verticalScroll(State* state, short wheelDelta)
{
	HWND sidebar = state->sidebar;
    ScrollWindowEx(sidebar, 0, wheelDelta, 0, 0, 0, 0, SW_INVALIDATE | SW_SCROLLCHILDREN);

}
void handleMouseVerticalWheel(State* state, WPARAM wParam, LPARAM lParam)
{
	notUsing(lParam);

	short wheelDelta = GET_WHEEL_DELTA_WPARAM(wParam);
    if(globalState.offsetY + wheelDelta < 0)
    {
        globalState.offsetY += wheelDelta;
		verticalScroll(state, wheelDelta);
    }
}
void horizontalScroll(State* state, int wheelDelta)
{
	HWND ruler = state->ruler;
    ScrollWindowEx(ruler, wheelDelta, 0, 0, 0, 0, 0, SW_INVALIDATE);
	HWND clipArea = state->clipArea;
    ScrollWindowEx(clipArea, wheelDelta, 0, 0, 0, 0, 0, SW_INVALIDATE | SW_SCROLLCHILDREN);
}
void handleMouseHorizontalWheel(State* state, WPARAM wParam, LPARAM lParam)
{
	notUsing(lParam);

	short wheelDelta = GET_WHEEL_DELTA_WPARAM(wParam);
    wheelDelta *= -1;

    if(globalState.offsetX + wheelDelta < 0)
    {
        globalState.offsetX += wheelDelta;
        horizontalScroll(state, wheelDelta);
    }
}
void handleSizeChanged(State* state, LPARAM lParam)
{
	int width = LOWORD(lParam);
	int height = HIWORD(lParam);

	POINT dimension = {width, height};
	HWND* childArray = (HWND*)state;
	for(uint i = 0; i != 4; ++i)
	{
		SendMessage(childArray[i], WM_RESIZE, (WPARAM)&dimension, 0);
	}
}
void paintWindow(HWND window)
{
	PAINTSTRUCT paintStruct;
	HDC deviceContext = BeginPaint(window, &paintStruct);

	RECT* invalidRectangle = &paintStruct.rcPaint;
	rectangleFill(deviceContext, invalidRectangle, COLOR_BLACK);

	EndPaint(window, &paintStruct);
}
void getTrackNumber(HWND window, int* trackNumber)
{
	POINT cursor;
	GetCursorPos(&cursor);
	ScreenToClient(window, &cursor);

	int cursorY = cursor.y;
	int trackHeight = globalState.trackHeight;
	*trackNumber = cursorY / trackHeight;
}
void togglePlayback(State* state)
{
	HWND audioEngine = state->audioEngine;
    if(state->playing)
    {
        SendMessage(audioEngine, WM_PAUSE, 0, 0);
        state->playing = 0;
    }
    else
    {
        SendMessage(audioEngine, WM_PLAY, 0, 0);
        state->playing = 1;
    }
}
void handleKeyboardInput(State* state, WPARAM wParam)
{
    switch(wParam)
    {
        case VK_SPACE:
        {
            togglePlayback(state);
        }
    }
}
void setAttribute(HWND window)
{
    SetWindowTheme(window, L"", L"");
    DragAcceptFiles(window, 1);
}
void createLayout(State* state, HWND window)
{
    topbar::create(window, &state->topbar);
    ruler::create(window, &state->ruler);
    clipArea::create(window, &state->clipArea);
    sidebar::create(window,  &state->sidebar);
}
void setCursor(State* state)
{
    HWND timelineCursor;
	HWND clipArea = state->clipArea;
	SendMessage(clipArea, WM_GETCURSOR, (WPARAM)&timelineCursor, 0);

	HWND audioEngine = state->audioEngine;
    SendMessage(audioEngine, WM_SETCURSOR, (WPARAM)timelineCursor, 0);
}
void initialize(HWND window)
{
	State* state = {};
	allocateSmallMemory(sizeof(State), (void**)&state);
	SetProp(window, L"state", state);

    state->playing = 0;
    state->trackCount = 0;

	int width, height;
	getWindowDimension(window, &width, &height);
    createLayout(state, window);

    setAttribute(window);
    audioEngine::create(window, &state->audioEngine);
    setCursor(state);
}
void getTrackNumber(State* state, uint* trackNumber)
{
    POINT cursor;
    GetCursorPos(&cursor);
    ScreenToClient(state->clipArea, &cursor);
    int number = cursor.y / globalState.trackHeight;
    *trackNumber = (uint)number;

    uint trackCount = state->trackCount;
    if(*trackNumber >= trackCount)
    {
        *trackNumber = trackCount;
    }
} 
void createTrack(State* state, uint trackNumber, uint fileCount)
{
    uint trackCount = state->trackCount;
    if(trackNumber + fileCount > trackCount)
    {
		uint newTrackCount = trackNumber + fileCount - trackCount;
        for(uint i = 0; i != newTrackCount; ++i)
        {
			HWND sidebar = state->sidebar;
            SendMessage(sidebar, WM_CREATETRACK, trackCount, 0);

			HWND audioEngine = state->audioEngine;
            SendMessage(audioEngine, WM_CREATETRACK, trackCount, 0);
			++trackCount;
        }
        state->trackCount = trackCount;
    }
}
void sendAudioClip(State* state, AudioClip* audioClip, uint trackNumber)
{
	HWND audioEngine = state->audioEngine;
    SendMessage(audioEngine, WM_FILEDROP, (WPARAM)&audioClip, trackNumber);

	HWND clipArea = state->clipArea;
    SendMessage(clipArea, WM_FILEDROP, (WPARAM)audioClip, trackNumber);
}
void getCursorX(State* state, int* x)
{
    POINT cursor;
    GetCursorPos(&cursor);

	HWND clipArea = state->clipArea;
    ScreenToClient(clipArea, &cursor);
    *x = cursor.x;
}
void prepareClip(AudioClip* audioClip, int x)
{
	sint64 framesPerPixel = globalState.framesPerPixel;
	uint64 frameCount = audioClip->waveFile.frameCount;

	int width = (int)(frameCount / (uint64)framesPerPixel);
	audioClip->width = width;
	audioClip->x = x;
}
void handleFileDrop(State* state, WPARAM wParam)
{
    uint fileCount;
    getFileCount(wParam, &fileCount);

    uint trackNumber;
    getTrackNumber(state, &trackNumber);
    createTrack(state, trackNumber, fileCount);

    int x;
    getCursorX(state, &x);

    for(uint i = 0; i != fileCount; ++i)
    {
        WCHAR filePath[256];
        getFilePath(wParam, filePath, i);
        if(*filePath == 0)
        {
            break;
        }

		AudioClip audioClip = {};
        waveFile::create(filePath, &audioClip.waveFile);
		prepareClip(&audioClip, x);

        sendAudioClip(state, &audioClip, trackNumber);
        ++trackNumber;
    }
}
void handleMouseLeftClick(State* state)
{
	HWND clipArea = state->clipArea;
	POINT cursor;
	GetCursorPos(&cursor);
	ScreenToClient(clipArea, &cursor);
	int cursorX = cursor.x;
	cursorX -= globalState.offsetX;
	if(cursorX > 0)
	{
		SendMessage(clipArea, WM_MOVECURSOR, (uint)cursorX, 0);
	}
}
LRESULT windowCallback(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
	State* state = (State*)GetProp(window, L"state");
	switch (message)
	{
		case WM_CREATE:
		{
            initialize(window);
			break;
		}
		case WM_PAINT:
		{
			paintWindow(window);
			break;
		}
		case WM_MOUSEWHEEL:
		{
			handleMouseVerticalWheel(state, wParam, lParam);
			break;
		}
		case WM_MOUSEHWHEEL:
		{
			handleMouseHorizontalWheel(state, wParam, lParam);
			break;
		}
		case WM_DROPFILES:
		{
			handleFileDrop(state, wParam);
			break;
		}
		case WM_KEYDOWN:
		{
			handleKeyboardInput(state, wParam);
			break;
		}
		case WM_LBUTTONDOWN:
		{
			handleMouseLeftClick(state);
			break;
		}
		case WM_SIZE:
		{
			handleSizeChanged(state, lParam);
			return 0;
		}
		case WM_NCCALCSIZE:
		{
			return 0;
		}
		case WM_NCACTIVATE:
		{
			lParam = -1;
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
	return DefWindowProc(window, message, wParam, lParam);
}


END_SCOPE
