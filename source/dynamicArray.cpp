#include "header.h"
#include "dynamicArray.h"

void createArray(void** arrayHandle, uint slotSize)
{
	DynamicArray* dynamicArray = {};
	allocateSmallMemory(sizeof(DynamicArray), (void**)&dynamicArray);
	dynamicArray->firstElement = 0;
	dynamicArray->totalSlotCount = 0;
	dynamicArray->freeSlotCount = 0;
	dynamicArray->occupiedSlotCount = 0;
	dynamicArray->slotSize = slotSize;
	*arrayHandle = dynamicArray;
}
void arrayAppend(void* arrayHandle, void* source)
{
	DynamicArray* dynamicArray = (DynamicArray*)arrayHandle;
	if(dynamicArray->freeSlotCount == 0)
	{
		uint newSlotCount = dynamicArray->totalSlotCount + 1;
		uint slotSize = dynamicArray->slotSize;
		uint newArraySize = newSlotCount * slotSize;
		char* newArray = {};
		allocateSmallMemory(newArraySize, (void**)&newArray);

		void* oldArray = dynamicArray->firstElement;
		if(oldArray)
		{
			uint oldArraySize = newArraySize - slotSize;
			memcpy(newArray, oldArray, oldArraySize);
			freeSmallMemory(oldArray);
		}

		dynamicArray->firstElement = newArray;
		++dynamicArray->totalSlotCount;
	}
	uint slotNumber = dynamicArray->occupiedSlotCount;
	uint slotSize = dynamicArray->slotSize;
	void* destination = (char*)dynamicArray->firstElement + (slotNumber * slotSize);
	memcpy(destination, source, slotSize);
	++dynamicArray->occupiedSlotCount;
}
void getArray(void* arrayHandle, void** firstElement, uint* elementCount)
{
	DynamicArray* dynamicArray = (DynamicArray*)arrayHandle;
	*firstElement = dynamicArray->firstElement;
	*elementCount = dynamicArray->occupiedSlotCount;
}
void getArrayStart(void* arrayHandle, void** firstElement)
{
	DynamicArray* dynamicArray = (DynamicArray*)arrayHandle;
	*firstElement = dynamicArray->firstElement;
}
void getElementCount(void* arrayHandle, uint* elementCount)
{
	DynamicArray* dynamicArray = (DynamicArray*)arrayHandle;
	*elementCount = dynamicArray->occupiedSlotCount;
}
void resetArray(void* arrayHandle)
{
	DynamicArray* dynamicArray = (DynamicArray*)arrayHandle;
	dynamicArray->freeSlotCount = dynamicArray->totalSlotCount;
	dynamicArray->occupiedSlotCount = 0;
}
void freeArray(void* arrayHandle)
{
	DynamicArray* dynamicArray = (DynamicArray*)arrayHandle;
	void* firstElement = dynamicArray->firstElement;
	freeSmallMemory(firstElement);
	freeSmallMemory(dynamicArray);
}
