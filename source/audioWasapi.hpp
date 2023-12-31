#pragma once
#include "platformWindows.hpp"
#include "dynamicArray.hpp"
#include "dataStructure.hpp"
#include "waveFormat.hpp"
#include "buffer.hpp"

START_SCOPE(audioWasapi)

const CLSID CLSID_MMDeviceEnumerator = { 0xbcde0395, 0xe52f, 0x467c, {0x8e, 0x3d, 0xc4, 0x57, 0x92, 0x91, 0x69, 0x2e} };
const IID   IID_IMMDeviceEnumerator = { 0xa95664d2, 0x9614, 0x4f35, {0xa7, 0x46, 0xde, 0x8d, 0xb6, 0x36, 0x17, 0xe6} };
const IID   IID_IMMNotificationClient = { 0x7991eec9, 0x7e89, 0x4d85, {0x83, 0x90, 0x6c, 0x70, 0x3c, 0xec, 0x60, 0xc0} };
const IID   IID_IAudioClient = { 0x1cb9ad4c, 0xdbfa, 0x4c32, {0xb1, 0x78, 0xc2, 0xf5, 0x68, 0xa7, 0x03, 0xb2} };
const IID   IID_IAudioClient3 = { 0x7ed4ee07, 0x8e67, 0x4cd4, {0x8c, 0x1a, 0x2b, 0x7a, 0x59, 0x87, 0xad, 0x42} };
const IID   IID_IAudioRenderClient = { 0xf294acfc, 0x3146, 0x4483, {0xa7, 0xbf, 0xad, 0xdc, 0xa7, 0xc2, 0x60, 0xe2} };
const IID   IID_IAudioSessionControl = { 0xf4b1a599, 0x7266, 0x4319, {0xa8, 0xca, 0xe7, 0x0a, 0xcb, 0x11, 0xe8, 0xcd} };
const IID   IID_IAudioSessionEvents = { 0x24918acc, 0x64b3, 0x37c1, {0x8c, 0xa9, 0x74, 0xa6, 0x6e, 0x99, 0x57, 0xa8} };
const IID   IID_IMMEndpoint = { 0x1be09788, 0x6894, 0x4089, {0x85, 0x86, 0x9a, 0x2a, 0x6c, 0x26, 0x5a, 0xc5} };
const IID   IID_IAudioClockAdjustment = { 0xf6e4c0a0, 0x46d9, 0x4fb8, {0xbe, 0x21, 0x57, 0xa3, 0xef, 0x2b, 0x62, 0x6c} };
const IID   IID_IAudioCaptureClient = { 0xc8adbd64, 0xe71e, 0x48a0, {0xa4, 0xde, 0x18, 0x5c, 0x39, 0x5c, 0xd3, 0x17} };
const IID   IID_IAudioClock = { 0xcd63314f, 0x3fba, 0x4a1b, {0x81, 0x2c, 0xef, 0x96, 0x35, 0x87, 0x28, 0xe7} };

struct State
{
	IAudioRenderClient* renderClient;
	IAudioClient* audioClient;
	IAudioClock* audioClock;

	Header format;
    float* buffer;
    float* endpointBuffer;
	uint64 endpointDeviceFrequency;
	HANDLE audioCallback;
	HANDLE endpointLoaderStartEvent;
    HANDLE endpointLoaderFinishEvent;
    HANDLE processorStartEvent;
    HANDLE processorFinishEvent;

	HANDLE exitSemaphore;
	HANDLE finishSemaphore;
	uint frameCount;
    uint inputLoaderCount;

    void* inputTrackArrayHandle;
    void* startEventArrayHandle;
	HWND clipAreaWindow;
};
struct EndpointControllerInfo
{
	IAudioClient* audioClient;
    HANDLE endpointBufferMutex;
	HANDLE endpointLoaderStartEvent;
    HANDLE endpointLoaderFinishEvent;
	HANDLE audioCallback;
    HANDLE exitSemaphore;
    uint frameCount;
    int padding;
    void* trackStartEventArrayHandle;
};
struct EndpointLoaderInfo
{
	HANDLE endpointLoaderStartEvent;
    HANDLE endpointLoaderFinishEvent;
    HANDLE processorStartEvent;
    HANDLE processorFinishEvent;
    HANDLE exitSemaphore;
	HANDLE finishSemaphore;
	IAudioRenderClient* renderClient;
	float* buffer;
    uint frameCount; 
    int padding;
};
struct EndpointProcessorInfo
{
    HANDLE processorStartEvent;
    HANDLE processorFinishEvent;
    HANDLE finishSemaphore;
    HANDLE exitSemaphore;
    uint inputLoaderCount;
    uint frameCount;
    float* buffer;
    float* endpointBuffer;
};

void create(HWND parent, HWND* window, uint frameCount, HWND clipAreaWindow);

END_SCOPE
