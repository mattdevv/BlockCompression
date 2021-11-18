#pragma once

#include <vector>
#include <string>
#include <iostream>
#include <math.h>
#include <stack>
#include "vec3.h"
#include "TagTable.h"

using namespace std;

#define uchar unsigned char
#define GET_NEXT_JOB(stack) stack.top(); stack.pop();

// enum to easily reference an axis
enum class Axis { x, y, z };

// holds definition of subvolume for job
// origin is local to the parent block
struct SubVolume
{
	vec3<ushort> origin;
	vec3<ushort> size;
};

// an axis and a value that seperates data
struct Split
{
	Axis axis;
	ushort point;
};

// current gain and whether data was homogeneous
struct GainLR
{
	float gain;
	bool leftSame;
	bool rightSame;
};

// best split and whether sub-volume can be printed as it is homogeneous
struct SplitResult
{
	Split split;
	bool printLeft;
	bool printRight;
};

class ParentBlock
{
private:
	static vec3<unsigned long> translations;		// offsets to move through 1D array in 3D 
	static TagTable<string, uchar>* tt;				// access to global tag IDs/names
	static vec3<ushort> pBlockDim;					// number of voxels per dimension in a parent block

	vector<uchar> voxels;							// voxels contained in this block
	vec3<ushort> originWS;							// offset from global origin to local origin
	unsigned long arrayIndex;						// index to store next voxel in voxels 

	int localIDCounter;								// count of tags in this pBlock
	map<uchar, uchar> localIDTable;					// map between global and local tagID's
	vector<string*> tagNames;						// string name which local tagID refers to
	uchar getLocalID(uchar globalID);				// convert global ID to local ID

	stack<SubVolume> jobStack;						// list of jobs needed to complete KDTreePrint()

	unsigned long convert3DIndexTo1D(vec3<ushort>& position);
	void KDTreePrint();
	void buildSliceTallys(vector<vector<int>>& slicesYZ, vector<vector<int>>& slicesXZ, vector<vector<int>>& slicesXY, vec3<ushort>& origin, vec3<ushort>& size);
	SplitResult ChooseSplit(vec3<ushort>& origin, vec3<ushort>& size);
	GainLR findGain(vector<int>& left, vector<int>& right, float totalInfo);
	

public:
	ParentBlock(vec3<ushort> dimensions);
	static void setup(vec3<ushort> dimensions, TagTable<string, uchar>* tt);
	void compressPrint();
	void printRaw();
	void reset(int numActivePlanes);
	void insertVoxel(uchar tag);
};