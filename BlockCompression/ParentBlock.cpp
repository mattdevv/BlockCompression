#include "ParentBlock.h"

vec3<ushort> ParentBlock::pBlockDim = { 1, 1, 1 };
vec3<unsigned long> ParentBlock::translations = { 1, 1, 1 };
TagTable* ParentBlock::tt = nullptr;

// static method for setup of ParentBlock class
// store information that will be constant for every parent block
// must be called before creating a ParentBlock instance
void ParentBlock::setup(vec3<ushort> dimensions, TagTable* planeTagTable)
{
    // dimension of parent block
    pBlockDim = dimensions;

    // number of 1D voxels needed to move for each dimension of 3D movement
    translations = { 1, dimensions.x, (ulong)(dimensions.x * dimensions.y) };

    // tag table used for lookups
    tt = planeTagTable;
}

// use pre-calculated translations to find 1D position from 3D
inline uint ParentBlock::convert3DIndexTo1D(const vec3<ushort>& position) 
{
    return position.x + position.y * translations.y + position.z * translations.z;
}

// update the index volume for all active blocks
inline void ParentBlock::refreshBlockIndices()
{
    for (const Block& block : blocks)
    {
        if (block.isValid)
            fillSubVolume(blockIndices[block.index], block.subVolume);
    }
}

ParentBlock::ParentBlock(vec3<ushort> _originWS)
{
    // create a 1D array to hold the contents of a 3D volume
	blockIndices.resize(pBlockDim.volume());

    // set parent block's voxel offset from total volume origin
    originWS = _originWS;

    currentIndex = 0;
}

inline void ParentBlock::fillSubVolume(uint newValue, const SubVolume& subVolume)
{
    // 1D position of sub-volume origin within parent-block
    unsigned long startIndex = convert3DIndexTo1D(subVolume.origin);

    for (int z = 0; z < subVolume.size.z; z++)
    {
        // offset to the start of XY plane at Z=z
        unsigned long lookupIndex = startIndex;

        for (int y = 0; y < subVolume.size.y; y++)
        {
            for (int x = 0; x < subVolume.size.x; x++)
            {
                // move up X, x times
                blockIndices[lookupIndex + x] = newValue;
            }

            // reset X
            // move up Y once
            lookupIndex += translations.y;
        }

        // reset X and Y
        // move up Z once
        startIndex += translations.z;
    }
}

// expand a Block along Y to cover a SubVolume
// assumes the origin/size is exact same for X & Z
inline void ParentBlock::mergeUpY(Block& block, const SubVolume& subVolume)
{
    // top-most index of block
    ushort top1 = block.subVolume.origin.y + block.subVolume.size.y;
    // top-most index of subVolume
    ushort top2 = subVolume.origin.y + subVolume.size.y;
    ushort deltaLength = top2 - top1;

    // the volume the block will expand to cover
    SubVolume difference = block.subVolume;
    difference.origin.y += block.subVolume.size.y;
    difference.size.y = deltaLength;

    // update all the blockIndices in the difference to the new block
    fillSubVolume(blockIndices[block.index], difference);

    // grow block below to stretch to the top of the input volume
    block.subVolume.size.y += deltaLength;
}

// expand a Block along Z to cover a SubVolume
// assumes the origin/size is exact same for X & Y
inline void ParentBlock::mergeUpZ(Block& block, const SubVolume& subVolume)
{
    // top-most index of block
    ushort top1 = block.subVolume.origin.z + block.subVolume.size.z;
    // top-most index of subVolume
    ushort top2 = subVolume.origin.z + subVolume.size.z;
	ushort deltaLength = top2 - top1;

    // the volume the block will expand to cover
    SubVolume difference = block.subVolume;
    difference.origin.z += block.subVolume.size.z;
    difference.size.z = deltaLength;

    // update all the blockIndices in the difference to the new block
    fillSubVolume(blockIndices[block.index], difference);

    // grow block below to stretch to the top of the input volume
    block.subVolume.size.z += deltaLength;

    
}

