#pragma once
#include "dataType.hpp"
#include "define.hpp"
#include "dataStructure.hpp"
#include "platformWindows.hpp"

struct Parameter
{
    uint id;
    float minimum;
    float maximum;
    float delta;
    float value;
    int padding;
};
void convertDecibelToGain(float decibel, float* gain);
void convertGainToDecibel(float gain, float* decibel);
