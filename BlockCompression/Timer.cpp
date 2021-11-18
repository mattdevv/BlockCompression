#include "Timer.h"

Timer::Timer(string id)
{
    name = id;
    start();
}

void Timer::start()
{
    m_StartTime = std::chrono::system_clock::now();
    m_bRunning = true;
}

void Timer::stop()
{
    m_EndTime = std::chrono::system_clock::now();
    m_bRunning = false;

    ofstream myfile;
    myfile.open("timeoutput.txt", ios::app);
    myfile << name + ":\n" + to_string(elapsedSeconds()) + " seconds\n" + to_string(elapsedMilliseconds()) + " milliseconds\n" + to_string(elapsedMicroseconds()) + " microseconds\n\n";
    myfile.close();
}

double Timer::elapsedSeconds()
{
    std::chrono::time_point<std::chrono::system_clock> endTime;

    if (m_bRunning)
    {
        endTime = std::chrono::system_clock::now();
    }
    else
    {
        endTime = m_EndTime;
    }

    return (double)(std::chrono::duration_cast<std::chrono::seconds>(endTime - m_StartTime).count());
}

double Timer::elapsedMilliseconds()
{
    std::chrono::time_point<std::chrono::system_clock> endTime;

    if (m_bRunning)
    {
        endTime = std::chrono::system_clock::now();
    }
    else
    {
        endTime = m_EndTime;
    }

    return (double)(std::chrono::duration_cast<std::chrono::milliseconds>(endTime - m_StartTime).count());
}

double Timer::elapsedMicroseconds()
{
    std::chrono::time_point<std::chrono::system_clock> endTime;

    if (m_bRunning)
    {
        endTime = std::chrono::system_clock::now();
    }
    else
    {
        endTime = m_EndTime;
    }

    return (double)(std::chrono::duration_cast<std::chrono::microseconds>(endTime - m_StartTime).count());
}
