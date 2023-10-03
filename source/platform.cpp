#include "header.h"
#include "platform.h"
#include "globalState.h"

void assert(bool result)
{
	if (result == 0)
	{
		*((char*)0) = 0;
	}
}
void createThread(LPTHREAD_START_ROUTINE startRoutine, LPVOID parameter, DWORD* threadId)
{
	HANDLE threadHandle = CreateThread(0, 0, startRoutine, parameter, 0, threadId);
	assert(threadHandle != 0);
    if(threadHandle)
    {
        CloseHandle(threadHandle);
    }
}
void createEvent(BOOL manualResetRequired, HANDLE* handle)
{
	HANDLE eventHandle = CreateEvent(0, manualResetRequired, 0, 0);
	assert(eventHandle != 0);
	if(handle)
	{
		*handle = eventHandle;
	}
}
void createMutex(int initialState, HANDLE* handle)
{
	HANDLE mutex = CreateMutex(0, initialState, 0);
	assert(mutex != 0);
	if(handle)
	{
		*handle = mutex;
	}
}
void createSemaphore(uint initialCount, uint maximumCount, HANDLE* handle)
{
	HANDLE semaphore = CreateSemaphore(0, (int)initialCount, (int)maximumCount, 0);
	assert(semaphore != 0);
	if(handle)
	{
		*handle = semaphore;
	}
}
void getProcessHeap(HANDLE* handle)
{
	HANDLE processHeapHandle = GetProcessHeap();
	assert(processHeapHandle != 0);
	if(handle)
	{
		*handle = processHeapHandle;
	}
}
void allocateSmallMemory(uint64 size, void** memory)
{
	HANDLE processHeapHandle;
	getProcessHeap(&processHeapHandle);
	void* memoryPointer = HeapAlloc(processHeapHandle, 0, size);
	assert(memoryPointer != 0);
	*memory = (char*)memoryPointer;
}
void freeSmallMemory(void* memory)
{
	HANDLE processHeapHandle;
	getProcessHeap(&processHeapHandle);
	BOOL memoryReleased = HeapFree(processHeapHandle, 0, memory);
	assert(memoryReleased != 0);
}
void allocateBigMemory(uint64 size, char** memory)
{
	char* memoryPointer = (char*)VirtualAlloc(0, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	assert(memoryPointer != 0);
	*memory = memoryPointer;
}
void freeBigMemory(void* memory)
{
	VirtualFree(memory, 0, MEM_RELEASE);
}
void openFileHandle(WCHAR* filePath, DWORD flag, HANDLE* handle)
{
	HANDLE fileHandle = CreateFile(filePath, GENERIC_READ | GENERIC_WRITE, 0, 0, flag, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, 0);
	assert(fileHandle != 0);
	*handle = fileHandle;
}
void closeFileHandle(HANDLE fileHandle)
{
	BOOL closedFileHandle = CloseHandle(fileHandle);
	assert(closedFileHandle != 0);
}
void readFromFile(HANDLE fileHandle, uint64* memorySize, char** memory)
{
	LARGE_INTEGER fileSize;
	BOOL gotFileSize = GetFileSizeEx(fileHandle, &fileSize);
	assert(gotFileSize != 0);

	char* memoryPointer;
	allocateBigMemory((uint64)fileSize.QuadPart, &memoryPointer);
	assert(memoryPointer != 0);

	DWORD bytesRead;
	BOOL readFile = ReadFile(fileHandle, memoryPointer, (DWORD)fileSize.QuadPart, &bytesRead, 0);
	assert(readFile != 0);

	*memorySize = bytesRead;
	*memory = memoryPointer;
}
void writeToFile(HANDLE fileHandle, char* memory, uint64 memorySize)
{
	BOOL wroteFile = WriteFile(fileHandle, memory, (DWORD)memorySize, 0, 0);
	assert(wroteFile != 0);
}
void loadFile(WCHAR* filePath, char** memory)
{
	HANDLE fileHandle;
	openFileHandle(filePath, OPEN_EXISTING, &fileHandle);
	uint64 waveFileSize;
	readFromFile(fileHandle, &waveFileSize, memory);
	closeFileHandle(fileHandle);
}
void getModuleHandle(HINSTANCE* instance)
{
	HINSTANCE windowInstance = GetModuleHandle(0);
	assert(windowInstance != 0);
	*instance = windowInstance;
}
void createWindowClass(const WCHAR* className, WNDPROC callbackFunction)
{
	HINSTANCE windowInstance;
	getModuleHandle(&windowInstance);

	WNDCLASSEX windowClass = {};
    BOOL result = GetClassInfoExW(windowInstance, className, &windowClass);
    if(result != 0)
    {
        return;
    }

	windowClass.cbSize = sizeof(WNDCLASSEX);
	windowClass.style = 0;
	windowClass.lpfnWndProc = callbackFunction;
	windowClass.cbClsExtra = 0;
	windowClass.cbWndExtra = 0;
	windowClass.hInstance = windowInstance;
	windowClass.hIcon = 0;
	windowClass.hCursor = LoadCursor(0, IDC_ARROW);
	windowClass.hbrBackground = 0;
	windowClass.lpszMenuName = 0;
	windowClass.lpszClassName = className;
	windowClass.hIconSm = 0;

	ATOM windowClassAtom = RegisterClassEx(&windowClass);
	assert(windowClassAtom != 0);
}
void createWindow(const WCHAR* className, int width, int height, HWND* window, void* lParam)
{
	HINSTANCE windowInstance;
	getModuleHandle(&windowInstance);

    DWORD style = WS_VISIBLE | WS_OVERLAPPEDWINDOW;
	HWND windowHandle = CreateWindowEx(0, className, L"", style, CW_USEDEFAULT, CW_USEDEFAULT, width, height, 0, 0, windowInstance, lParam);
	assert(windowHandle != 0);
    if(window)
    {
        *window = windowHandle;
    }
}
void createOwnedWindow(const WCHAR* className, HWND parentHandle, HWND* window, void* lParam)
{
	HINSTANCE windowInstance;
	getModuleHandle(&windowInstance);

	DWORD style = WS_VISIBLE;
	HWND windowHandle = CreateWindowEx(0, className, L"", style, 0, 0, 0, 0, parentHandle, 0, windowInstance, lParam);
	assert(windowHandle != 0);
	*window = windowHandle;
}
void createChildWindow(const WCHAR* className, HWND parentHandle, HWND* window, void* lParam)
{
	HINSTANCE windowInstance;
	getModuleHandle(&windowInstance);

    DWORD style = WS_VISIBLE | WS_CHILD;
	HWND windowHandle = CreateWindowEx(0, className, L"", style, 0, 0, 0, 0, parentHandle, 0, windowInstance, lParam);
	assert(windowHandle != 0);
	*window = windowHandle;
}
void setState(HWND window, LPARAM lParam)
{
	CREATESTRUCT* createStruct = (CREATESTRUCT*)lParam;
	LPVOID state = createStruct->lpCreateParams;
	SetProp(window, L"state", state);
}
void placeWindow(HWND window, int x, int y, int width, int height)
{
    MoveWindow(window, x, y, width, height, 1);
}
void getRect(RECT* rectangle, int* left, int* top, int* right, int* bottom)
{
	*left = rectangle->left;
	*top = rectangle->top;
	*right = rectangle->right;
	*bottom = rectangle->bottom;
}
void getWindowRectangle(HWND windowHandle, int* left, int* top, int* right, int* bottom)
{
	RECT windowRectangle;
	GetWindowRect(windowHandle, &windowRectangle);
	getRect(&windowRectangle, left, top, right, bottom);
}
void getWindowDimension(HWND windowHandle, int* width, int* height)
{
	RECT windowRectangle;
	GetClientRect(windowHandle, &windowRectangle);
	int left, top;
	getRect(&windowRectangle, &left, &top, width, height);
}void getWindowPosition(HWND windowHandle, int* x, int* y)
{
	RECT windowRectangle;
	GetWindowRect(windowHandle, &windowRectangle);
	int right, bottom;
	getRect(&windowRectangle, x, y, &right, &bottom);
}
void rectangleFrame(HDC deviceContext, RECT* invalidRectangle, uint color)
{
	HBRUSH brush = CreateSolidBrush(color);
	FrameRect(deviceContext, invalidRectangle, brush);
	DeleteObject(brush);
}
void rectangleFill(HDC deviceContext, RECT* invalidRectangle, uint color)
{
	HBRUSH brush = CreateSolidBrush(color);
	FillRect(deviceContext, invalidRectangle, brush);
	DeleteObject(brush);
}
void mapToParent(HWND window, int* x, int* y)
{
	POINT point = { *x, *y };
	HWND parent = GetAncestor(window, GA_PARENT);
	ScreenToClient(parent, &point);

	*x = point.x;
	*y = point.y;
}
void startMessageLoop(void)
{
	MSG message;
	while (GetMessage(&message, 0, 0, 0))
	{
		TranslateMessage(&message);
		DispatchMessage(&message);
	}
}
void handleWindowSizeChanged(HWND window, LPARAM lParam)
{
	HWND child = GetWindow(window, GW_CHILD);
	SendMessage(child, WM_RESIZE, 0, lParam);
}
void resizeWindow(HWND window, int width, int height)
{
	int x, y;
	getWindowPosition(window, &x, &y);
	mapToParent(window, &x, &y);

	MoveWindow(window, x, y, width, height, 1);
}
void resizeWindow(HWND window, LPARAM lParam)
{
	int width = LOWORD(lParam);
	int height = HIWORD(lParam);

	int x, y;
	getWindowPosition(window, &x, &y);
	mapToParent(window, &x, &y);

	width -= x;;
	height -= y;

	MoveWindow(window, x, y, width, height, 1);
}
void handleVerticalScroll(HWND window, LPARAM lParam)
{
	int wheelDelta = (int)lParam;

	int x, y;
	getWindowPosition(window, &x, &y);
	mapToParent(window, &x, &y);

	y += wheelDelta;
	if (y > 0)
	{
		return;
	}

	int width, height;
	getWindowDimension(window, &width, &height);
	height -= wheelDelta;

	MoveWindow(window, x, y, width, height, 1);
}
void handleHorizontalScroll(HWND window, LPARAM lParam)
{
	int wheelDelta = (int)lParam;

	int x, y;
	getWindowPosition(window, &x, &y);
	mapToParent(window, &x, &y);

	x -= wheelDelta;
	if (x > 0)
	{
		return;
	}

	int width, height;
	getWindowDimension(window, &width, &height);
	width += wheelDelta;

	MoveWindow(window, x, y, width, height, 1);
}
void zoomWindow(HWND window, LPARAM oldFramesPerPixel)
{
	sint64 newFramesPerPixel = globalState.framesPerPixel;
	POINT cursor = {};
	GetCursorPos(&cursor);
	ScreenToClient(window, &cursor);
	int cursorX = cursor.x;

	int width, height;
	getWindowDimension(window, &width, &height);
	sint64 cursorFrame = oldFramesPerPixel * cursorX;
	int newCursorX = (int)(cursorFrame / newFramesPerPixel);
	int mod = newCursorX % newFramesPerPixel;
	if (newFramesPerPixel / 2 < mod)
	{
		++newCursorX;
	}
	int offsetX = cursorX - newCursorX;

	int x, y;
	getWindowPosition(window, &x, &y);
	mapToParent(window, &x, &y);

	x += offsetX;
	if(x > 0)
	{
		x = 0;
		offsetX = 0;
	}
	width -= offsetX;

	MoveWindow(window, x, y, width, height, 0);
	InvalidateRect(window, 0, 0);
}
void drawMarking(HDC deviceContext, RECT* invalidRectangle, int offsetX)
{
	HFONT font = CreateFont(15, 0, 0, 0, 0, 0, 0, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
	SelectObject(deviceContext, font);

	SetTextAlign(deviceContext, TA_LEFT);
	SetBkMode(deviceContext, TRANSPARENT);
	SetTextColor(deviceContext, COLOR_WHITE);

	sint64 framesPerPixel = globalState.framesPerPixel;
	int framesPerMarking = (int)globalState.sampleRate;

	int pixelsPerMarking = (int)(framesPerMarking / framesPerPixel);
	int left = invalidRectangle->left - offsetX;
	int right = invalidRectangle->right - offsetX;
	int leftMarking = left / pixelsPerMarking;
	int rightMarking = right / pixelsPerMarking;

	WCHAR marking[4] = {};
	for (int i = leftMarking; i != rightMarking + 1; ++i)
	{
		int x = (i * pixelsPerMarking) + offsetX;
		wsprintf(marking, L"%d", i);
		int stringLength = (int)wcslen(marking);
		TextOut(deviceContext, x + 4, 0, marking, stringLength);
	}
	DeleteObject(font);
}
void drawGrid(HDC deviceContext, RECT* invalidRectangle, int offsetX)
{
	sint64 framesPerPixel = globalState.framesPerPixel;
	int framesPerMarking = (int)globalState.sampleRate;

	int left = invalidRectangle->left - offsetX;
	int right = invalidRectangle->right - offsetX;


	int pixelsPerMarking = (int)(framesPerMarking / framesPerPixel);
	int leftMarking = left / pixelsPerMarking;
	int rightMarking = right / pixelsPerMarking;
    int top = invalidRectangle->top;
    int bottom = invalidRectangle->bottom;

	for (int i = leftMarking; i != rightMarking + 1; ++i)
	{
		int x = (i  * pixelsPerMarking) + offsetX;
		MoveToEx(deviceContext, x, top, 0);
		LineTo(deviceContext, x, bottom);
	}
}
void getFileCount(WPARAM wParam, uint* count)
{
    HDROP drop = (HDROP)wParam;
	UINT magic = 0xffffffff;
	*count = DragQueryFile(drop, magic, 0, 128);
}
void checkExtension(WCHAR* filePath, uint64 stringLength)
{
	WCHAR v = L'v';
	WCHAR lastCharacter = *(filePath + stringLength - 1);
	if (lastCharacter != v)
	{
		*filePath = 0;
	}
}
void getFilePath(WPARAM wParam, String* filePath, uint index)
{
	HDROP drop = (HDROP)wParam;
	filePath->characterCount = DragQueryFile(drop, index, filePath->string, 256);
}
void horizontalScroll(HWND window, LPARAM lParam)
{
	int wheelDelta = (int)lParam;
	int x, y;
	getWindowPosition(window, &x, &y);

	mapToParent(window, &x, &y);

	x -= wheelDelta;
	if (x > 0)
	{
		return;
	}

	int width, height;
	getWindowDimension(window, &width, &height);
	width += wheelDelta;

	MoveWindow(window, x, y, width, height, 1);
}
void getTrackNumber(HWND window, int* trackNumber, int height)
{
	POINT cursor;
	GetCursorPos(&cursor);
	ScreenToClient(window, &cursor);

	int cursorY = cursor.y;
	*trackNumber = (cursorY / height);
}
void boundCheck(RingBuffer* ringBuffer, void** bufferPointer, uint offset)
{
	char* bufferStart = ringBuffer->start + offset;
	char* bufferEnd = ringBuffer->end + offset;
	if (*bufferPointer == bufferEnd)
	{
		*bufferPointer = bufferStart;
	}
}
void setEventArray(HANDLE* eventArray, uint eventCount)
{
    for(uint i = 0; i != eventCount; ++i)
    {
        SetEvent(eventArray[i]);
    }
}
void waitForSemaphore(HANDLE semaphore)
{
    while(1)
    {
        DWORD result = WaitForSingleObject(semaphore, 0);
        if(result == WAIT_TIMEOUT)
        {
            break;
        }
        ReleaseSemaphore(semaphore, 1, 0);
        Sleep(30);
    }

}
void checkCompletion(HANDLE event, uint totalCount)
{
	uint currentCount = 1;
	while (currentCount != totalCount)
	{
		WaitForSingleObject(event, INFINITE);
		++currentCount;
	}
}
