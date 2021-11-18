#pragma once

#include <iostream>
#include "cuda_runtime.h"
#include "device_launch_parameters.h"
#include "DataTypes.h"
#include "TagTable.h"
#include "Helpers.h"

//#define USE_TIMER

#ifdef USE_TIMER
extern float timerTotal;
#endif

// from main.cu
extern TagTable globalTT;

namespace Compressor
{
	// exposed methods
	__host__ void setupDevice(ushort3& descriptionVolumeSize, ushort3& descriptionParentBlockSize);
	__host__ void cleanupDevice();
	__host__ void compressPrint(BlockPlane& pBlock);
}