#include "ParentBlock.h"

vec3<ushort> ParentBlock::pBlockDim = { 1, 1, 1 };
vec3<unsigned long> ParentBlock::translations = { 1, 1, 1 };
TagTable<string, uchar>* ParentBlock::tt = nullptr;

// sum elementwise a vector of vectors between two iterators
vector<int> sumVectors(vector<vector<int>>::iterator start, vector<vector<int>>::iterator end)
{
    size_t vecSize = (start->size());

    vector<int> totals = vector<int>(vecSize);

    while (start < end)
    {
        for (size_t i = 0; i < vecSize; i++)
        {
            totals[i] += start->at(i);
        }

        start++;
    }

    return totals;
}

// directly subtract from a vector of same size
void subFromVector(vector<int>& a, vector<int>& b)
{
    for (size_t i = 0; i < b.size(); i++)
    {
        a[i] -= b[i];
    }
}

// directly add to a vector of same size
void addToVector(vector<int>& a, vector<int>& b)
{
    for (size_t i = 0; i < b.size(); i++)
    {
        a[i] += b[i];
    }
}

// total sum of individual elements in a vector
int vectorTotal(vector<int>& vec)
{
    int total = 0;

    for (auto e : vec)
    {
        total += e;
    }

    return total;
}

// find information content of a vector (how homogeneous the values are)
// 0 = all the same, 1 = 50:50 split
inline float informationContent(vector<int>& counters)
{
    int total = vectorTotal(counters);
    float invTotalVoxels = 1.0f / (float)total;

    float totalInfo = 0.0f;

    for (auto& tagCount : counters)
    {
        // finding log2f(0) is undefined, skip this
        if (tagCount == 0)
            continue;
        
        // return early if all tags are the same
        if (tagCount == total)
            return 0.0f;

        // use proportion to keep sum of all log2()'s between 0-1
        float proportionOfTotal = (float)tagCount * invTotalVoxels;
        totalInfo -= proportionOfTotal * log2f(proportionOfTotal);
    }

    return totalInfo;
}

// static method for setup of ParentBlock class
// store information that will be constant for every parent block
// must be called before creating a ParentBlock instance
void ParentBlock::setup(vec3<ushort> dimensions, TagTable<string, uchar>* _tt)
{
    // dimension of parent block
    pBlockDim = dimensions;

    // number of 1D voxels needed to move for each dimension of 3D movement
    translations = { 1, dimensions.x, (unsigned long)(dimensions.x * dimensions.y) };

    // tag table used for lookups
    tt = _tt;
}

// convert a global ID, to a local ID within parent block
uchar ParentBlock::getLocalID(uchar globalID)
{
    // Set the next id
    map<uchar, unsigned char>::iterator lookup = localIDTable.find(globalID);

    // 'tag' is in the map, return from map
    if (lookup != localIDTable.end()) return lookup->second;

    // Else add to maps and save value, return nextID
    else
    {
        uchar nextID = localIDCounter;

        // Add to the maps
        localIDTable[globalID] = nextID;
        tagNames.push_back(tt->getTagPointer(globalID));

        // Increment id for next tag
        localIDCounter++;

        return nextID;
    }
}

ParentBlock::ParentBlock(vec3<ushort> _origin)
{
    // create a 1D array to hold the contents of a 3D volume
	voxels.resize(pBlockDim.volume());

    // set parent block's voxel offset from total volume origin
    originWS = _origin;

    // setup inital values
    localIDCounter = 0;
    arrayIndex = 0;
}

// uses precalcuated translation offsets for fast conversion
inline unsigned long ParentBlock::convert3DIndexTo1D(vec3<ushort>& position)
{
    return position.x + position.y * translations.y + position.z * translations.z;
}

