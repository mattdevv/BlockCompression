#pragma once

#include <iostream>
#include <sstream>
#include <vector>
#include <string>

#include "TagReader.h"

#include "ParentBlock.h"
#include "TagTable.h"
#include "vec3.h"
#include "Timer.h"
#include "uDataTypes.h"

using namespace std;

class BlockPlane
{
private:
    static TagTable tagTable;                           // stores global ID for each possible tag
    static vec3<ushort> volumeDim;                      // how many voxels fit in volume per dimension
    static vec3<ushort> pBlockDim;                      // how many voxels fit in a parent-block per dimension
    static vec3<ushort> numPBlocks;                     // how many Parent-blocks fit in volume per dimension
    static ushort currentPlane;                         // which XY plane (of parent blocks) is next to read
    static ushort numInstances;                         // number of instances of BlockPlane
    static void readDimensions();                       // read dimensions to be used in creating BlockPlanes/ParentBlocks
    static string getTagFromChars(char* start);         // Get the tag from a input voxel description string

    vector<ParentBlock> parentBlocks;                   // vector of parent blocks
    ushort planeID;                                     // this BlockPlane's instance ID             
    void createParentBlocks();                          // allocate memory for this BlockPlane's ParentBlocks

public:
    static void setup();                                // call functions that prepare BlockPlane for use
    static bool canRead();                              // check whether there are more block planes to be read
    static bool canUseOnePlane();                       // checks whether 1 plane of parent blocks covers entire volume

    BlockPlane();
    void printBlockPlane();
    void readBlockPlane();
};
