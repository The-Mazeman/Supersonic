#pragma once
#include "dataType.hpp"
#include "platformWindows.hpp"

struct DynamicArray
{
    void* array;
    uint16 totalSlotCount;
    uint16 occupiedSlotCount;
    uint16 freeSlotCount;
    uint16 slotSize;
};

void createDynamicArray(void** arrayHandle, uint16 slotSize);
void appendElement(void* arrayHandle, void* element);
void getOccupiedSlotCount(void* arrayHandle, uint* slotCount);
void getArray(void* arrayHandle, void** array, uint* slotCount);
void resizeArray(void* arrayHandle, uint deltaElementCount);
void getArrayStart(void* arrayHandle, void** array);
void resetArray(void* arrayHandle);

