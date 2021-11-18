#pragma once

#include <iostream>
#include <string>
#include <stdio.h>

#define MAX_LINE_LENGTH 1048576

using namespace std;

class TagReader
{
	static char* bufferEnd;
	static char charBuffer[MAX_LINE_LENGTH];

public:
	static string setup();
	static string getNextTagName();
};

