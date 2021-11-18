#include "TagTable.h"

TagTable::TagTable()
{
    id = 0;
    names.reserve(maxTagCount);
}

// Return the tag from an id
string TagTable::getTag(uchar _id)
{
    return names[_id];
}

// Return the tag from an id
string* TagTable::getTagPointer(uchar _id)
{
    return &names[_id];
}

// Return the id if it is in the map
// Otherwise add to both maps
uchar TagTable::getID(string tag)
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

int TagTable::getTotalTags()
{
    return id;
}

void TagTable::reset()
{
    id = 0;

    tags.clear();
    names.clear();
}