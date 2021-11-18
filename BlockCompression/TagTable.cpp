#include "TagTable.h"

TagTable::TagTable()
{
    numTags = 0;
    constexpr int numUniqueCharValues = 256;
    names.reserve(numUniqueCharValues);
}

// Return the tag from an id
string TagTable::getTag(const uchar id)
{
    return names[id];
}

// Return the tag from an id
string* TagTable::getTagPointer(const uchar id)
{
    return &names[id];
}

// Return the id if it is in the map
// Otherwise insert to both maps
uchar TagTable::getID(string tag)
{
    // Set the next id
    map<string, uchar>::iterator lookup = tags.find(tag);

    // 'tag' is in the map, return stored ID
    if (lookup != tags.end())
    {
        return lookup->second;
    }
    // else add to map and vector, return the new ID
    else
    {
        // Get a new id
        uchar nextID = numTags;

        // Add to the maps
        tags[tag] = nextID;
        names.push_back(tag);

        // Increment id for next tag
        numTags++;

        return nextID;
    }
}

// public getter for number of held tags
int TagTable::getTotalTags() const
{
    return numTags;
}

// clear contents of a TagTable
void TagTable::reset()
{
    numTags = 0;

    tags.clear();
    names.clear();
}