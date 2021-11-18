#pragma once

#include <iostream>
#include <string>
#include <cstdio>

// how many chars will be read at once
#define MAX_LINE_LENGTH 1048576

using namespace std;

class TagReader
{
private:
	static char charBuffer[MAX_LINE_LENGTH];
	static char* bufferEnd;

public:
	static string setup();
	static string getNextTagName();
};

