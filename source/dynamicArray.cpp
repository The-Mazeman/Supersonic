#include "include.hpp"
#include "dynamicArray.hpp"

void createDynamicArray(void** arrayHandle, uint16 slotSize)
{
    DynamicArray* dynamicArray = {};
    allocateMemory(sizeof(DynamicArray), (void**)&dynamicArray);
    dynamicArray->totalSlotCount = 0;
    dynamicArray->occupiedSlotCount = 0;
    dynamicArray->freeSlotCount = 0;
    dynamicArray->slotSize = slotSize;
    *arrayHandle = (char*)dynamicArray;
}
void increaseSlotCount(DynamicArray* dynamicArray, uint deltaElementCount)
{
    uint totalCount = dynamicArray->totalSlotCount;
    uint slotSize = dynamicArray->slotSize;
    uint newCount = totalCount + deltaElementCount;
    uint newSize = newCount * slotSize;
    uint oldSize = totalCount * slotSize;

    void* newArray = {};
    allocateMemory(newSize, &newArray);

    void* oldArray = dynamicArray->array;
    memcpy(newArray, oldArray, oldSize);
    dynamicArray->array = newArray;
    dynamicArray->totalSlotCount = (uint16)newCount;
    dynamicArray->freeSlotCount = (uint16)deltaElementCount;
}
void resizeArray(void* arrayHandle, uint deltaElementCount)
{
    DynamicArray* dynamicArray = (DynamicArray*)arrayHandle;
    increaseSlotCount(dynamicArray, deltaElementCount);
}
void appendElement(void* arrayHandle, void* element)
{
    DynamicArray* dynamicArray = (DynamicArray*)arrayHandle;
    uint freeSlotCount = dynamicArray->freeSlotCount;
    if(freeSlotCount == 0)
    {
        increaseSlotCount(dynamicArray, 1);
    }
    void* array = dynamicArray->array;
    uint occupiedSlotCount = dynamicArray->occupiedSlotCount;
    uint slotSize = dynamicArray->slotSize;
    void* elementSlot = (char*)array + (occupiedSlotCount * slotSize);
    memcpy(elementSlot, element, slotSize);

    ++dynamicArray->occupiedSlotCount;
    --dynamicArray->freeSlotCount;
}
void getOccupiedSlotCount(void* arrayHandle, uint* slotCount)
{
    DynamicArray* dynamicArray = (DynamicArray*)arrayHandle;
    *slotCount = dynamicArray->occupiedSlotCount;
}
void getArray(void* arrayHandle, void** array, uint* slotCount)
{
    DynamicArray* dynamicArray = (DynamicArray*)arrayHandle;
    *array = dynamicArray->array;
    *slotCount = dynamicArray->occupiedSlotCount;
}
void getArrayStart(void* arrayHandle, void** array)
{
    DynamicArray* dynamicArray = (DynamicArray*)arrayHandle;
    *array = dynamicArray->array;
}
void resetArray(void* arrayHandle)
{
    DynamicArray* dynamicArray = (DynamicArray*)arrayHandle;
    dynamicArray->freeSlotCount = dynamicArray->totalSlotCount;
    dynamicArray->occupiedSlotCount = 0;
}
