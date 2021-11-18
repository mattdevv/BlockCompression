//  TagTable.hpp
//  BlockCompression

#pragma once

#include <string>
#include <map>
#include <iostream>
#include <vector>

using namespace std;

#define uchar unsigned char

template<class t, class u>
class TagTable
{
private:
    // Translate string to id
    map<t, u> tags;
    
    // Translate id to string
    vector<t> names;
    
    // ID
    int id;
    
public:
    TagTable();
    t getTag(u _id);
    t* getTagPointer(u _id);
    u getID(t tag);
    int getTotalTags();
    void reset();
};

template<class t, class u>
TagTable<t, u>::TagTable()
{
    id = 0;
    names.reserve(256);
}

// Return the tag from an id
template<class t, class u>
t TagTable<t, u>::getTag(u _id)
{
    return names[_id];
}

// Return the tag from an id
template<class t, class u>
t* TagTable<t, u>::getTagPointer(u _id)
{
    return &names[_id];
}

// Return the id if it is in the map
// Otherwise add to both maps
template<class t, class u>
u TagTable<t, u>::getID(t tag)
{

    // Set the next id
    map<string, unsigned char>::iterator lookup = tags.find(tag);

    // 'tag' is in the map, return from map
    if (lookup != tags.end()) return lookup->second;

    // Else add to both maps, return nextID
    else
    {
        // Get an id
        unsigned char nextID = id;

        // Add to the maps
        tags[tag] = nextID;
        names.push_back(tag);

        // Increment id for next tag
        id++;

        return nextID;
    }
}

template<class t, class u>
int TagTable<t, u>::getTotalTags()
{
    return id;
}

template<class t, class u>
void TagTable<t, u>::reset()
{
    id = 0;

    tags.clear();
    names.clear();
}


