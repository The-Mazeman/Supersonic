#include "include.hpp"
#include "audioClip.hpp"

START_SCOPE(audioClip)
    
void create(String* filePathArray, uint64 startFrame, uint fileCount, AudioClip** audioClipArray)
{
    allocateMemory(sizeof(AudioClip) * fileCount, (void**)audioClipArray);
    AudioClip* audioClip = *audioClipArray;
    for(uint i = 0; i != fileCount; ++i)
    {
        audioClip[i].filePath = filePathArray[i];
        audioClip[i].startFrame = startFrame;
        getFileName(&filePathArray[i], &audioClip[i].fileName);
        waveFormat::create(filePathArray[i].string, &audioClip[i].sampleChunk, &audioClip[i].frameCount);
        audioClip[i].endFrame = startFrame + audioClip[i].frameCount;
    }
}

END_SCOPE