inline bool ParentBlock::shelfCompressY(const SubVolume& subVolume, uint index, uchar topID)
{
    const uint blockBelowIndex = blockIndices[index];
    Block& blockBelow = blocks[blockBelowIndex];

	if (blockBelow.ID != topID)
        return false;

    const ushort xMin1 = subVolume.origin.x;
    const ushort xMin2 = blockBelow.subVolume.origin.x;
    const ushort xMax1 = subVolume.origin.x + subVolume.size.x;
    const ushort xMax2 = blockBelow.subVolume.origin.x + blockBelow.subVolume.size.x;

    const ushort zMin1 = subVolume.origin.z;
    const ushort zMin2 = blockBelow.subVolume.origin.z;
    const ushort zMax1 = subVolume.origin.z + subVolume.size.z;
    const ushort zMax2 = blockBelow.subVolume.origin.z + blockBelow.subVolume.size.z;

    const bool alignedXMin = xMin1 == xMin2;
    const bool alignedXMax = xMax1 == xMax2;
    const bool alignedZMin = zMin1 == zMin2;
    const bool alignedZMax = zMax1 == zMax2;

    const int alignedEdges = alignedXMin + alignedXMax + alignedZMin + alignedZMax;

    // perfect match, can merge
    if (alignedEdges == 4)
    {
        mergeUpY(blockBelow, subVolume);
        return true;
    }

    // have too many shelves or nothing below, cannot merge
    if (alignedEdges < 3 || blockBelow.subVolume.origin.y == 0)
        return false;

    uint nextBlockIndex = index - blockBelow.subVolume.size.y * translations.y;

    // single shelf along X, negative direction
    if (xMin1 > xMin2)
    {
        // try to continue merging along Y
        if (shelfCompressY(subVolume, nextBlockIndex, topID))
        {
            // block below becomes shelf to move out of way
            blockBelow.subVolume.size.x -= subVolume.size.x;
            return true;
        }

        // this is the shelf's SubVolume
        SubVolume virtualVolume = blockBelow.subVolume;
        virtualVolume.size.x -= subVolume.size.x;
        unsigned long virtualIndex = blockBelow.index;

        // try to remove shelf
        if (shelfCompressY(virtualVolume, virtualIndex - translations.y, topID) ||
            (blockBelow.subVolume.origin.z != 0 && shelfCompressZ(virtualVolume, virtualIndex - translations.z, topID)))
        {
            // remove shelf from blockBelow
            blockBelow.subVolume.origin.x = subVolume.origin.x;
            blockBelow.subVolume.size.x = subVolume.size.x;
            blockBelow.index += virtualVolume.size.x * translations.x;

            // merge blockBelow upto the start
            mergeUpY(blockBelow, subVolume);

            return true;
        }
    }
    // single shelf along X, positive direction
    else if (xMax1 < xMax2)
    {
        // try to continue merging along Y
        if (shelfCompressY(subVolume, nextBlockIndex, topID))
        {
            // block below becomes shelf to move out of way
            blockBelow.subVolume.origin.x += subVolume.size.x;
            blockBelow.subVolume.size.x -= subVolume.size.x;
            blockBelow.index += subVolume.size.x * translations.x;
            return true;
        }

        // this is the shelf's volume
        SubVolume virtualVolume = blockBelow.subVolume;
        virtualVolume.origin.x += subVolume.size.x;
        virtualVolume.size.x -= subVolume.size.x;
        unsigned long virtualIndex = blockBelow.index + subVolume.size.x * translations.x;

        // try to remove shelf
        if (shelfCompressY(virtualVolume, virtualIndex - translations.y, topID) || 
            (blockBelow.subVolume.origin.z != 0 && shelfCompressZ(virtualVolume, virtualIndex - translations.z, topID)))
        {
            // remove shelf from blockBelow
            blockBelow.subVolume.origin.x = subVolume.origin.x;
            blockBelow.subVolume.size.x = subVolume.size.x;

            // merge blockBelow up to the start
            mergeUpY(blockBelow, subVolume);

            return true;
        }
    }
    // single shelf along Z, positive direction
    else if (zMax1 < zMax2)
    {
        // try to continue merging along Y
        if (shelfCompressY(subVolume, nextBlockIndex, topID))
        {
            // block below becomes shelf to move out of way
            blockBelow.subVolume.origin.z += subVolume.size.z;
            blockBelow.subVolume.size.z -= subVolume.size.z;
            blockBelow.index += subVolume.size.z * translations.z;
            return true;
        }

        // this is the shelf's volume
        SubVolume virtualVolume = blockBelow.subVolume;
        virtualVolume.origin.z += subVolume.size.z;
        virtualVolume.size.z -= subVolume.size.z;
        unsigned long virtualIndex = blockBelow.index + subVolume.size.z * translations.z;

        // try to remove shelf
        // don't merge down Z as the original block is in that direction
        if (shelfCompressY(virtualVolume, virtualIndex - translations.y, topID))
        {
            // remove shelf from blockBelow
            blockBelow.subVolume.origin.z = subVolume.origin.z;
            blockBelow.subVolume.size.z = subVolume.size.z;

            // merge blockBelow up to the start
            mergeUpY(blockBelow, subVolume);

            return true;
        }
    }
    // single shelf along Z, negative direction
    else if (zMin2 < zMin1)
    {
        // try to continue merging along Y
        if (shelfCompressY(subVolume, nextBlockIndex, topID))
        {
            // block below becomes shelf to move out of way
            blockBelow.subVolume.size.z -= subVolume.size.z;
            return true;
        }

        // this is the shelf's volume
        SubVolume virtualVolume = blockBelow.subVolume;
        virtualVolume.size.z -= subVolume.size.z;
        unsigned long virtualIndex = blockBelow.index;

        // try to remove shelf
        if (shelfCompressY(virtualVolume, virtualIndex - translations.y, topID) ||
            (blockBelow.subVolume.origin.z != 0 && shelfCompressZ(virtualVolume, virtualIndex - translations.z, topID)))
        {
            // remove shelf from blockBelow
            blockBelow.subVolume.origin.z = subVolume.origin.z;
            blockBelow.subVolume.size.z = subVolume.size.z;
            blockBelow.index += virtualVolume.size.z * translations.z;

            // merge blockBelow up to the start
            mergeUpY(blockBelow, subVolume);

            return true;
        }
    }

    return false;
}

