#include "BlockPlane.h"

using namespace std;

TagTable<string, uchar> BlockPlane::tagTable = TagTable<string, uchar>();   // table of tags and associated ID's that have been previously seen
vec3<ushort> BlockPlane::volumeDim = { 1, 1, 1 };                           // how many voxels fit in volume per dimension
vec3<ushort> BlockPlane::pBlockDim = { 1, 1, 1 };                           // how many voxels fit in a parent-block per dimension
vec3<ushort> BlockPlane::numPBlocks = { 1, 1, 1 };                          // how many Parent-blocks fit in volume per dimension
ushort BlockPlane::currentPlane = 0;                                        // which XY plane (of parent blocks) is next to read
int BlockPlane::numInstances = 0;                                           // how many Block Planes are active, used for shifting BlockPlane's Z position

void BlockPlane::setup()
{
    // Set the dimensions
    readDimensions();
}

BlockPlane::BlockPlane()
{
    // create blocks for input to be stored in
    createParentBlocks();

    ID = numInstances;
    numInstances++;
}

#ifdef DEBUG
// exact path to input file
std::ifstream is("D:/Documents/UNI/2021/Semester 2/Software Engineering Project/runner/the_intro_one_32768_4x4x4_2.csv", std::ifstream::binary);
#endif

// Get the first input line and set all dimension members
inline void BlockPlane::readDimensions()
{
    // Get the dimension description
    // always contained in first line
    
#ifdef DEBUG
    string description = "";
    getline(is, description);
#else
    string description = TagReader::setup();
#endif

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

    // create 2D grid of parent blocks
    parentBlocks.reserve(numPBlocks.x * numPBlocks.y);
    for (ushort y = 0; y < numPBlocks.y; y++)
    {
        for (ushort x = 0; x < numPBlocks.x; x++)
        {
            // setup chunk world space origin for first (0th) layer
            vec3<ushort> chunkIndex = { x, y, (ushort)numInstances };
            parentBlocks.push_back(ParentBlock(chunkIndex * pBlockDim));
        }
    }
}

// Read and store the next block plane in voxels
void BlockPlane::readBlockPlane()
{
    //Timer t("read");

    // Set re-used variables for reading input
#ifdef DEBUG
    char chars[MAX_LINE_LENGTH];
#endif

    // index of current parent block
    unsigned int pBlockIndex = 0;

    // read 1 block plane of voxels into structure
    // move along 1 voxel in z
    for (ushort a = 0; a < pBlockDim.z; a++)
    {
        // moves along 1 pBlock along y
        for (ushort b = 0; b < numPBlocks.y; b++)
        {
            // move along 1 voxel along y
            for (ushort c = 0; c < pBlockDim.y; c++)
            {
                // move along 1 pBlock in x
                for (ushort d = 0; d < numPBlocks.x; d++)
                {
                    // move along 1 voxels in x
                    for (ushort e = 0; e < pBlockDim.x; e++)
                    {
#ifdef DEBUG
                        is.getline(chars, MAX_LINE_LENGTH);

                        // extract tag and convert to ID
                        string tagName = getTagFromChars(chars);
#else
                        // Get next voxel description
                        string tagName = TagReader::getNextTagName();
#endif
                        uchar tagID = tagTable.getID(tagName);

                        // store in a parent block
                        parentBlocks[pBlockIndex].insertVoxel(tagID);
                    }
                    pBlockIndex++;
                }
                pBlockIndex -= numPBlocks.x;
            }
            pBlockIndex += numPBlocks.x;
        }
        pBlockIndex = 0;
    }
    
    currentPlane++;

    //t.stop();
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
    //Timer t("write");

    // Print all blocks
    for (size_t i=0; i<parentBlocks.size(); i++)
    {
        // use compression and print this block
        parentBlocks[i].compressPrint();

        // reset storage and increment parentBlock z position ready for next BlockPlane
        parentBlocks[i].reset(numInstances);
    }

    //t.stop();
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

        cout << "BIG ERROR, read too many block planes\n";
        exit(2);
    }

    return true;
}