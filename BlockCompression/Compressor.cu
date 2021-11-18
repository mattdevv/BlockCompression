#include "Compressor.cuh"

#define BYTES_IN_BLOCK 16
struct __align__(BYTES_IN_BLOCK) Block
{
    ushort3 size;   // 6 bytes
    ulong index;    // 8 bytes
    uchar ID;       // 1 byte
    bool active;    // 1 byte
};

namespace Compressor
{
	const uint KERNEL_SIZE = 64;            // number of kernels in a thread group
	const uint NULL_INDEX = 0xFFFFFFFF;     // uint value representing index of a NULL block
    
    // device global constants
    __constant__ uint3 d_maxThreads;        // how many threads should be run for a Kernel acting on dimension XYZ
	__constant__ ushort3 d_volDimensions;   // dimension sizes of volume loaded on device (currently a block plane)
    __constant__ ushort3 d_pBlockSize;      // size of a parent block
	__constant__ uint3 d_translations;      // precalculated values to move through 3D volume using 1D offsets

    // device pointers to device variables
    uint* dev_numBlocks;				    // counter of found blocks
    uint* dev_nextBlockIndex;			    // counter for current block index
    Block* dev_blockList;				    // pointer to list of blocks
    uint* dev_volume;					    // pointer to loaded volume

    // host variables
    Block* blockList;                       // host-side array of blocks used as location to copy from device into
    uint3 threadsPerDimension;              // how many threads should be run when compressing dimension X, Y, or Z

	uint numBlocks = 0;                     // maximum number of Blocks (a struct) in the current loaded volume
											// type is uint, so maximum number of blocks must not exceed 2^32
											// can happen if volume size is >1600x1600x1600 and no compression possible
											// storing 2^32 blocks would take 65GB though so the program still wouldnt work
											// recommend keeping size less than 1000x1000x1000 and a volume of 1 billion
											// at this size the worst case memory usage is ~20GB

    ushort3 volumeSize;                     // size of entire volume
    ushort3 pBlockSize;                     // size of parent block cells inside volume

    ushort3 deviceVolumeSize;               // exact XYZ size of volume to load to device, does not change
											// not size of whole volume, just amount to load to device at once
	ull deviceVolumeNumVoxels;              // count of how many voxels fit in deviceVolumeSize
											// used to know how many bytes to copy


    // kernel to count maximum number of blocks that will be outputted
    // used to allocate memory for blocks and assign an index to new blocks
    __global__ void countMinimumBlocks(uint* volume, uint* numBlocks)
    {
        // unique thread index from thread group number and interior threadID
        ulong threadID = blockIdx.x * blockDim.x + threadIdx.x;

        // kill thread if index is more than needed amount
        if (threadID >= d_maxThreads.x)
            return;

        // convert threadID to 1D position in volume
        // only places threads along the first YZ slice of parent blocks
        ulong planeVolume = d_volDimensions.y * d_volDimensions.z;
        ulong planeIndex = threadID / planeVolume;
        threadID = threadID % planeVolume;

        // starting 1D position in parent block (YZ slice at local X=0)
        ulong threadPos1D = threadID * d_volDimensions.x + planeIndex * d_pBlockSize.x;

        // find where to stop
        ulong endPos1D = threadPos1D + d_pBlockSize.x - 1;

        // start foundBlocks at one as the final voxel is skipped due to out-of-range
        uint foundBlocks = 1;

        while (threadPos1D < endPos1D)
        {
            // count how many times two sequential tags are not equal
            if (volume[threadPos1D] != volume[threadPos1D + 1])
            {
                foundBlocks++;
            }
            threadPos1D++;
        }

        // thread-safe addition to global counter
        atomicAdd(numBlocks, foundBlocks);
    }