// compress and print parent block
void ParentBlock::compressPrint()
{
    //cout << originWS.to_string() << endl;
    // if only 1 local tag, parent-block is homogeneous and can be printed early
    if (tagNames.size() == 1)
    {
        cout << (originWS).to_string() + "," + pBlockDim.to_string() + ",'" + *tagNames[0] + "'\n";
        return;
    }

    // first job is entire parent block
    jobStack.push({{ 0, 0, 0 }, pBlockDim});

    // start processing
    KDTreePrint();
}

// Using a KDTree algorithm, split volumes into 2 sub-volumes
// until minimum block-size or 
void ParentBlock::KDTreePrint()
{
    // while loop takes jobs off stack and adds upto 2 new ones
    while (!jobStack.empty())
    {
        // load next job information
        SubVolume subVolume = GET_NEXT_JOB(jobStack);
        vec3<ushort>& origin = subVolume.origin;
        vec3<ushort>& size = subVolume.size;

        // find which split gives best tag separation
        // split value between 1->dimensionSize-1 (inclusive)
        // a split at 1 means between index 0 and 1
        SplitResult splitResult = ChooseSplit(origin, size);
        Split& split = splitResult.split;

        // origin and size of new sub-volumes
        // can reuse origin as origin1
        vec3<ushort> origin2 = origin;
        vec3<ushort> newSize1 = size;
        vec3<ushort> newSize2 = size;

        // calculate new origin and sizes
        if (split.axis == Axis::x)
        {
            origin2.x += split.point;
            newSize1.x = split.point;
            newSize2.x = size.x - (split.point);
        }
        else if (split.axis == Axis::y)
        {
            origin2.y += split.point;
            newSize1.y = split.point;
            newSize2.y = size.y - (split.point);
        }
        else if (split.axis == Axis::z)
        {
            origin2.z += split.point;
            newSize1.z = split.point;
            newSize2.z = size.z - (split.point);
        }

        // decide what to do with right side of split
        if (splitResult.printRight)
        {
            // sub volume is homogenous, print
            unsigned long index = convert3DIndexTo1D(origin2);
            cout << (origin2 + originWS).to_string() + "," + newSize2.to_string() + ",'" + *tagNames[voxels[index]] + "'\n";
        }
        else if (newSize2.x == 1 && newSize2.y == 1 && newSize2.z == 1)
        {
            // sub volume cannot be cut any further, print
            unsigned long index = convert3DIndexTo1D(origin2);
            cout << (origin2 + originWS).to_string() + ",1,1,1,'" + *tagNames[voxels[index]] + "'\n";
        }
        else
        {
            // add new job
            jobStack.push({ origin2, newSize2 });
        }

        // decide what to do with left side of split
        if (splitResult.printLeft)
        {
            // sub volume is homogenous, print
            unsigned long index = convert3DIndexTo1D(origin);
            cout << (origin + originWS).to_string() + "," + newSize1.to_string() + ",'" + *tagNames[voxels[index]] + "'\n";
        }
        else if (newSize1.x == 1 && newSize1.y == 1 && newSize1.z == 1)
        {
            // sub volume cannot be split any further, print
            unsigned long index = convert3DIndexTo1D(origin);
            cout << (origin + originWS).to_string() + ",1,1,1,'" + *tagNames[voxels[index]] + "'\n";
        }
        else
        {
            // add new job
            jobStack.push({ origin, newSize1 });
        }
    }
}

// create a counter of how many times each tag appears in each slice 
inline void ParentBlock::buildSliceTallys(vector<vector<int>>& slicesYZ, vector<vector<int>>& slicesXZ, vector<vector<int>>& slicesXY, vec3<ushort>& origin, vec3<ushort>& size)
{
    // 1D position of sub-volume origin within parent-block
    unsigned long startIndex = convert3DIndexTo1D(origin);

    // offset being used to read 1D data from
    // doing it this way is more efficient
    unsigned long lookupIndex = startIndex;

    // read every voxel in this sub-volume
    // add voxel's tag to slice counters (1 per
    for (int z = 0; z < size.z; z++)
    {
        lookupIndex = startIndex;
        for (int y = 0; y < size.y; y++)
        {
            for (int x = 0; x < size.x; x++)
            {
                // read tag at this position
                uchar tagID = voxels[lookupIndex + x];

                // increament this tag's counter for each dimension
                slicesYZ[x][tagID]++;
                slicesXZ[y][tagID]++;
                slicesXY[z][tagID]++;
            }
            lookupIndex += translations.y;
        }
        startIndex += translations.z;
    }
}

