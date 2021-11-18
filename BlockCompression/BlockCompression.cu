#include "cuda_runtime.h"
#include "device_launch_parameters.h"

#include <sstream>
#include <vector>
#include "Compressor.cuh"
#include "DataTypes.h"
#include "Helpers.h"
#include "TagReader.h"
#include "TagTable.h"
#include "Timer.h"
#include "VolumeDesc.h"

#ifdef USE_TIMER // timer is in "Compressor.cuh"
float timerTotal = 0;
#include <iostream>
#endif

using namespace std;

// container for block of voxels
vector<BlockPlane> blockPlane;

// lookup between string names and uchar IDs
TagTable globalTT = TagTable();

// read file first line to get description
// and copy to device
__host__ void readFileDescription()
{
    // Get the description string from setup TagReader
    string description = TagReader::setup();

    // Parse and save the compression dimensions
    stringstream ss(description);
    char ignore;
    ss >> ignore // discard first char which is '#'
        >> VolumeDesc::totalSize.x
        >> VolumeDesc::totalSize.y
        >> VolumeDesc::totalSize.z
        >> VolumeDesc::pBlockSize.x
        >> VolumeDesc::pBlockSize.y
        >> VolumeDesc::pBlockSize.z;

    // Calculate the total number of voxels in the volume
    VolumeDesc::totalNumVoxels = getVolume(VolumeDesc::totalSize);

    // Calculate the total number of blocks in a parent block
    VolumeDesc::pBlockNumVoxels = getVolume(VolumeDesc::pBlockSize);

    // how many parent blocks fit in each dimension
    VolumeDesc::numPBlocks = VolumeDesc::totalSize / VolumeDesc::pBlockSize;

    // parent-block XY plane size
    VolumeDesc::bPlaneSize = {
    	VolumeDesc::totalSize.x,
    	VolumeDesc::totalSize.y,
    	VolumeDesc::pBlockSize.z };

    // volume of a parent block
    VolumeDesc::bPlaneNumVoxels = getVolume(VolumeDesc::bPlaneSize);
}

// reserve memory to store Volume on host and device
__host__ void reserveVolumeMemory() 
{
    // Reserve pinned memory for each 
    blockPlane.reserve(VolumeDesc::numPBlocks.z);

    for (ushort i = 0; i < VolumeDesc::numPBlocks.z; i++)
    {
        uint* data;
        cudaMallocHost((void**)&data, VolumeDesc::bPlaneNumVoxels * sizeof(uint));

        // calculate offset to origin of block plane
        ushort3 pBlockIndex = { 0, 0, i };
        ushort3 offset = pBlockIndex * VolumeDesc::bPlaneSize;

        blockPlane.push_back({ data, offset });
    }
}

// Initialise the compression vars and TagReader
__host__ void setup()
{
    readFileDescription();
    reserveVolumeMemory();
}

// Cleanup any memory allocated on the heap
__host__ void cleanup()
{
    // host copy of all block planes in the volume
    for (const BlockPlane& bPlane : blockPlane)
    {
        cudaFreeHost(bPlane.volume);
    }
}

// read voxels from file
__host__ void readVolume()
{
    // for each block plane
    for (ushort i = 0; i < VolumeDesc::numPBlocks.z; i++)
    {
        // read number of voxels that can fit in block plane
        for (ulong j = 0; j < VolumeDesc::bPlaneNumVoxels; j++)
        {

            uchar tagID = globalTT.getID(TagReader::getNextTagName());

            // store in block plane
            blockPlane[i].volume[j] = tagID;
        }
    }
}

int main()
{
    // Check there is an available CUDA enabled device
    cudaSetDevice(0);
    cudaCheckErrors("set Device failed");

    // initialise program
    setup();
    Compressor::setupDevice(VolumeDesc::totalSize, VolumeDesc::pBlockSize);

    // read voxels into block Planes
    readVolume();

    // compress block planes
    for (int i=0; i<blockPlane.size(); i++)
    {
        Compressor::compressPrint(blockPlane[i]);
    }

    // cleanup any allocated memory
    cleanup();
    Compressor::cleanupDevice();

#ifdef USE_TIMER
    cout << "Compression Time: " << timerTotal << endl;
#endif

    return 0;
}