    // kernel to perform x compression and create the first list of blocks
    // loaded volume is also converted from representing tags, to the index in the block-list occupying that space
    // a null tag is used for all voxels besides the block's origin so the block can only be accessed from reading one voxel
    __global__ void xRowCompression(uint* volume, Block* blockList, uint* nextBlockIndex)
    {
        // unique thread index from thread group number and interior threadID
        ulong threadID = blockIdx.x * blockDim.x + threadIdx.x;

        // kill thread if index is more than needed amount
        if (threadID >= d_maxThreads.x)
            return;

        // calculate 1D position
        ulong planeVolume = d_volDimensions.y * d_volDimensions.z;
        ulong planeIndex = threadID / planeVolume;
        threadID = threadID % planeVolume;

        // 1D position in volume (YZ slice at X=0 for some parent block)
        ulong threadPos1D = threadID * d_volDimensions.x + planeIndex * d_pBlockSize.x;

        // find where to stop
        ulong endPos1D = threadPos1D + d_pBlockSize.x;

        // start the first block line
        ushort length = 1;
        uchar prevTag = volume[threadPos1D];
        uint blockOrigin1D = threadPos1D;

        // move over first voxel which was already read
        threadPos1D++;

        // thread moves along x axis for length of parent block X
        while (threadPos1D < endPos1D)
        {
            uchar thisTag = volume[threadPos1D];

            // grow line if same
            if (thisTag == prevTag)
            {
                length++;
            }
            // break line if different
            else
            {
                // request a blockList index
                uint blockIndex = atomicAdd(nextBlockIndex, 1);

                // backtrack and set voxels to null
                // origin is set to be the block's index
                volume[blockOrigin1D] = blockIndex;
                for (ushort j = 1; j < length; j++)
                {
                    volume[blockOrigin1D + j] = NULL_INDEX;
                }

                // store the block in the list
                blockList[blockIndex] = {
                    { length, 1, 1 },
                    blockOrigin1D,
                    prevTag,
                    true };

                // reset line
                prevTag = thisTag;
                length = 1;
                blockOrigin1D = threadPos1D;
            }

            // move along X once
            threadPos1D++;
        }

        // manually create the final block
        // request a blockList index
        uint blockIndex = atomicAdd(nextBlockIndex, 1);

        // backtrack and set voxels to null
        // origin is set to be the block's index
        volume[blockOrigin1D] = blockIndex;
        for (ushort j = 1; j < length; j++)
        {
            volume[blockOrigin1D + j] = NULL_INDEX;
        }

        __syncthreads();

        // store the block in the list
        blockList[blockIndex] = {
            { length, 1, 1 },
            blockOrigin1D,
            prevTag,
            true };
    }

    // kernel to perform y compression
    // blocks eliminated by compression are removed from volume but still in block-list marked as inactive 
    __global__ void yRowCompression(uint* volume, Block* blockList)
    {
        // unique thread index from thread group number and interior threadID
        ulong threadID = blockIdx.x * blockDim.x + threadIdx.x;
        // kill thread if higher than pre-calculated allowed amount
        if (threadID >= d_maxThreads.y)
            return;

        // variables to help find thread position
        const ulong planeVolume = d_volDimensions.x * d_volDimensions.z;
        const ulong planeIndex = threadID / planeVolume;
        const uint pBlockYTranslation = d_pBlockSize.y * d_translations.y;
        threadID = threadID % planeVolume;

        // find thread's individual offsets per dimension
        const ulong offset1DX = threadID % d_volDimensions.x;
        const ulong offset1DY = planeIndex * pBlockYTranslation;
        const ulong offset1DZ = (threadID / d_volDimensions.x) * d_translations.z;

        // 1D position in volume (XZ slice at Y=0 for some parent block)
        ulong threadPos1D = offset1DX + offset1DY + offset1DZ;

        ulong endPos1D = threadPos1D + pBlockYTranslation;

        Block* p_blockBelow;
        Block blockBelow;
        bool canMergeBelow = false;

        // thread moves along y axis for length of parent block Y
        while (threadPos1D < endPos1D)
        {
            // get reference to block at thread position
            uint blockIndex = volume[threadPos1D];

            // if null, thread is not inside the origin of a block
            // must be inside origin for thread to be allowed to use that block
            if (blockIndex == NULL_INDEX)
            {
                canMergeBelow = false;
                threadPos1D += d_translations.y;
                continue;
            }

            Block* p_block = &blockList[blockIndex];
            Block block = blockList[blockIndex];

            if (canMergeBelow 
                && block.size.x == blockBelow.size.x 
                && block.ID == blockBelow.ID)
            {
                // update blocks
                p_block->active = false;
                p_blockBelow->size.y++;

                // disable top block in index volume so Z pass doesn't use it
                volume[threadPos1D] = NULL_INDEX;
            }
            else
            {
                p_blockBelow = p_block;
                blockBelow = block;
                canMergeBelow = true;
            }

            threadPos1D += d_translations.y;
        }
        
    }

