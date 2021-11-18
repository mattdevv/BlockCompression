//  TagTable.hpp
//  BlockCompression

#pragma once

#include <string>
#include <map>
#include <iostream>
#include <vector>
#include "cuda_runtime.h"
#include "Helpers.h"
#include "DataTypes.h"

using namespace std;

const ushort maxTagCount = 256;

// #define uchar unsigned char

class TagTable
{
private:
    // Translate string to id
    map<string, uchar> tags;

    // Translate id to string
    vector<string> names;

    // ID
    int id;

public:
    TagTable();
    string getTag(uchar _id);
    string* getTagPointer(uchar _id);
    uchar getID(string tag);
    int getTotalTags();
    void reset();
};