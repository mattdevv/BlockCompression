# Block Compression Algorithm
Branches available
- main: CPU greedy/shelving
- CUDA: GPU greedy
- KDTree: CPU KDTree
## Overview
This algorithm is designed to reduce the number of blocks needed to represent a block model with lossless compression voxels. The block model is a 3D volume containing up to 256 different types of blocks. The volume is subdivied into parent blocks that tile the volume with no remainder, blocks must be always able to be contained in a single parent block.
![KDTree Parrot](https://i.imgur.com/BIMgGgv.png)
## Input Files
Input should be in a .csv or .txt and follow the form:
```
volume_size_x, volume_size_y, volume_size_z, parent_block_size_x, parent_block_size_y, parent_block_size_z
block_position_x, block_position_y, block_position_z, block_size_x, block_size_y, block_size_z, 'block_type' 
block_position_x, block_position_y, block_position_z, block_size_x, block_size_y, block_size_z, 'block_type' 
block_position_x, block_position_y, block_position_z, block_size_x, block_size_y, block_size_z, 'block_type' 
...
block_position_x, block_position_y, block_position_z, block_size_x, block_size_y, block_size_z, 'block_type' 
```
Input blocks must all be 1x1x1 and sorted in row-major order.
## Building/Running the Program
I recommend building with ICPC for the fastest speed. 

To run the program, execute the .exe in a console and pass in the dataset through redirection. 
e.g.
```
excecutable.exe < dataset.txt
```
