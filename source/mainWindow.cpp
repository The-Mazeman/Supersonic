#include "header.h"
#include "mainWindow.h"

START_SCOPE(mainWindow)

LRESULT windowCallback(HWND window, UINT message, WPARAM wParam, LPARAM lParam);

void setCursor(State* state)
{
	HWND timelineCursor;
	HWND clipArea = state->clipArea;
	SendMessage(clipArea, WM_GETCURSOR, (WPARAM)&timelineCursor, 0);

	HWND audioEngine = state->audioEngine;
	SendMessage(audioEngine, WM_SETCURSOR, (WPARAM)timelineCursor, 0);
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
	sidebar::create(window, &state->sidebar);
}
void create()
{
	State* state = {};
	allocateSmallMemory(sizeof(State), (void**)&state);

	state->playing = 0;
	state->trackCount = 0;

	HWND window;
    createWindowClass(L"mainWindowClass", windowCallback);
    createWindow(L"mainWindowClass", 0, 0, &window, state);

	setAttribute(window);
	createLayout(state, window);

	audioEngine::create(window, &state->audioEngine);
	setCursor(state);

	placeWindow(window, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
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
        SendMessage(audioEngine, WM_PAUSE, state->trackCount, 0);
        state->playing = 0;
    }
    else
    {
        SendMessage(audioEngine, WM_PLAY, state->trackCount, 0);
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
void createTrack(State* state, uint newTrackCount)
{
    uint trackCount = state->trackCount;
    for(uint i = 0; i != newTrackCount; ++i)
    {
		HWND audioTrack = {};
		HWND audioEngine = state->audioEngine;
        SendMessage(audioEngine, WM_CREATETRACK, trackCount, (LPARAM)&audioTrack);

		HWND sidebar = state->sidebar;
        SendMessage(sidebar, WM_CREATETRACK, trackCount, (LPARAM)audioTrack);
		++trackCount;
    }
    state->trackCount = trackCount;
}
void sendAudioClip(State* state, AudioClip* audioClip, uint trackNumber)
{
	HWND audioEngine = state->audioEngine;
    SendMessage(audioEngine, WM_FILEDROP, (WPARAM)audioClip, trackNumber);

	HWND clipArea = state->clipArea;
    SendMessage(clipArea, WM_FILEDROP, (WPARAM)audioClip, trackNumber);
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
	allocateSmallMemory(sizeof(String) * filePathCount, (void**)&filePath);
	*filePathArray = filePath;
	const WCHAR* extensionString = L"wav";

	String extension = {};
	extension.string = (WCHAR*)extensionString;
	extension.characterCount = 3;
	for(uint i = 0; i != filePathCount; ++i)
	{
		WCHAR* pathString = {};
		allocateSmallMemory(sizeof(WCHAR) * 256, (void**)&pathString);

		filePath->string = pathString;
		getFilePath(wParam, filePath, i);

		checkExtension(filePath, &extension);
		if(filePath->characterCount == 0)
		{
			continue;
		}
		++filePath;
		++(*fileCount);
	}
}
void getTrackInformation(State* state,uint fileCount, uint* trackNumber, uint* newTrackCount)
{
	POINT cursor;
	GetCursorPos(&cursor);
	HWND clipArea = state->clipArea;
	ScreenToClient(clipArea, &cursor);
	int y = cursor.y;
	int x = cursor.x;
	SendMessage(clipArea, WM_SETDROP, (WPARAM)x, 0);

	int trackHeight = globalState.trackHeight;
	*trackNumber = (uint)(y / trackHeight);

	uint trackCount = globalState.trackCount;
	if(*trackNumber >= trackCount)
	{
		*trackNumber = trackCount;
	}
	*newTrackCount = *trackNumber + fileCount - trackCount;
	globalState.trackCount += *newTrackCount;
}
void getStartFrame(int x, uint64* startFrame)
{
	sint64 framesPerPixel = globalState.framesPerPixel;
	*startFrame = (uint64)(x * framesPerPixel);
}
void handleFileDrop(State* state, WPARAM wParam)
{
	String* filePathArray;
	uint fileCount = {};
	getFilePathArray(wParam, &filePathArray, &fileCount);

	uint newTrackCount = {};
	uint trackNumber = {};
	getTrackInformation(state, fileCount, &trackNumber, &newTrackCount);

	createTrack(state, newTrackCount);

    for(uint i = 0; i != fileCount; ++i)
    {
		WaveFile waveFile = {};
		char* waveFilePointer = {};
        waveFile::create(filePathArray[i].string, &waveFile, &waveFilePointer);
		AudioClip* audioClip = {};
		waveFile::createAudioClip(&waveFile, &audioClip);

		freeSmallMemory(filePathArray[i].string);
		freeBigMemory(waveFilePointer);
        sendAudioClip(state, audioClip, trackNumber);
        ++trackNumber;
    }
	freeSmallMemory(filePathArray);
}
void handleMouseLeftClick(State* state)
{
	HWND clipArea = state->clipArea;
	POINT cursor;
	GetCursorPos(&cursor);
	ScreenToClient(clipArea, &cursor);
	int cursorX = cursor.x;
	cursorX -= globalState.offsetX;
	if(state->playing == 0)
	{
		if(cursorX > 0)
		{
			SendMessage(clipArea, WM_MOVECURSOR, (uint)cursorX, 0);
		}
	}
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
