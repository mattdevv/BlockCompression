#pragma once
#include <iostream>
#include <chrono>
#include <ctime>
#include <cmath>
#include <fstream>
#include <string>

using namespace std;

class Timer
{
public:
    Timer(string outputName, bool startImmediately);
    void start();
    void stop();
    void reset();
    void print();

private:
    string name;
    std::chrono::time_point<std::chrono::system_clock> m_StartTime;
    std::chrono::time_point<std::chrono::system_clock> m_EndTime;
    double totalTime;
    bool running;

    double elapsedSeconds() const;
    double elapsedMilliseconds() const;
    double elapsedMicroseconds() const;
    double getSeconds() const;
    double getMilliseconds() const;
    double getMicroseconds() const;
};
