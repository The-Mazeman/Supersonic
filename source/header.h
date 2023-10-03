#pragma once

#pragma warning(disable: 5045)
#pragma warning(disable: 4100)
#pragma warning(disable: 4189)
#if 0
#endif

#define WIN32_LEAN_AND_MEAN  
#define CINTERFACE  

#include <windows.h>
#include <windowsx.h>
#include <Uxtheme.h>
#include <shellapi.h>

#include <immintrin.h>
#include <stdint.h>
#include <limits.h>
#include <math.h>

#include <mmdeviceapi.h>
#include <audiopolicy.h>
#include <audioclient.h>
#include <functiondiscoverykeys_devpkey.h>

#define notUsing(x) (x)

#define sint8  int8_t
#define sint8  int8_t
#define sint16 int16_t
#define sint32 int32_t
#define sint64 int64_t

#define uint8  uint8_t
#define uint16 uint16_t
#define uint32 uint32_t
#define uint64 uint64_t

#define uint unsigned int
#define ushort unsigned short

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720
#define FRAMES_TO_AVERAGE 1024

#define COLOR_WHITE 0x00ffffff
#define COLOR_BLACK 0x00000000
#define COLOR_GREY 0x00333333

#define COLOR_GREEN 0x0000ff00
#define COLOR_RED   0x000000ff

#ifndef WM_NCUAHDRAWCAPTION
#define WM_NCUAHDRAWCAPTION (0x00ae)
#endif
#ifndef WM_NCUAHDRAWFRAME
#define WM_NCUAHDRAWFRAME (0x00af)
#endif
#define START_SCOPE(x) namespace x {
#define END_SCOPE }

#define FADER_0 0
#define FADER_1 1
#define FADER_2 2
#define FADER_3 3

#define WM_RESIZE 0x8001
#define WM_PLAY 0x8002
#define WM_PAUSE 0x8003
#define WM_STARTOUTPUTLOADER 0x8004
#define WM_FILEDROP 0x8005
#define WM_CREATETRACK 0x8006
#define WM_SETOUTPUT 0x8007
#define WM_MOVECURSOR 0x8008
#define WM_GETCURSOR 0x8009
#define WM_SETDROP 0x800a
#define WM_SETCONTROL 0x800b
#define WM_SETINPUT 0x800c
#define WM_STARTINPUTLOADER 0x800d
#define WM_SETTRACKARRAY 0x800e
#define WM_SETOUTPUTLOADER 0x800f
#define WM_SETINPUTLOADER 0x8010
#define WM_SENDINPUTLOADER 0x8011
#define WM_SENDOUTPUTLOADER 0x8012
#define WM_CREATEBUFFER 0x8013
#define WM_SENDCONTROL 0x8014
#define WM_FADERMOVE 0x8015

#define AVX2_FRAME_SIZE 32

typedef void (*ProcessFunction)(float*, uint, void*);
struct WindowPosition
{
	int x;
	int y;
	int width;
	int height;
};
struct String
{
	WCHAR* string;
	uint64 characterCount;
};
struct RingBuffer
{
	char* start;
	char* end;
};
struct Header
{
	uint16 type;
	uint16 channelCount;
	uint32 sampleRate;
	uint32 byteRate;
	uint16 blockAlign;
	uint16 bitDepth;
};
struct WaveFile
{
	Header header;
	uint64 frameCount;
	String name;
	float* sampleChunk;
};
struct AudioClip
{
	uint64 startFrame;
	uint64 endFrame;
	uint64 frameCount;
	uint startOffset;
	uint endOffset;

	WaveFile waveFile;
};
struct AudioEffect
{
	ProcessFunction process;
	void* state;
};
struct TrackControl
{
	__m256 gain;
	__m256 pan;
};
struct BufferInfo
{
	float* buffer;
	HANDLE bufferCompleteSemaphore;
	uint loaderCount;
	uint loaderPosition;
};
struct BusLoaderInfo
{
	TrackControl trackControl;
	BufferInfo outputBufferInfo;
	float* inputBuffer;
    HANDLE exitSemaphore;
    HANDLE startBusLoaderEvent;
	uint64 padding1;
	uint64 padding2;
};
struct MasterBusLoaderInfo
{
	BufferInfo outputBufferInfo;
	float* inputBuffer;
    HANDLE exitSemaphore;
    HANDLE startBusLoaderEvent;
    HANDLE loadOutputEvent;
};
struct BusProcessorInfo
{
    HANDLE bufferCompleteSemaphore;
    HANDLE exitSemaphore;
    float* inputBuffer;
    HANDLE* busLoaderStartEventArray;
    uint outputLoaderCount;
    uint inputLoaderCount;
};
struct TrackProcessorInfo
{
    HANDLE startTrackProcessorEvent;
    HANDLE exitSemaphore;
    AudioClip** clipList;
    float* inputBuffer;
    HANDLE* busLoaderStartEventArray;
    uint outputLoaderCount;
    uint clipCount;
};


