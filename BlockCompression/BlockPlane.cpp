#include "BlockPlane.h"

using namespace std;

TagTable BlockPlane::tagTable = TagTable();                                 // table of tags and associated ID's that have been previously seen
vec3<ushort> BlockPlane::volumeDim = { 1, 1, 1 };                   // how many voxels fit in volume per dimension
vec3<ushort> BlockPlane::pBlockDim = { 1, 1, 1 };                   // how many voxels fit in a parent-block per dimension
vec3<ushort> BlockPlane::numPBlocks = { 1, 1, 1 };                  // how many Parent-blocks fit in volume per dimension
ushort BlockPlane::currentPlane = 0;                                        // which XY plane (of parent blocks) is next to read
ushort BlockPlane::numInstances = 0;                                           // how many Block Planes are active, used for shifting BlockPlane's Z position

void BlockPlane::setup()
{
    // Set the dimensions
    readDimensions();
}

BlockPlane::BlockPlane()
{
    // track how many BlockPlanes exist and give each an ID
    planeID = numInstances;
    numInstances++;

    // create blocks for input to be stored in
    createParentBlocks();
}

// Get the first input line and set all dimension members
inline void BlockPlane::readDimensions()
{
    // Get the dimension description
    // always contained in first line
    

    string description = TagReader::setup();

    // Replace comma characters with whitespace
    for (auto& c : description) 
        if (c == ',') c = ' ';

    // Parse volume description
    stringstream ss(description);
    char ignore;
    ss >> ignore
        >> volumeDim.x
        >> volumeDim.y
        >> volumeDim.z
        >> pBlockDim.x
        >> pBlockDim.y
        >> pBlockDim.z;

    //pBlockDim.x = 2064;
    //pBlockDim.y = 1840;
    //pBlockDim.z = 260;

    // how many Parent-blocks fit in x and y and z dimensions
    numPBlocks =
    {
        (ushort)(volumeDim.x / pBlockDim.x),
        (ushort)(volumeDim.y / pBlockDim.y),
        (ushort)(volumeDim.z / pBlockDim.z)
    };

    currentPlane = 0;
}

// Using the dimension information, allocate memory required to store a parent-block-plane's voxels and origin coordinates
inline void BlockPlane::createParentBlocks()
{
    // set important static variables in ParentBlock
    ParentBlock::setup(pBlockDim, &tagTable);

    // create 2D plane of parent blocks
    // order is important to read voxels correctly
    parentBlocks.reserve(numPBlocks.x * numPBlocks.y);
    for (ushort y = 0; y < numPBlocks.y; y++)
    {
        for (ushort x = 0; x < numPBlocks.x; x++)
        {
            // setup chunk world space origin for the layer this plane represents
            // chunk z starts at the instance number of this BlockPlane
            vec3<ushort> chunkIndex = { x, y, planeID };
            parentBlocks.emplace_back(chunkIndex * pBlockDim);
        }
    }
}

// Read and store the next block plane as lines of voxels
void BlockPlane::readBlockPlane()
{
    //Timer timerRead("read");

    // index of current parent block
    unsigned int pBlockIndex = 0;

    // each voxel along z
    for (ushort a = 0; a < pBlockDim.z; a++)
    {
        // each pBlock along y
        for (ushort b = 0; b < numPBlocks.y; b++)
        {
            // each voxel inside pBlock y
            for (ushort c = 0; c < pBlockDim.y; c++)
            {
                // each voxel along total x
                for (ushort d = 0; d < numPBlocks.x; d++)
                {
                    // find lines of tags by breaking lines when the next tag changes
                    // always break the line when crossing a parent block boundary
                    ushort length = 1;

                    // first tag type
                    uchar prevID = tagTable.getID(TagReader::getNextTagName());

                    // each voxel in parent block x
                    // start at 2nd voxel
                    for (ushort e = 1; e < pBlockDim.x; e++)
                    {
                        // Get next voxel description
                        string tagName = TagReader::getNextTagName();
                        uchar tagID = tagTable.getID(tagName);

                        // if next tag is same just increase line length
                        if (tagID == prevID)
                        {
                            // increase line length
                            length++;
                        }
                        // else break the line and store the previous line's tag and size
                        else
                        {
                            // store in a parent block
                            parentBlocks[pBlockIndex].insertBlockLine({ (ushort)(e - length), c, a }, length, prevID);

                            // reset line
                            prevID = tagID;
                            length = 1;
                        }
                    }

                    // manually save final line in this parent block
                    parentBlocks[pBlockIndex].insertBlockLine({ (ushort)(pBlockDim.x - length), c, a }, length, prevID);

                    pBlockIndex++;
                }
                pBlockIndex -= numPBlocks.x;
            }
            pBlockIndex += numPBlocks.x;
        }
        pBlockIndex = 0;
    }
    
    currentPlane++;

    //timerRead.print();
}

// Read voxel description forwards to get tag
inline string BlockPlane::getTagFromChars(char* start)
{
    // index 6 is the first place the ' symbol might appear
    start += 6;

    // find first ' symbol
    while (*start != '\'') start++;

    // move over first ' symbol
    start++;

    // save
    char* end = start;

    // find second ' symbol
    while (*end != '\'') end++;

    // extract string between start and end coordinates
    return string(start, end - start);
}

// Print all blocks in parentBlocks
void BlockPlane::printBlockPlane()
{
    //Timer timerWrite("write", true);

    // Print all blocks
    for (auto& parentBlock : parentBlocks)
    {
        // use compression and print this block
        parentBlock.compressPrint();

        // reset storage and increment parentBlock z position ready for next BlockPlane
        parentBlock.reset(numInstances);
    }

    //timerWrite.print();
}

// check if all planes have been read
bool BlockPlane::canRead()
{
    // If we have done all planes, return exit flag
    if (currentPlane >= numPBlocks.z)
    {
        if (currentPlane == numPBlocks.z)
        {
            // exit correctly
            return false;
        }

        cerr << "BIG ERROR, read too many block planes\n";
        exit(2);
    }

    return true;
}

bool BlockPlane::canUseOnePlane()
{
    return pBlockDim.z == volumeDim.z;
}