    // kernel to perform z compression
    // blocks eliminated by compression are kept in the volume and still in block-list marked as inactive 
    __global__ void zRowCompression(uint* volume, Block* blockList)
    {
        // unique thread index from thread group number and interior threadID
        ulong threadID = blockIdx.x * blockDim.x + threadIdx.x;
        // kill thread if higher than pre-calculated allowed amount
    	if (threadID >= d_maxThreads.z)
            return;

        // 1D position in volume (XY slice at Z=0, always bottom parent block)
        ulong threadPos1D = threadID;

        // calculate maximum offset from starting position
        ulong endPos1D = threadPos1D + d_volDimensions.z * d_translations.z;

        // store a previous block
        Block* p_blockBelow;
        Block blockBelow;
        bool canMergeBelow = false; // check if stored block is valid merge

        // thread moves along z axis for length of parent block Z
        while (threadPos1D < endPos1D)
        {
            // get reference to block at thread position
            uint blockIndex = volume[threadPos1D];

            // if null, thread is not inside the origin of a block
            // must be inside origin for thread to be allowed to use that block
            if (blockIndex == NULL_INDEX)
            {
                canMergeBelow = false;
                threadPos1D += d_translations.z;
                continue;
            }

            // index not null, grab current block pointer + copy
            Block* p_block = &blockList[blockIndex];
            Block block = blockList[blockIndex];

            // if last block was a valid merge
            // and both blocks have same XY dimensions
            // and both blocks have same tagIDs
            if (canMergeBelow 
                && block.size.x == blockBelow.size.x 
                && block.size.y == blockBelow.size.y 
                && block.ID == blockBelow.ID)
            {
                // perfect match => can merge top block into bottom block
                p_block->active = false;
                p_blockBelow->size.z++;
                // we dont need to update volume as no other threads will execute
                //volume[threadPos1D] = NULL_INDEX;
            }
            else
            {
                // did not match but top block might match next block so store it
                p_blockBelow = p_block;
                blockBelow = block;
                canMergeBelow = true;
            }

            // move along Z once 
            threadPos1D += d_translations.z;
        }
    }

    // set constant variables on device
    __host__ void setupDeviceConstants()
    {
    	// Calculate how many 1D voxels to move over for a single translation in each 3D dimension
        uint3 translations = {
            (uint)1, // one voxel
            (uint)deviceVolumeSize.x, // one x row
            (uint)(deviceVolumeSize.x * deviceVolumeSize.y) // one xy plane
        }; 

        // Calculate how many voxels exist in a single slice per dimension
        uint3 sliceVolumes = {
            deviceVolumeSize.y * deviceVolumeSize.z,
            deviceVolumeSize.x * deviceVolumeSize.z,
            deviceVolumeSize.x * deviceVolumeSize.y
        };

        // how many parent-blocks are there in a block plane
        uint3 blocksInVolume = {
        	(uint)(volumeSize.x / pBlockSize.x),
            (uint)(volumeSize.y / pBlockSize.y),
        	1
        };

        // start a thread in every voxel of 1 slice in every parent-block
        // slice is along dimension being compressed, e.g when X slice is YZ plane
        threadsPerDimension = blocksInVolume * sliceVolumes;

        // copy constants to device
        cudaMemcpyToSymbol(d_maxThreads, &threadsPerDimension, sizeof(uint3));
        cudaCheckErrors("set d_maxPlaneIndex failed");
        cudaMemcpyToSymbol(d_volDimensions, &deviceVolumeSize, sizeof(ushort3));
        cudaCheckErrors("set d_volDimensions failed");
        cudaMemcpyToSymbol(d_pBlockSize, &pBlockSize, sizeof(ushort3));
        cudaCheckErrors("set d_pBlockDimensions failed");
        cudaMemcpyToSymbol(d_translations, &translations, sizeof(uint3));
        cudaCheckErrors("set d_translations failed");
    }

