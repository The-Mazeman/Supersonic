#include "include.hpp" 
#include "platformWindows.hpp"

void assert(bool result)
{
	if (result == 0)
	{
		*((char*)0) = 0;
	}
}
void createThread(LPTHREAD_START_ROUTINE startRoutine, LPVOID parameter)
{
	HANDLE threadHandle = CreateThread(0, 0, startRoutine, parameter, 0, 0);

	assert(threadHandle != 0);
    if(threadHandle)
    {
        CloseHandle(threadHandle);
    }
}
void createEvent(HANDLE* handle)
{
	HANDLE eventHandle = CreateEvent(0, 0, 0, 0);
	assert(eventHandle != 0);
	if(handle)
	{
		*handle = eventHandle;
	}
}
void createMutex(HANDLE* handle)
{
	HANDLE mutex = CreateMutex(0, 0, 0);
	assert(mutex != 0);
	if(handle)
	{
		*handle = mutex;
	}
}
void createSemaphore(HANDLE* handle)
{
	HANDLE semaphore = CreateSemaphore(0, 0, 128, 0);
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
void allocateMemory(uint64 size, void** memory)
{
	HANDLE processHeapHandle;
	getProcessHeap(&processHeapHandle);
	void* memoryPointer = HeapAlloc(processHeapHandle, 0, size);
	assert(memoryPointer != 0);
	*memory = memoryPointer;
}
void freeMemory(void* memory)
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
void getFileSize(HANDLE fileHandle, uint64* fileSize)
{
	LARGE_INTEGER largeInteger;
	BOOL result = GetFileSizeEx(fileHandle, &largeInteger);
	assert(result != 0);
    *fileSize = (uint64)largeInteger.QuadPart;
}
void readFromFile(HANDLE fileHandle, uint64 memorySize, void* memory)
{
	DWORD bytesRead;
	BOOL result = ReadFile(fileHandle, memory, (DWORD)memorySize, &bytesRead, 0);
	assert(result != 0);
}
void writeToFile(HANDLE fileHandle, void* memory, uint64 memorySize)
{
	BOOL result = WriteFile(fileHandle, memory, (DWORD)memorySize, 0, 0);
	assert(result != 0);
}
void loadFile(WCHAR* filePath, void** memory)
{
	HANDLE fileHandle;
	openFileHandle(filePath, OPEN_EXISTING, &fileHandle);

	uint64 fileSize;
    getFileSize(fileHandle, &fileSize);

    allocateMemory(fileSize, memory);
	readFromFile(fileHandle, fileSize, *memory);

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
void createWindow(const WCHAR* className, void* lParam, HWND* window)
{
	HINSTANCE windowInstance;
	getModuleHandle(&windowInstance);

    DWORD style = WS_VISIBLE | WS_OVERLAPPEDWINDOW;
	HWND windowHandle = CreateWindowEx(0, className, L"", style, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, 0, 0, windowInstance, lParam);
	assert(windowHandle != 0);
    if(window)
    {
        *window = windowHandle;
    }
}
void createOwnedWindow(const WCHAR* className, HWND parentHandle, void* lParam, HWND* window)
{
	HINSTANCE windowInstance;
	getModuleHandle(&windowInstance);

	DWORD style = WS_VISIBLE;
	HWND windowHandle = CreateWindowEx(0, className, L"", style, 0, 0, 0, 0, parentHandle, 0, windowInstance, lParam);
	assert(windowHandle != 0);
	*window = windowHandle;
}
void createChildWindow(const WCHAR* className, HWND parentHandle, void* lParam, HWND* window)
{
	HINSTANCE windowInstance;
	getModuleHandle(&windowInstance);

    DWORD style = WS_VISIBLE | WS_CHILD;
	HWND windowHandle = CreateWindowEx(0, className, L"", style, 0, 0, 0, 0, parentHandle, 0, windowInstance, lParam);
	assert(windowHandle != 0);
    if(window)
    {
        *window = windowHandle;
    }
}
void setState(HWND window, LPARAM lParam)
{
	CREATESTRUCT* createStruct = (CREATESTRUCT*)lParam;
	LPVOID state = createStruct->lpCreateParams;
	SetProp(window, L"state", state);
}
LRESULT CALLBACK defaultWindowCallback(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch(message)
    {
        case WM_CREATE:
        {
            setState(window, lParam);
            break;
        }
    }
    return DefWindowProc(window, message, wParam, lParam);
}
void placeWindow(HWND window, int x, int y, int width, int height)
{
    MoveWindow(window, x, y, width, height, 1);
}
void placeWindow(HWND window, RECT* boundingBox) 
{
    int x = boundingBox->left;
    int y = boundingBox->top;
    int width = boundingBox->right;
    int height = boundingBox->bottom;
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
}
void setWindowSize(HWND windowHandle, int width, int height)
{
    SetWindowPos(windowHandle, 0, 0, 0, width, height, SWP_NOMOVE);
}
void getWindowPosition(HWND windowHandle, int* x, int* y)
{
	RECT windowRectangle;
	GetWindowRect(windowHandle, &windowRectangle);
	int right, bottom;
	getRect(&windowRectangle, x, y, &right, &bottom);
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
void getFileCount(WPARAM wParam, uint* count)
{
    HDROP drop = (HDROP)wParam;
	UINT magic = 0xffffffff;
	*count = DragQueryFile(drop, magic, 0, 128);
}
void getFilePath(WPARAM wParam, WCHAR* filePath, uint* characterCount, uint index)
{
	HDROP drop = (HDROP)wParam;
	*characterCount = DragQueryFile(drop, index, filePath, 256);
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
void getFileName(String* filePath, String* fileName)
{
    WCHAR* pathString = filePath->string;
    pathString += filePath->characterCount;

	uint count = {};
	while (*pathString != 47 && *pathString != 92)
	{
		++count;
		--pathString;
	}
	++pathString;

	fileName->string = pathString;
	fileName->characterCount = count;
}
