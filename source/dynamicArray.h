#pragma once
#include "platform.h"


struct DynamicArray
{
	void* firstElement;
	uint totalSlotCount;
	uint freeSlotCount;
	uint occupiedSlotCount;
	uint slotSize;
};

void arrayAppend(void*, void*);
void createArray(void**, uint);
void freeArray(void*);
void getArrayStart(void*, void**);
void getElementCount(void*, uint*);
void getArray(void*, void**, uint*);
void resetArray(void*);