// check all posible splits and return best according to information gain
SplitResult ParentBlock::ChooseSplit(vec3<ushort>& origin, vec3<ushort>& size)
{
    // default split will always be overidden 
    Split bestSplit = { Axis::x, 1 };
    float bestGain = -1.0f;

    bool leftSame = false;
    bool rightSame = false;

    // cache information content for frequent use
    float totalInfoContent;

    // create an empty tally of how many times each tag appears
    vector<int> empty(tagNames.size(), 0);

    // create a tally for each slice along an axis
    // i.e. slicesXY[2][5] is the number of times tagID 5 along slice z=2
    vector<vector<int>> slicesX(size.x, empty);
    vector<vector<int>> slicesY(size.y, empty);
    vector<vector<int>> slicesZ(size.z, empty);

    buildSliceTallys(slicesX, slicesY, slicesZ, origin, size);

    // flatten slicesX into a single tally
    vector<int> left = empty;
    vector<int> right = sumVectors(slicesX.begin(), slicesX.end());

    // cache totalInfoContent of entire subvolume
    totalInfoContent = informationContent(right);

    // move slices from right to left, testing the effect on gain
    if (size.x > 1)
    {
        GainLR p_gainLR; 
        bool found = false; 
        for (ushort i = 0; i < size.x - 1; i++)
        {
            addToVector(left, slicesX[i]);
            subFromVector(right, slicesX[i]);
            
            GainLR gainLR = findGain(left, right, totalInfoContent); 
            float gain = gainLR.gain; 
            
            if (gainLR.leftSame)
            {
                p_gainLR = gainLR; 
                found = true; 
            }
            else
            {
                if (found)
                {
                    return { { Axis::x, (ushort)(i) }, p_gainLR.leftSame, p_gainLR.rightSame };
                }
            }
            if (gainLR.rightSame)
            {
                return { { Axis::x, (ushort)(i + 1) }, gainLR.leftSame, gainLR.rightSame };
            }
                    
            if (gain > bestGain)
            {
                bestGain = gain; 
                bestSplit = { Axis::x, (ushort)(i + 1) };
                        
                leftSame = gainLR.leftSame; 
                rightSame = gainLR.rightSame; 
            }
        }
        if (found)
        {
            return { { Axis::x, (ushort)(size.x - 1) }, true, false };
        }
    }

    // flatten slicesY into a single tally
    left = empty;
    right = sumVectors(slicesY.begin(), slicesY.end());

    // move slices from right to left, testing the effect on gain
    if (size.y > 1)
    {
        GainLR p_gainLR;
        bool found = false;
        for (ushort i = 0; i < size.y - 1; i++)
        {
            addToVector(left, slicesY[i]);
            subFromVector(right, slicesY[i]);

            GainLR gainLR = findGain(left, right, totalInfoContent);
            float gain = gainLR.gain;

            if (gainLR.leftSame)
            {
                p_gainLR = gainLR;
                found = true;
            }
            else
            {
                if (found)
                {
                    return { { Axis::y, (ushort)(i) }, p_gainLR.leftSame, p_gainLR.rightSame };
                }
            }
            if (gainLR.rightSame)
            {
                return { { Axis::y, (ushort)(i + 1) }, gainLR.leftSame, gainLR.rightSame };
            }

            if (gain > bestGain)
            {
                bestGain = gain;
                bestSplit = { Axis::y, (ushort)(i + 1) };

                leftSame = gainLR.leftSame;
                rightSame = gainLR.rightSame;
            }
        }
        if (found)
        {
            return { { Axis::y, (ushort)(size.y - 1) }, true, false };
        }
    }

    // flatten slicesZ into a single tally
    left = empty;
    right = sumVectors(slicesZ.begin(), slicesZ.end());

    // move slices from right to left, testing the effect on gain
    if (size.z > 1)
    {
        GainLR p_gainLR;
        bool found = false;
        for (ushort i = 0; i < size.z - 1; i++)
        {
            addToVector(left, slicesZ[i]);
            subFromVector(right, slicesZ[i]);

            GainLR gainLR = findGain(left, right, totalInfoContent);
            float gain = gainLR.gain;

            if (gainLR.leftSame)
            {
                p_gainLR = gainLR;
                found = true;
            }
            else
            {
                if (found)
                {
                    return { { Axis::z, (ushort)(i) }, p_gainLR.leftSame, p_gainLR.rightSame };
                }
            }
            if (gainLR.rightSame)
            {
                return { { Axis::z, (ushort)(i + 1) }, gainLR.leftSame, gainLR.rightSame };
            }

            if (gain > bestGain)
            {
                bestGain = gain;
                bestSplit = { Axis::z, (ushort)(i + 1) };

                leftSame = gainLR.leftSame;
                rightSame = gainLR.rightSame;
            }
        }
        if (found)
        {
            return { { Axis::z, (ushort)(size.z - 1) }, true, false };
        }
    }


    return { bestSplit, leftSame, rightSame };
}

