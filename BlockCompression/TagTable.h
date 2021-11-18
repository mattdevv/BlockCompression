//  TagTable.hpp
//  BlockCompression

#pragma once

#include <string>
#include <map>
#include <iostream>
#include <vector>
#include "uDataTypes.h"

using namespace std;

class TagTable
{
private:
    map<string, uchar> tags;
    vector<string> names;
    int numTags;
    
public:
    TagTable();
    string getTag(uchar id);
    string* getTagPointer(uchar id);
    uchar getID(string tag);
    int getTotalTags() const;
    void reset();
};