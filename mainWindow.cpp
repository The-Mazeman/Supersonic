#include "header.h"
#include "platform.h"
#include "globalState.h"
#include "waveFile.h"
#include "mainWindow.h"
#include "topbar.h"
#include "ruler.h"
#include "sidebar.h"
#include "clipArea.h"
#include "clipGrid.h"
#include "audioClip.h"
#include "wasapi.h"
#include "audioEngine.h"
#include "timelineCursor.h"

START_SCOPE(main)

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
void verticalScroll(HWND window, short wheelDelta)
{
	State* state = (State*)GetProp(window, L"state");

	HWND sidebar = state->sidebar;
	HWND trackHeaderGroup = GetWindow(sidebar, GW_CHILD);
	SendMessage(trackHeaderGroup, WM_VERTICALMOUSEWHEEL, 0, wheelDelta);

	HWND clipArea = state->clipArea;
	HWND clipGrid = GetWindow(clipArea, GW_CHILD);
	SendMessage(clipGrid, WM_VERTICALMOUSEWHEEL, 0, wheelDelta);
}
void pinchZoom(HWND window, short wheelDelta)
{
	wheelDelta /= 120;
	wheelDelta *= 32;
	sint64 oldFramesPerPixel = globalState.framesPerPixel;
	if(oldFramesPerPixel - wheelDelta < 32)
	{
		return;
	}

	globalState.framesPerPixel -= wheelDelta;
	State* state = (State*)GetProp(window, L"state");

	HWND ruler = state->ruler;
	HWND rulerGrid = GetWindow(ruler, GW_CHILD);
	SendMessage(rulerGrid, WM_PINCHZOOM, 0, oldFramesPerPixel);

	HWND clipArea = state->clipArea;
	HWND clipGrid = GetWindow(clipArea, GW_CHILD);
	SendMessage(clipGrid, WM_PINCHZOOM, 0, oldFramesPerPixel);
}
void handleMouseVerticalWheel(HWND window, WPARAM wParam, LPARAM lParam)
{
	notUsing(lParam);

	short wheelDelta = GET_WHEEL_DELTA_WPARAM(wParam);
	ushort keyState = GET_KEYSTATE_WPARAM(wParam);
	if (keyState == MK_CONTROL)
	{
		pinchZoom(window, wheelDelta);
	}
	else
	{
		verticalScroll(window, wheelDelta);
	}
}
void horizontalScroll(HWND window, int wheelDelta)
{
	State* state = (State*)GetProp(window, L"state");
	HWND ruler = state->ruler;
	HWND rulerGrid = GetWindow(ruler, GW_CHILD);
	SendMessage(rulerGrid, WM_HORIZONTALMOUSEWHEEL, 0, wheelDelta);

	HWND clipArea = state->clipArea;
	HWND clipGrid = GetWindow(clipArea, GW_CHILD);
	SendMessage(clipGrid, WM_HORIZONTALMOUSEWHEEL, 0, wheelDelta);
}
void handleMouseHorizontalWheel(HWND window, WPARAM wParam, LPARAM lParam)
{
	notUsing(lParam);

	short wheelDelta = GET_WHEEL_DELTA_WPARAM(wParam);
	horizontalScroll(window, wheelDelta);
}
void handleSizeChanged(HWND window, LPARAM lParam)
{
	WINDOWPOS* position = (WINDOWPOS*)lParam;
	int width = position->cx;
	int height = position->cy;
	if(width < 138)
	{
		return;
	}
	POINT dimension = {width, height};
	HWND* childArray = (HWND*)GetProp(window, L"state");

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
void handleFileDrop(HWND window, WPARAM wParam)
{
	HDROP drop = (HDROP)wParam;
	uint count;
	getFileCount(drop, &count);

	for(uint i = 0; i != count; ++i)
	{
		WCHAR filePath[256];
		getFilePath(wParam, filePath, i);
		uint64 stringLength = wcslen(filePath);
		checkIfWaveFile(filePath, stringLength);
		if (*filePath == 0)
		{
			return;
		}
		AudioClip* audioClip;
		allocateSmallMemory(sizeof(AudioClip), (char**)&audioClip);

		void* sampleChunk;
		waveFile::load(filePath, &audioClip->waveFile, &sampleChunk);
		audioClip->waveFile.sampleChunk = sampleChunk;

		State* state = (State*)GetProp(window, L"state");
		HWND sidebar = state->sidebar;
		HWND trackHeaderGroup = GetWindow(sidebar, GW_CHILD);

		HWND clipArea = state->clipArea;
		HWND clipGrid = GetWindow(clipArea, GW_CHILD);

		HWND audioEngine = state->audioEngine;

		POINT cursor;
		GetCursorPos(&cursor);
		ScreenToClient(clipGrid, &cursor);
		audioClip->x = cursor.x;

		int trackHeight = globalState.trackHeight;
		int trackNumber = cursor.y / trackHeight;
		trackNumber += i;
		int trackCount = globalState.trackCount;
		if(trackNumber >= trackCount)
		{
			
			trackNumber = trackCount;
			SendMessage(trackHeaderGroup, WM_CREATETRACK, (WPARAM)audioClip, trackNumber);
			SendMessage(audioEngine, WM_CREATETRACK, 0, trackNumber);
		}
		SendMessage(audioEngine, WM_FILEDROP, (WPARAM)audioClip, trackNumber);

		SendMessage(clipGrid, WM_FILEDROP, (WPARAM)audioClip, trackNumber);
	}
	DragFinish(drop);
}
void createState(HWND window)
{
	State* state = {};
	allocateSmallMemory(sizeof(State), (char**)&state);
	state->playing = 0;
	SetProp(window, L"state", state);
}
typedef LRESULT (*WindowCallback)(HWND, UINT, WPARAM, LPARAM);
void createLayout(HWND window)
{
	State* state = (State*)GetProp(window, L"state");
    WCHAR className[4][30] = {L"topBarWindowClass", L"rulerWindowClass", L"sidebarWindowClass", L"clipAreaWindowClass"};
    WindowCallback callback[] = {topbar::windowCallback, ruler::windowCallback, sidebar::windowCallback, clipArea::windowCallback};
    HWND* childArray = (HWND*)state;
    Position* position = {};
	allocateSmallMemory(sizeof(Position) * 4, (char**)&position);

    position[0].x = 0;
    position[0].y = 0;

	int rulerX = globalState.sidebarWidth;
	int rulerY = globalState.topbarHeight;
	position[1].x = rulerX;
    position[1].y = rulerY;

	int sidebarX = 0;
	int sideBarY = globalState.topbarHeight + globalState.rulerHeight;
	position[2].x = sidebarX;
	position[2].y = sideBarY;

	int clipAreaX = globalState.sidebarWidth;
	int clipAreaY = globalState.topbarHeight + globalState.rulerHeight;
	position[3].x = clipAreaX;
	position[3].y = clipAreaY;


    for(uint i = 0; i != 4; ++i)
    {
		createWindowClass(&className[i][0], callback[i]);
		createLayer(&className[i][0], window, &childArray[i]);
		MoveWindow(childArray[i], position[i].x, position[i].y, 0, 0, 1);
    }
}
void setupTimelineCursor(State* state)
{
	HWND audioEngine = state->audioEngine;
	HWND clipArea = state->clipArea;
	HWND clipGrid = GetWindow(clipArea, GW_CHILD);
	HWND timelineCursor = GetWindow(clipGrid, GW_CHILD);
	SendMessage(audioEngine, WM_SETCURSOR, (WPARAM)timelineCursor, 0);
}
void handleSpaceBar(HWND window)
{
	State* state = (State*)GetProp(window, L"state");
	HWND audioEngine = state->audioEngine;
	if(state->playing)
	{
		state->playing = 0;
		SendMessage(audioEngine, WM_PAUSE, 0, 0);
	}
	else
	{
		setupTimelineCursor(state);
		state->playing = 1;
		SendMessage(audioEngine, WM_PLAY, 0, 0);
	}

}
void handleKeyboardInput(HWND window, WPARAM wParam)
{
	if (wParam == VK_SPACE)
	{
		handleSpaceBar(window);
	}
}

void createAudioEngine(HWND window)
{
	createWindowClass(L"audioEngineWindowClass", audioEngine::windowCallback);
	HWND audioEngine;
	createChildWindow(L"audioEngineWindowClass", window, &audioEngine);

	State* state = (State*)GetProp(window, L"state");
	state->audioEngine = audioEngine;
}
LRESULT windowCallback(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_CREATE:
		{
			createState(window);
            createLayout(window);
			createAudioEngine(window);
			break;
		}
		case WM_PAINT:
		{
			paintWindow(window);
			break;
		}
		case WM_MOUSEWHEEL:
		{
			handleMouseVerticalWheel(window, wParam, lParam);
			break;
		}
		case WM_MOUSEHWHEEL:
		{
			handleMouseHorizontalWheel(window, wParam, lParam);
			break;
		}
		case WM_DROPFILES:
		{
			handleFileDrop(window, wParam);
			break;
		}
		case WM_KEYDOWN:
		{
			handleKeyboardInput(window, wParam);
			break;
		}
		case WM_WINDOWPOSCHANGING:
		{
			handleSizeChanged(window, lParam);
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