    // allocate memory for frequently used variables
    __host__ void reserveDeviceMemory()
    {
        // Reserve memory for the volume on GPU device
        cudaMalloc((void**)&dev_volume, getVolume(deviceVolumeSize) * sizeof(uint));
        cudaCheckErrors("malloc device volume failed");

        // counter for the maximum number of blocks
        cudaMalloc((void**)&dev_numBlocks, 1 * sizeof(uint));
        cudaCheckErrors("malloc device numBlocks failed");

        // counter for the number of blocks that have been created
        cudaMalloc((void**)&dev_nextBlockIndex, 1 * sizeof(uint));
        cudaCheckErrors("malloc device nextBlockIndex failed");
    }

    // do all setup to start using device
    __host__ void setupDevice(ushort3& descriptionVolumeSize, ushort3& descriptionParentBlockSize)
    {
        volumeSize = descriptionVolumeSize;
        pBlockSize = descriptionParentBlockSize;

        // device will read an XY plane 1 parent block thick along Z
        deviceVolumeSize = { volumeSize.x, volumeSize.y, pBlockSize.z };
        deviceVolumeNumVoxels = getVolume(deviceVolumeSize);

        setupDeviceConstants();
        reserveDeviceMemory();
    }

    // move a volume of voxels onto device
    __host__ void copyVolumeToDevice(uint* volume)
    {
        // Copy volume of voxels from host to device.
        cudaMemcpy(dev_volume, volume, deviceVolumeNumVoxels * sizeof(uint), cudaMemcpyHostToDevice);
        cudaCheckErrors("memcpy volume to device failed");
    }

    // reset counters for a new parent block
    __host__ void resetBlockCounters()
    {
        const uint zero = 0;
        cudaMemcpy(dev_numBlocks, &zero, sizeof(uint), cudaMemcpyHostToDevice);
        cudaCheckErrors("device set numBlocks=0 failed");
        cudaMemcpy(dev_nextBlockIndex, &zero, sizeof(uint), cudaMemcpyHostToDevice);
        cudaCheckErrors("device set nextBlockIndex=0 failed");
    }

    // creates array to hold the outputted blocks
	// size of array is calculated by a kernel that sweeps the entire volume
    __host__ void createBlockList()
    {
        // set values in device counters to 0
        resetBlockCounters();

        static const uint numThreadBlocks = divideCeil(threadsPerDimension.x, KERNEL_SIZE);

        // count number of blocks needed for run-length encoding
        countMinimumBlocks <<< numThreadBlocks, KERNEL_SIZE >>> (dev_volume, dev_numBlocks);
        cudaCheckErrors("countMinimumBlocks kernel failed");

        // copy the counter to host
        cudaMemcpy(&numBlocks, dev_numBlocks, sizeof(uint), cudaMemcpyDeviceToHost);
        cudaCheckErrors("cudaMemcpy numBlocks failed");
    	//cout << "Num Blocks: " << numBlocks << endl;

        // Reserve memory for the blockList on host and device
        blockList = new Block[numBlocks];
        cudaMalloc((void**)&dev_blockList, numBlocks * BYTES_IN_BLOCK);
        cudaCheckErrors("malloc device blockList failed");
    }