inline bool ParentBlock::shelfCompressZ(const SubVolume& subVolume, uint index, uchar topID)
{
    const uint blockBelowIndex = blockIndices[index];
    Block& blockBelow = blocks[blockBelowIndex];

    if (blockBelow.ID != topID)
        return false;

    const ushort xMin1 = subVolume.origin.x;
    const ushort xMin2 = blockBelow.subVolume.origin.x;
    const ushort xMax1 = subVolume.origin.x + subVolume.size.x;
    const ushort xMax2 = blockBelow.subVolume.origin.x + blockBelow.subVolume.size.x;

    const ushort yMin1 = subVolume.origin.y;
    const ushort yMin2 = blockBelow.subVolume.origin.y;
    const ushort yMax1 = subVolume.origin.y + subVolume.size.y;
    const ushort yMax2 = blockBelow.subVolume.origin.y + blockBelow.subVolume.size.y;

    const bool alignedXMin = xMin1 == xMin2;
    const bool alignedXMax = xMax1 == xMax2;
    const bool alignedYMin = yMin1 == yMin2;
    const bool alignedYMax = yMax1 == yMax2;

    const int alignedEdges = alignedXMin + alignedXMax + alignedYMin + alignedYMax;

    // perfect match, can merge
    if (alignedEdges == 4)
    {
        mergeUpZ(blockBelow, subVolume);

        return true;
    }

    // have too many shelves, cannot merge
    if (alignedEdges < 3 || blockBelow.subVolume.origin.z == 0)
        return false;

    uint nextBlockIndex = index - blockBelow.subVolume.size.z * translations.z;

    // single shelf along X, negative direction
    if (xMin2 < xMin1)
    {
        // try to remove above subvolume
        if (shelfCompressZ(subVolume, nextBlockIndex, topID))
        {
            // move block out of the way to the left
            blockBelow.subVolume.size.x -= subVolume.size.x;
            return true;
        }

        // this is the shelf's volume
        SubVolume virtualVolume = blockBelow.subVolume;
        virtualVolume.size.x -= subVolume.size.x;
        unsigned long virtualIndex = blockBelow.index;

        // try to remove shelf
        if ((blockBelow.subVolume.origin.y != 0 && shelfCompressY(virtualVolume, virtualIndex - translations.y, topID)) ||
            shelfCompressZ(virtualVolume, virtualIndex - translations.z, topID))
        {
            // resize blockBelow by removing the virtual block
            blockBelow.subVolume.origin.x = subVolume.origin.x;
            blockBelow.subVolume.size.x = subVolume.size.x;
            blockBelow.index += virtualVolume.size.x * translations.x;

            // merge upto the start
            mergeUpZ(blockBelow, subVolume);

            return true;
        }
    }
    // single shelf along X, positive direction
    else if (xMax1 < xMax2)
    {
        // try to remove above subvolume
        if (shelfCompressZ(subVolume, nextBlockIndex, topID))
        {
            // move block out of the way to the right
            blockBelow.subVolume.origin.x += subVolume.size.x;
            blockBelow.subVolume.size.x -= subVolume.size.x;
            blockBelow.index += subVolume.size.x * translations.x;
            return true;
        }

        // this is the shelf's volume
        SubVolume virtualVolume = blockBelow.subVolume;
        virtualVolume.origin.x += subVolume.size.x;
        virtualVolume.size.x -= subVolume.size.x;
        unsigned long virtualIndex = blockBelow.index + subVolume.size.x * translations.x;

        // try to remove shelf
        if ((blockBelow.subVolume.origin.y != 0 && shelfCompressY(virtualVolume, virtualIndex - translations.y, topID)) ||
            shelfCompressZ(virtualVolume, virtualIndex - translations.z, topID))
        {
            // resize blockBelow by removing the virtual block
            blockBelow.subVolume.origin.x = subVolume.origin.x;
            blockBelow.subVolume.size.x = subVolume.size.x;

            // merge upto the start
            mergeUpZ(blockBelow, subVolume);

            return true;
        }
    }
    // single shelf along Y, positive direction
    else if (yMax1 < yMax2)
    {
        // try to remove above subvolume
        if (shelfCompressZ(subVolume, nextBlockIndex, topID))
        {
            // move block out of the way up
            blockBelow.subVolume.origin.y += subVolume.size.y;
            blockBelow.subVolume.size.y -= subVolume.size.y;
            blockBelow.index += subVolume.size.y * translations.y;
            return true;
        }

        // this is the shelf's volume
        SubVolume virtualVolume = blockBelow.subVolume;
        virtualVolume.origin.y += subVolume.size.y;
        virtualVolume.size.y -= subVolume.size.y;
        unsigned long virtualIndex = blockBelow.index + subVolume.size.y * translations.y;

        // try to remove shelf
        // cant merge down Y as that would hit the original block
        if (shelfCompressZ(virtualVolume, virtualIndex - translations.z, topID))
        {
            // resize blockBelow by removing the virtual block
            blockBelow.subVolume.origin.y = subVolume.origin.y;
            blockBelow.subVolume.size.y = subVolume.size.y;

            // merge upto the start
            mergeUpZ(blockBelow, subVolume);

            return true;
        }
    }
    // single shelf along Y, negative direction
    else if (yMin2 < yMin1)
    {
        // try to remove above subvolume
        if (shelfCompressZ(subVolume, nextBlockIndex, topID))
        {
            // move block out of the way down
            blockBelow.subVolume.size.y -= subVolume.size.y;
            return true;
        }

        // this is the shelf's volume
        SubVolume virtualVolume = blockBelow.subVolume;
        virtualVolume.size.y -= subVolume.size.y;
        unsigned long virtualIndex = blockBelow.index;

        // try to remove shelf
        if ((blockBelow.subVolume.origin.y != 0 && shelfCompressY(virtualVolume, virtualIndex - translations.y, topID)) ||
            shelfCompressZ(virtualVolume, virtualIndex - translations.z, topID))
        {
            // resize blockBelow by removing the virtual block
            blockBelow.subVolume.origin.y = subVolume.origin.y;
            blockBelow.subVolume.size.y = subVolume.size.y;
            blockBelow.index += virtualVolume.size.y * translations.y;

            // merge upto the start
            mergeUpZ(blockBelow, subVolume);

            return true;
        }
    }

    return false;
}

