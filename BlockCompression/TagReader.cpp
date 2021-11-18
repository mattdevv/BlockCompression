#include "TagReader.h"

//#define DEBUG

#ifdef DEBUG
FILE* pFile;
#endif

#define SKIP_AMOUNT 14

char TagReader::charBuffer[MAX_LINE_LENGTH];
char* TagReader::bufferEnd;

// fill initial buffer and return description line
string TagReader::setup()
{
    // fill initial buffer
#ifdef DEBUG
    fopen_s(&pFile, "D:/Documents/UNI/2021/Semester 2/Software Engineering Project/runner/the_stratal_one_42000000_14x10x12.csv", "r");
    fread(charBuffer, sizeof(char), MAX_LINE_LENGTH, pFile);
#else
    fread(charBuffer, sizeof(char), MAX_LINE_LENGTH, stdin);
#endif

    // set end point
    bufferEnd = charBuffer + MAX_LINE_LENGTH;

    // find end of first line which contains volume description
    char* endLine = charBuffer + 1;
    while (*endLine != '\n')
    {
        endLine++;
    }

    // Save description line into a string
    string description = string(charBuffer, endLine);

    // Replace comma characters with whitespace
    for (auto& c : description)
    {
        if (c == ',')
            c = ' ';
    }

    return description;
}

// get next tag name
// looks through buffer finding tags
// if hits buffer ends, reads more chars
string TagReader::getNextTagName()
{
    bool reading = false;
    char* begin_str = nullptr;

    bool usedCache = false;
    string cachedString;

    static char* iter = charBuffer;

    // continuously read into buffer
    // loop is broken by finishing a tag
    while (true)
    {
        // while not hit buffer end
        while (iter < bufferEnd)
        {
            // found start/finish of tag
            if (*iter == '\'')
            {
                // found finish
                if (reading)
                {
                    reading = false;

                    // move over minimum distance of next tag position/size
                    iter += SKIP_AMOUNT;

                    // concatenate cached start to tag name if needed
                    if (usedCache)
                    {
                        usedCache = false;
                        return cachedString + string(begin_str, iter - SKIP_AMOUNT);
                    }

                    // extract tag between start and end point
                    return string(begin_str, iter - SKIP_AMOUNT);
                }
                // found start
                else
                {
                    reading = true;

                    // save where tag starts
                    begin_str = iter + 1;
                }
            }
            
            iter++;
        }

        // finished buffer, but didnt not finish reading tag, cache
        if (reading)
        {
            usedCache = true;
            cachedString = string(begin_str, iter);
            begin_str = charBuffer;
        }

        iter -= MAX_LINE_LENGTH;
#ifdef DEBUG
        fread(charBuffer, sizeof(char), MAX_LINE_LENGTH, pFile);
#else
        fread(charBuffer, sizeof(char), MAX_LINE_LENGTH, stdin);
#endif
    }

    // never hit
    return "";
}