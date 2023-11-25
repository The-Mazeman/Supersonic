#pragma once 

#include "define.hpp"
#include "dataType.hpp"
#include "dataStructure.hpp"
#include "platformWindows.hpp"
#include "waveFormat.hpp"

struct AudioClip
{
    String fileName;
    String filePath;
    float* sampleChunk;
    uint64 frameCount;
    uint64 startFrame;
    uint64 endFrame;
};

START_SCOPE(audioClip)

void create(String* filePathArray, uint64 startFrame, uint fileCount, AudioClip** audioClipArray);

END_SCOPE
