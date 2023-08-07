#pragma once

#pragma warning(disable: 5045)
#if 0
#pragma warning(disable: 4100)
#pragma warning(disable: 4189)
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

#define WM_RESIZE 0x8001
#define WM_PLAY 0x8002
#define WM_PAUSE 0x8003
#define WM_STARTLOADER 0x8004
#define WM_FILEDROP 0x8005
#define WM_CREATETRACK 0x8006
#define WM_ASSIGNBUS 0x8007
#define WM_MOVECURSOR 0x8008
#define WM_GETCURSOR 0x8009
#if 0
#define WM_HORIZONTALMOUSEWHEEL 0x8002
#define WM_VERTICALMOUSEWHEEL 0x8003
#define WM_PINCHZOOM 0x8004
#define WM_SETTIMER 0x8007
#define WM_TOGGLEPLAYBACK 0x8008
#define WM_SETCALLBACK 0x800a
#define WM_MOVECURSOR 0x800b
#define WM_GETOUTPUT 0x800e
#define WM_SETOUTPUT 0x8010
#endif


#define AVX2_FRAME_SIZE 32


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

struct Loader
{
	RingBuffer buffer;
	HANDLE loadEvent;
	HANDLE exitSemaphore;
	HANDLE finishSemaphore;
	uint* finishCount;

	uint trackCount;
	uint trackNumber;

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
	float* sampleChunk;
	String name;
};
struct AudioClip
{
	uint64 startFrame;
	uint64 endFrame;
	uint startOffset;
	uint endOffset;

	int x;
	int width;
	void* start;
	uint64 frameCount;
	uint64 id;
	WaveFile waveFile;
	float* waveformChunk;
};



