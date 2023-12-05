#include "include.hpp"
#include "audioUtilities.hpp"

void convertDecibelToGain(float decibel, float* gain)
{
    *gain = (float)pow(10.0f, decibel / 20.0f);
}
void convertGainToDecibel(float gain, float* decibel)
{
    *decibel = 20.0f * (float)log(gain);
}


