#include <iostream>
#include <string>
#include <thread>
#include "BlockPlane.h"
#include "Timer.h"

int main()
{
    //Timer t("global", true);

    BlockPlane::setup();

    // check if only 1 block plane is needed
    // if so run program using only 1
    if (BlockPlane::canUseOnePlane())
    {
        BlockPlane blockPlane = BlockPlane();
        blockPlane.readBlockPlane();
        blockPlane.printBlockPlane();

        //t.print();
        return 0;
    }

    // must setup all BlockPlanes together before using any
    BlockPlane blockPlane1 = BlockPlane();
    BlockPlane blockPlane2 = BlockPlane();

    // pointers used to switch which is number 1/2
    BlockPlane* writingPlane = &blockPlane1;
    BlockPlane* readingPlane = &blockPlane2;

    // start reading the first plane so it can write while another reads
    writingPlane->readBlockPlane();

    // stop when all planes have been read
    while (BlockPlane::canRead())
    {
        // start reading a second plane on background thread
        auto readingThread = thread(&BlockPlane::readBlockPlane, readingPlane);

        // print on main thread the results from first plane
        writingPlane->printBlockPlane();

        // once all threads are done switch thread's task between reading and writing
        readingThread.join();
        BlockPlane* temp = writingPlane;
        writingPlane = readingPlane;
        readingPlane = temp;
    } 

    // print final plane
    writingPlane->printBlockPlane();
    
    //t.print();

    return 0;
}
