#pragma once
#include "define.hpp"
#include "dataType.hpp"

struct String
{
    WCHAR* string;
    uint characterCount;
    int padding;
};