inline void ParentBlock::shelfY()
{
    for (Block& block : blocks)
    {
        if (!block.isValid)
            continue;

        // try and shelf merge down Y
        if (block.subVolume.origin.y != 0
            && shelfCompressY(block.subVolume, block.index - translations.y, block.ID))
        {
            block.isValid = false;
            continue;
        }
    }
}

inline void ParentBlock::shelfZ()
{
    for (Block& block : blocks)
    {
        if (!block.isValid)
            continue;

        // try and shelf merge down Z
        if (block.subVolume.origin.z != 0
            && shelfCompressZ(block.subVolume, block.index - translations.z, block.ID))
        {
            block.isValid = false;
            continue;
        }
    }
}

inline void ParentBlock::shelfCompress()
{
    for (Block& block : blocks)
    {
        if (!block.isValid)
            continue;

        // try and shelf merge down Y
        if (block.subVolume.origin.y != 0 
            && shelfCompressY(block.subVolume, block.index - translations.y, block.ID))
        {
            block.isValid = false;
            continue;
        }

        // try and shelf merge down Z
        if (block.subVolume.origin.z != 0
            && shelfCompressZ(block.subVolume, block.index - translations.z, block.ID))
        {
            block.isValid = false;
            continue;
        }
    }
}

inline void ParentBlock::greedyCompressY()
{
    for (Block& block : blocks)
    {
        // cannot merge an invalid block or one at the bottom
        if (!block.isValid || block.subVolume.origin.y == 0)
            continue;

        // using the index volume, find the block which is below on Y axis
        uint blockBelowIndex = blockIndices[block.index - translations.y];

        if (blockBelowIndex == NULL_INDEX)
            continue;

        Block& blockBelow = blocks[blockBelowIndex];

        if (block.ID == blockBelow.ID
            && block.subVolume.origin.x == blockBelow.subVolume.origin.x
            && block.subVolume.size.x == blockBelow.subVolume.size.x
            // not necessary as Y compression done before Z
            //&& block.subVolume.origin.z == blockBelow.subVolume.origin.z
            //&& block.subVolume.size.z == blockBelow.subVolume.size.z
            )
        {
            // disable top block
            block.isValid = false;

            // block volume now points to block below
            blockIndices[block.index] = blockBelowIndex;

            // grow bottom block to contain top block
            // assume the top block is y size 1
            blockBelow.subVolume.size.y += 1;
        }
    }
}