// for debugging
// print out each voxel as a single block
void ParentBlock::printRaw()
{
    unsigned long index = 0;
    for (ushort z = 0; z < pBlockDim.z; z++)
    {
        for (ushort y = 0; y < pBlockDim.y; y++)
        {
            for (ushort x = 0; x < pBlockDim.x; x++)
            {
                vec3<ushort> cur3Dindex = { x, y, z };

                cout << (cur3Dindex + originWS).to_string() + ",1,1,1,'" + *tagNames[voxels[index]] + "'\n";

                index++;
            }
        }
    }
}

// find the difference made to the information content when the whole volume is split into 2 sub-volumes
// must be passed sub-volumes which have already been combined into single counter
inline GainLR ParentBlock::findGain(vector<int>& left, vector<int>& right, float totalInfo)
{
    // how many voxels (tags) are in left/right sub-volume
    int volumeLeft = vectorTotal(left);
    int volumeRight = vectorTotal(right);
    
    // precalculate 1/totalVolume
    float invTotalVolume = 1.0f / (float)(volumeLeft + volumeRight);

    // find proportion of tags which are left/right of split
    float pLower = (float)volumeLeft * invTotalVolume;
    float pHigher = (float)volumeRight * invTotalVolume;

    // find inforation content (homogeneousness) of left/right sub-volumes
    // 0 = all tags are the same
    // 1 = split 50:50
    float infoLower = informationContent(left);
    float infoHigher = informationContent(right);

    // add left/right information content as a proportion of their volume
    float combinedInfo = pLower * infoLower + pHigher * infoHigher;

    // we want the combined information content of the sub-volumes to be less than when they were a single volume
    // this would mean they are closer to all tags being the same (homogeneous)
    // the difference is how much information we have gained (removed by splitting)
    float gain = totalInfo - combinedInfo;

    //return { gain, false, false };
    return { gain, infoLower == 0.0f, infoHigher == 0.0f };
}

// update variables to be ready foring reading next block plane
void ParentBlock::reset(int numActivePlanes)
{
    // move 1 parent block size up Z
    originWS.z += pBlockDim.z * numActivePlanes;

    // reset input counter
    arrayIndex = 0;

    // reset local tags
    localIDCounter = 0;
    tagNames.clear();
    localIDTable.clear();
}

// let parentblock decide where to insert voxel
void ParentBlock::insertVoxel(uchar tag)
{
    // get/insert tag in local table
    uchar id = getLocalID(tag);

    // store local ID in 1D vector
    voxels[arrayIndex] = id;
    arrayIndex++;
}
