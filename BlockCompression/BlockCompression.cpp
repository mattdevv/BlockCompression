#include <iostream>
#include <thread>
#include "BlockPlane.h"
#include "Timer.h"

int main()
{
    //Timer t("global");

    BlockPlane::setup();

    // setup the blockPlane
    BlockPlane blockPlane1 = BlockPlane();
    BlockPlane blockPlane2 = BlockPlane();

    BlockPlane* pBlockPlane1 = &blockPlane1;
    BlockPlane* pBlockPlane2 = &blockPlane2;

    pBlockPlane1->readBlockPlane();

    // Get next block plane
    while (BlockPlane::canRead())
    {
        auto readingThread = thread(&BlockPlane::readBlockPlane, pBlockPlane2);
        pBlockPlane1->printBlockPlane();

        readingThread.join();

        BlockPlane* temp = pBlockPlane1;
        pBlockPlane1 = pBlockPlane2;
        pBlockPlane2 = temp;
    } 

    pBlockPlane1->printBlockPlane();
    

    //t.stop();

    return 0;
}