inline void ParentBlock::greedyCompressZ()
{
    for (Block& block : blocks)
    {
        // cannot merge an invalid block or one at the bottom
        if (!block.isValid || block.subVolume.origin.z == 0)
            continue;

        // use pointer volume to find next block along Y
        uint blockBelowIndex = blockIndices[block.index - translations.z];

        if (blockBelowIndex == NULL_INDEX)
            continue;

        Block& blockBelow = blocks[blockBelowIndex];

        // check if they align exactly along X and Y
        if (block.ID == blockBelow.ID
            && block.subVolume.origin.x == blockBelow.subVolume.origin.x
            && block.subVolume.size.x == blockBelow.subVolume.size.x
            && block.subVolume.origin.y == blockBelow.subVolume.origin.y
            && block.subVolume.size.y == blockBelow.subVolume.size.y)
        {
            // disable top block
            block.isValid = false;

            // block volume now points to block below
            blockIndices[block.index] = blockBelowIndex;

            // grow bottom block to contain top block
            // assume the top block is z size 1
            blockBelow.subVolume.size.z += 1;
        }
    }
}

// compress and print parent block
void ParentBlock::compressPrint()
{
    if (allSameTag())
    {
        printWholeParentBlock();
        return;
    }

    // do greedy search to eliminate most blocks quickly
    greedyCompressY();
    greedyCompressZ();
    

    // more complex shelf compression needs index volume to be correct
    refreshBlockIndices();
    //shelfCompress();
    shelfY();
    shelfZ();

    // output this block plane
    printBlocks();
}

// for debugging
// print out each Block as a single block
inline void ParentBlock::printBlocks()
{
    for (auto& block : blocks)
    {
        if (!block.isValid)
            continue;

        cout << (block.subVolume.origin + originWS).to_string() + "," + block.subVolume.size.to_string() + ",'" + tt->
	        getTag(block.ID) + "'\n";
    }
}

inline void ParentBlock::printWholeParentBlock()
{
    cout << originWS.to_string() + "," + pBlockDim.to_string() + ",'" + tt->getTag(blocks[0].ID) + "'\n";
}

inline bool ParentBlock::allSameTag()
{
    const uchar firstID = blocks[0].ID;

    for (size_t i = 1; i <= blocks.size(); i++)
    {
        if (blocks[i].ID != firstID)
            return false;
    }

    return true;
}

// update variables to be ready for reading next block plane
void ParentBlock::reset(int numActivePlanes)
{
    // move 1 parent block size up Z
    originWS.z += pBlockDim.z * numActivePlanes;

    blocks.clear();

    currentIndex = 0;
}

void ParentBlock::insertBlockLine(vec3<ushort> origin, ushort length, uchar ID)
{
    // Index the block will be stored at
    uint blockIndex = (uint)blocks.size();

    // store an n*1*1 line at origin of the found length
    blocks.push_back({ true, { origin, { length, 1, 1 } }, ID, currentIndex });

    blockIndices[currentIndex] = blockIndex;
    currentIndex++;

    // set voxels in 3D volume to point at the index of the block it represents
    for (int i=1; i<length; i++)
    {
        blockIndices[currentIndex] = NULL_INDEX;
        currentIndex++;
    }
}
