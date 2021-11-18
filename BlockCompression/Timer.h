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
    Timer(string id);
    void start();
    void stop();
    double elapsedMilliseconds();
    double elapsedMicroseconds();
    double elapsedSeconds();

private:
    string name;
    std::chrono::time_point<std::chrono::system_clock> m_StartTime;
    std::chrono::time_point<std::chrono::system_clock> m_EndTime;
    bool m_bRunning = false;
};
