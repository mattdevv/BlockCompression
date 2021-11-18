#pragma once

#include <iostream>
#include <string>
#include <vector>
#include "vec3.h"
#include "TagTable.h"
#include "uDataTypes.h"

using namespace std;

const uint NULL_INDEX = 0xFFFFFFFF;

// enum to easily reference an axis
enum class Axis { x, y, z };

// holds definition of subvolume for job
// origin is local to the parent block
struct SubVolume
{
	vec3<ushort> origin;
	vec3<ushort> size;
};

struct Block
{
	bool isValid;
	SubVolume subVolume;
	uchar ID;
	unsigned long index;
};

class ParentBlock
{
private:
	static vec3<ulong> translations;		// offsets to move through 1D array in 3D 
	static TagTable* tt;							// access to global tag IDs/names
	static vec3<ushort> pBlockDim;					// number of voxels per dimension in a parent block
	uint currentIndex;								// next empty index to read voxels into
	vec3<ushort> originWS{};						// offset from global origin to local origin
	vector<Block> blocks;
	vector<uint> blockIndices;

	static uint convert3DIndexTo1D(const vec3<ushort>& position);
	void fillSubVolume(uint newValue, const SubVolume& subVolume);
	void refreshBlockIndices();
	void mergeUpY(Block& block, const SubVolume& subVolume);
	void mergeUpZ(Block& block, const SubVolume& subVolume);
	bool shelfCompressY(const SubVolume& subVolume, uint index, uchar topID);
	bool shelfCompressZ(const SubVolume& subVolume, uint index, uchar topID);
	void shelfY();
	void shelfZ();
	void shelfCompress();
	void greedyCompressY();
	void greedyCompressZ();
	void printBlocks();
	void printWholeParentBlock();
	bool allSameTag();

public:
	ParentBlock(vec3<ushort> _originWS);
	static void setup(vec3<ushort> dimensions, TagTable* planeTagTable);
	void compressPrint();
	void reset(int numActivePlanes);
	void insertBlockLine(vec3<ushort> origin, ushort length, uchar ID);
};