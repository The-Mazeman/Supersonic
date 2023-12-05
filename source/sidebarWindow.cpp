#include "include.hpp"
#include "sidebarWindow.hpp"

START_SCOPE(sidebarWindow)

LRESULT CALLBACK windowCallback(HWND window, UINT message, WPARAM wParam, LPARAM lParam);

void create(GlobalState* globalState, HWND parent, HWND* window, HWND clipAreaWindow)
{
    State* state = {};
    allocateMemory(sizeof(State), (void**)&state);
    state->globalState = globalState;
	state->x = 0;
	state->y = (int)(globalState->titleBarHeight + globalState->rulerHeight);
    state->globalSoloState = 0;
    state->soloTrackCount = 0;

    createDynamicArray(&state->trackHeaderArrayHandle, sizeof(HWND));
    createDynamicArray(&state->audioTrackArrayHandle, sizeof(HWND));
    createDynamicArray(&state->busTrackArrayHandle, sizeof(HWND));
    createDynamicArray(&state->audioTrackStartEventArrayHandle, sizeof(HANDLE));

    createWindowClass(L"sidebarWindowClass", windowCallback);
    createChildWindow(L"sidebarWindowClass", parent, state, window);

    HWND wasapi = {};
    uint frameCount = globalState->audioEndpointFrameCount;
    audioWasapi::create(*window, &wasapi, frameCount, clipAreaWindow);
    appendElement(state->busTrackArrayHandle, (void**)&wasapi);

    String* menuOptionArray = {};
    allocateMemory(sizeof(String) * 2, (void**)&menuOptionArray);
    menuOptionArray[0].string = (WCHAR*)L"Add Audio Track";
    menuOptionArray[0].characterCount = 15;
    menuOptionArray[1].string = (WCHAR*)L"Add Bus Track";
    menuOptionArray[1].characterCount = 13;
    contextMenuWindow::create(*window, menuOptionArray, 2, &state->contextMenu);
}
void paint(State* state, HWND window)
{
    NOT_USING(state);
	PAINTSTRUCT paintStruct;
	HDC deviceContext = BeginPaint(window, &paintStruct);

	RECT* invalidRectangle = &paintStruct.rcPaint;
	rectangleFill(deviceContext, invalidRectangle, COLOR_BLACK);
    int width, height;
    getWindowDimension(window, &width, &height);

    int right = invalidRectangle->right - 1;
    int bottom = invalidRectangle->bottom;
    POINT start = {width - 1, 0};
    POINT end = {width - 1, bottom};

    SelectObject(deviceContext, GetStockObject(DC_PEN));
	SetDCPenColor(deviceContext, COLOR_WHITE);
    drawLine(deviceContext, &start, &end);

	EndPaint(window, &paintStruct);
}
void handleResize(State* state, HWND window, LPARAM lParam)
{
    int x = state->x;
	int y = state->y;
    int width = (int)state->globalState->sidebarWidth;
    int height = HIWORD(lParam) - y;
    placeWindow(window, x, y, width, height);
}
void handleMouseRightClick(State* state, HWND window, LPARAM lParam)
{
    int y = GET_Y_LPARAM(lParam);
    int x = GET_X_LPARAM(lParam);

    POINT point = {x, y};
    ClientToScreen(window, &point);
    x = point.x;
    y = point.y;
    HWND contextMenu = state->contextMenu;
    int width, height;
    getWindowDimension(contextMenu, &width, &height);
    placeWindow(contextMenu, x, y, width, height);
    ShowWindow(contextMenu, SW_SHOW);
}
void createAudioTrack(State* state, HWND window, WPARAM wParam)
{
    void* trackHeaderArrayHandle = state->trackHeaderArrayHandle;
    uint trackNumber = {};
    getOccupiedSlotCount(trackHeaderArrayHandle, &trackNumber);
    uint newTrackCount = (uint)wParam;
    resizeArray(trackHeaderArrayHandle, newTrackCount);

    int sidebarWidth = (int)state->globalState->sidebarWidth;
    int trackHeight = (int)state->globalState->trackHeight;
    int x = 0;
    int offsetY = (int)state->globalState->offsetY;
    for(uint i = 0; i != newTrackCount; ++i)
    {
        int y = (int)((trackNumber + i) * trackHeight) - offsetY;
        RECT boundingBox = {x, y, sidebarWidth, trackHeight};

        HWND trackHeader = {};
        String name = {};
        name.string = (WCHAR*)L"Audio Track";
        name.characterCount = 11;

        trackHeaderWindow::create(&name, window, &boundingBox, &trackHeader);
        appendElement(trackHeaderArrayHandle, &trackHeader);

        void* audioTrackArrayHandle = state->audioTrackArrayHandle;
        appendElement(audioTrackArrayHandle, &trackHeader);

        HANDLE startEvent = {};
        createEvent(&startEvent);

        void* audioTrackStartEventArrayHandle = state->audioTrackStartEventArrayHandle;
        appendElement(audioTrackStartEventArrayHandle, &startEvent);
    }
}
void handleContextMenu(State* state, HWND window, WPARAM wParam)
{
    uint menuOptionId = (uint)wParam;
    switch(menuOptionId)
    {
        case 0:
        {
            HWND parent = GetAncestor(window, GA_PARENT);
            SendMessage(parent, WM_CREATEAUDIOTRACK, 1, 0);
            break;
        }
        case 1:
        {
            HWND parent = GetAncestor(window, GA_PARENT);
            SendMessage(parent, WM_CREATEBUSTRACK, 1, 0);
        }
    }
}
void resizeTrackHeight(State* state)
{
    void* trackHeaderArrayHandle = state->trackHeaderArrayHandle;
    HWND* trackHeaderArray = {};
    uint trackHeaderCount = {};
    getArray(trackHeaderArrayHandle, (void**)&trackHeaderArray, &trackHeaderCount);

    int x = 0;
    int trackHeight = (int)state->globalState->trackHeight;
    int offsetY = (int)state->globalState->offsetY;
    for(int i = 0; i != (int)trackHeaderCount; ++i)
    {
        int width, height;
        getWindowDimension(trackHeaderArray[i], &width, &height);
        int y = (i * trackHeight) - offsetY;
        MoveWindow(trackHeaderArray[i], x, y, width, trackHeight, 1);
        InvalidateRect(trackHeaderArray[i], 0, 0);
    }
}
void startPlayback(State* state, WPARAM wParam)
{
    void* audioTrackArrayHandle = state->audioTrackArrayHandle;
    HWND* audioTrackArray = {};
    uint audioTrackCount = {};
    getArray(audioTrackArrayHandle, (void**)&audioTrackArray, &audioTrackCount);

    void* audioClipContainerArrayHandle = (void*)wParam;
    void** audioClipContainerArray = {};
    getArrayStart(audioClipContainerArrayHandle, (void**)&audioClipContainerArray);

    uint64 readCursor = state->globalState->readCursor;
    uint frameCount = state->globalState->audioEndpointFrameCount;
    uint64 value[2] = {readCursor, frameCount};
    for(uint i = 0; i != audioTrackCount; ++i)
    {
        SendMessage(audioTrackArray[i], WM_PREPARETOPLAY, (WPARAM)audioClipContainerArray[i], (LPARAM)value);
    }
    void* busTrackArrayHandle = state->busTrackArrayHandle;
    HWND* busTrackArray = {};
    uint busTrackCount = {};
    getArray(busTrackArrayHandle, (void**)&busTrackArray, &busTrackCount);

    void* audioTrackStartEventArrayHandle = state->audioTrackStartEventArrayHandle;
    HANDLE* startEventArray = {};
    getArrayStart(audioTrackStartEventArrayHandle, (void**)&startEventArray);
    setEventArray(startEventArray, audioTrackCount);
    SendMessage(busTrackArray[0], WM_PREPARETOPLAY, (WPARAM)audioTrackStartEventArrayHandle, 0);

    for(uint i = 0; i != audioTrackCount; ++i)
    {
        SendMessage(audioTrackArray[i], WM_PLAY, (WPARAM)startEventArray[i], (LPARAM)value);
    }
    for(uint i = 0; i != busTrackCount; ++i)
    {
        SendMessage(busTrackArray[i], WM_PLAY, 0, 0);
    }
}
void createBusTrack(State* state, HWND window, WPARAM wParam)
{
    void* trackHeaderArrayHandle = state->trackHeaderArrayHandle;
    uint trackNumber = {};
    getOccupiedSlotCount(trackHeaderArrayHandle, &trackNumber);
    uint newTrackCount = (uint)wParam;
    resizeArray(trackHeaderArrayHandle, newTrackCount);

    int sidebarWidth = (int)state->globalState->sidebarWidth;
    int trackHeight = (int)state->globalState->trackHeight;
    int x = 0;
    int offsetY = (int)state->globalState->offsetY;
    for(uint i = 0; i != newTrackCount; ++i)
    {
        int y = (int)((trackNumber + i) * trackHeight) - offsetY;
        RECT boundingBox = {x, y, sidebarWidth, trackHeight};

        HWND trackHeader = {};
        String name = {};
        name.string = (WCHAR*)L"Bus Track";
        name.characterCount = 9;

        trackHeaderWindow::create(&name, window, &boundingBox, &trackHeader);
        appendElement(trackHeaderArrayHandle, &trackHeader);

        void* busTrackArrayHandle = state->busTrackArrayHandle;
        appendElement(busTrackArrayHandle, &trackHeader);
    }
}
void setOutput(State* state, WPARAM wParam, LPARAM lParam)
{
    uint busNumber = (uint)lParam;
    void* busTrackArrayHandle = state->busTrackArrayHandle;
    HWND* outputBusArray = {};
    getArrayStart(busTrackArrayHandle, (void**)&outputBusArray);
    SendMessage(outputBusArray[busNumber], WM_SETINPUT, wParam, 0);
}
void stopPlayback(State* state)
{
    void* audioTrackArrayHandle = state->audioTrackArrayHandle;
    HWND* audioTrackArray = {};
    uint audioTrackCount = {};
    getArray(audioTrackArrayHandle, (void**)&audioTrackArray, &audioTrackCount);

    for(uint i = 0; i != audioTrackCount; ++i)
    {
        SendMessage(audioTrackArray[i], WM_PAUSE, 0, 0);
    }

    void* busTrackArrayHandle = state->busTrackArrayHandle;
    HWND* busTrackArray = {};
    uint busTrackCount = {};
    getArray(busTrackArrayHandle, (void**)&busTrackArray, &busTrackCount);

    for(uint i = 0; i != busTrackCount; ++i)
    {
        SendMessage(busTrackArray[i], WM_PAUSE, 0, 0);
    }
}
void toggleSoloTrack(State* state, WPARAM wParam, LPARAM lParam)
{
    uint globalSoloState = state->globalSoloState;
    uint soloTrackCount = state->soloTrackCount;
    uint soloState = (uint)wParam;
    HWND audioTrack = (HWND)lParam;
    if(soloState == 1)
    {
        ++soloTrackCount;
    }
    else
    {
        --soloTrackCount;
    }
    state->soloTrackCount = soloTrackCount;
    if(globalSoloState && soloTrackCount > 0)
    {
        uint muteState = !soloState;
        SendMessage(audioTrack, WM_TOGGLEMUTETRACK, muteState, 0);
        return;
    }
    void* audioTrackArrayHandle = state->audioTrackArrayHandle;
    HWND* audioTrackArray = {};
    uint audioTrackCount = {};
    getArray(audioTrackArrayHandle, (void**)&audioTrackArray, &audioTrackCount);

    for(uint i = 0; i != audioTrackCount; ++i)
    {
        if(audioTrackArray[i] == audioTrack)
        {
            continue;
        }
        SendMessage(audioTrackArray[i], WM_TOGGLEMUTETRACK, soloState, 0);
    }
    state->globalSoloState = !globalSoloState;
    
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
        case WM_RESIZE:
        {
            handleResize(state, window, lParam);
            break;
        }
        case WM_RBUTTONDOWN:
        {
            handleMouseRightClick(state, window, lParam);
            break;
        }
        case WM_RESIZETRACKHEIGHT:
        {
            resizeTrackHeight(state);
            break;
        }
        case WM_CREATEAUDIOTRACK:
        {
            createAudioTrack(state, window, wParam);
            break;
        }
        case WM_CREATEBUSTRACK:
        {
            createBusTrack(state, window, wParam);
            break;
        }
        case WM_CONTEXTMENUOPTION:
        {
            handleContextMenu(state, window, wParam);
            break;
        }
        case WM_TOGGLESOLOTRACK:
        {
            toggleSoloTrack(state, wParam, lParam);
            break;
        }
        case WM_PLAY:
        {
            startPlayback(state, wParam);
            break;
        }
        case WM_PAUSE:
        {
            stopPlayback(state);
            break;
        }
        case WM_SETOUTPUT:
        {
            setOutput(state, wParam, lParam);
            break;
        }
    }
    return defaultWindowCallback(window, message, wParam, lParam);
}
END_SCOPE