    // free device and host memory containing block-list
    __host__ void deleteBlockList()
    {
        delete[] blockList;
        cudaFree(dev_blockList);
    }

    // after all variables have been setup compress the voxels
    __host__ void compressVolume()
    {
        // compress along X
        static const uint numBlocksX = divideCeil(threadsPerDimension.x, KERNEL_SIZE);
        xRowCompression <<< numBlocksX, KERNEL_SIZE >>> (dev_volume, dev_blockList, dev_nextBlockIndex);
        cudaCheckErrors("xrow compression kernel failed");

        // compress along Y
        static const uint numBlocksY = divideCeil(threadsPerDimension.y, KERNEL_SIZE);
        yRowCompression <<< numBlocksY, KERNEL_SIZE >>> (dev_volume, dev_blockList);
        cudaCheckErrors("yrow compression kernel failed");

        // compress along Z
        static const uint numBlocksZ = divideCeil(threadsPerDimension.z, KERNEL_SIZE);
        zRowCompression <<< numBlocksZ, KERNEL_SIZE >>> (dev_volume, dev_blockList);
        cudaCheckErrors("zrow compression kernel failed");
    }
    
    // convert 1D position to 3D
    __host__ ushort3 convert1DIndexTo3D(const ulong index)
    {
        static const uint xyPlaneVolume = (deviceVolumeSize.x * deviceVolumeSize.y);

        const ushort xDirection = index % deviceVolumeSize.x;
        const ushort yDirection = (index / deviceVolumeSize.x) % deviceVolumeSize.y;
        const ushort zDirection = index / xyPlaneVolume;

        return { xDirection, yDirection, zDirection };
    }

    // copy all blocks to host for outputting
    // some invalid blocks will be mixed in marked as inactive
    __host__ void copyBlocksToHost()
    {
        cudaMemcpy(blockList, dev_blockList, numBlocks * BYTES_IN_BLOCK, cudaMemcpyDeviceToHost);
        cudaCheckErrors("memcpy blockList to host failed");
    }

    // write to console the active blocks
    __host__ void printBlocks(const ushort3& offset)
    {
        // print the valid blocks
        for (int i = 0; i < numBlocks; i++)
        {
            const Block& block = blockList[i];

            if (block.active)
            {
                // block only stores 1D position
                // convert back to 3D and add blockPlane offset
                const ushort3 origin = offset + convert1DIndexTo3D(block.index);

                // convert tagIDs back to strings using a tagTable
                std::cout << coordToString(origin) + coordToString(block.size)
                    + "\'" + globalTT.getTag(block.ID) + "\'\n";
            }
        }
    }

    // deallocate reused memory at program end
    __host__ void cleanupDevice()
    {
        // free device variables that are kept between calls
        cudaFree(dev_volume);
        cudaFree(dev_numBlocks);
        cudaFree(dev_nextBlockIndex);
    }

    // after calling setupDevice only need an array of voxels + an offet
    // includes all steps to perform compression and printing + memory management
    __host__ void compressPrint(BlockPlane& pBlock)
    {
        // copy voxels to device
        copyVolumeToDevice(pBlock.volume);

#ifdef USE_TIMER
        cudaEvent_t start, stop;
        cudaEventCreate(&start);
        cudaEventCreate(&stop);
        cudaEventRecord(start);
#endif

        /* a pre-pass counts the maximum blocks that compression will 
        result in and allocates block list to hold output blocks */
        createBlockList();

        // do 3 passes of compression
        compressVolume();

#ifdef USE_TIMER
        cudaEventRecord(stop);
        cudaEventSynchronize(stop);
        float milliseconds = 0;
        cudaEventElapsedTime(&milliseconds, start, stop);
        timerTotal += milliseconds;
#else

    	// copy the list of all blocks
        copyBlocksToHost();

        // copy blocks back to host and print
        printBlocks(pBlock.offset);

#endif

        // cleanup results of this volume
        deleteBlockList();
    }
}