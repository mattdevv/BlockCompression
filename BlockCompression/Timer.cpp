#include "Timer.h"

Timer::Timer(string outputName, bool startImmediately = true)
{
    name = outputName;
    totalTime = 0;

    if (startImmediately)
		start();
}

void Timer::start()
{
    m_StartTime = std::chrono::system_clock::now();
    running = true;
}

void Timer::stop()
{
    m_EndTime = std::chrono::system_clock::now();
    running = false;

    totalTime += Timer::elapsedMicroseconds();
}

void Timer::reset()
{
    if (running)
        stop();

    totalTime = 0;
}

void Timer::print()
{
    if (running)
        stop();

    ofstream myfile;
    myfile.open("timeoutput.txt", ios::app);
    myfile << 
        name + ":\n" + 
        to_string(getSeconds()) + " seconds\n" + 
        to_string(getMilliseconds()) + " milliseconds\n" + 
        to_string(getMicroseconds()) + " microseconds\n\n";
    myfile.close();
}

double Timer::elapsedSeconds() const
{
    return (double)(std::chrono::duration_cast<std::chrono::seconds>(m_EndTime - m_StartTime).count());
}

double Timer::elapsedMilliseconds() const
{
    return (double)(std::chrono::duration_cast<std::chrono::milliseconds>(m_EndTime - m_StartTime).count());
}

double Timer::elapsedMicroseconds() const
{
    return (double)(std::chrono::duration_cast<std::chrono::microseconds>(m_EndTime - m_StartTime).count());
}

double Timer::getSeconds() const
{
    return totalTime / 1000000.0;
}

double Timer::getMilliseconds() const
{
    return totalTime / 1000.0;
}

double Timer::getMicroseconds() const
{
    return totalTime;
}
