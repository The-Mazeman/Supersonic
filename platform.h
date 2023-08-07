#pragma once
#include "header.h"

struct Position
{
    int x;
    int y;
};
void assert(bool result);

void createThread(LPTHREAD_START_ROUTINE startRoutine, LPVOID parameter, DWORD* threadId);
void createEvent(BOOL manualResetRequired, HANDLE* handle);
void createMutex(int initialState, HANDLE* handle);
void createSemaphore(uint initialCount, uint maximumCount, HANDLE* handle);

void getProcessHeap(HANDLE* handle);

void allocateSmallMemory(uint64 size, void** memory);
void freeSmallMemory(void* memory);
void allocateBigMemory(uint64 size, char** memory);
void freeBigMemory(void* memory);

void openFileHandle(WCHAR* filePath, DWORD flag, HANDLE* handle);
void closeFileHandle(HANDLE fileHandle);
void readFromFile(HANDLE fileHandle, uint64* memorySize, char** memory);
void writeToFile(HANDLE fileHandle, char* memory, uint64 memorySize);
void loadFile(WCHAR* filePath, char** memory);

void getModuleHandle(HINSTANCE* instance);

void createWindowClass(const WCHAR* className, WNDPROC callbackFunction);
void createWindow(const WCHAR* className, int width, int height, HWND* window);
void createChildWindow(const WCHAR* className, HWND parentHandle, HWND* window);

void getRect(RECT* rectangle, int* left, int* top, int* right, int* bottom);
void getWindowRectangle(HWND windowHandle, int* left, int* top, int* right, int* bottom);
void getWindowDimension(HWND windowHandle, int* width, int* height);
void getWindowPosition(HWND windowHandle, int* x, int* y);

void placeWindow(HWND, int, int, int, int);
void rectangleFrame(HDC deviceContext, RECT* invalidRectangle, uint color);
void rectangleFill(HDC deviceContext, RECT* invalidRectangle, uint color);

void mapToParent(HWND window, int* x, int* y);
void startMessageLoop(void);
void handleWindowSizeChanged(HWND window, LPARAM lParam);
void resizeWindow(HWND window, int width, int height);
void resizeWindow(HWND window, LPARAM lParam);
void handleHorizontalScroll(HWND window, LPARAM lParam);
void handleVerticalScroll(HWND window, LPARAM lParam);
void zoomWindow(HWND window, LPARAM oldFramesPerPixel);

void drawMarking(HDC deviceContext, RECT* invalidRectangle, int offset);
void drawGrid(HDC deviceContext, RECT* invalidRectangle, int offsetX);

void getFilePath(WPARAM wParam, WCHAR* filePath, uint index);
void getFileCount(WPARAM wParam, uint* count);
void horizontalScroll(HWND window, LPARAM lParam);
void getTrackNumber(HWND window, int* trackNumber, int height);

void boundCheck(RingBuffer* ringBuffer, void** bufferPointer, uint offset);
void checkCompletion(uint* inputFinishCount, uint trackCount, HANDLE inputSemaphore);