#pragma once

#include "cuda_runtime.h"

/* --------  --------  typedefs  --------  -------- */
typedef unsigned char       uchar;
typedef unsigned short      ushort;
typedef unsigned int        uint;
typedef unsigned long       ulong;
typedef unsigned long long  ull;

struct BlockPlane
{
    uint* volume;
    ushort3 offset;
};