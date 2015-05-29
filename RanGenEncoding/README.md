###RanGenEncoding

## How to compile:
* First you have to compile VoxSim project
* You must have Tinyxml lib installed
* Then, in dir `Soft-Robots-Evolution/RanGenEncoding/`
* Run, `$ mkdir build && cmake .. && make`

## How to run experiment:

Command format: `Command format: ./RanGenEncoding -f <fitnessType> -t <type> -n <numberOfSimulations> -o <outputPath>`

* `-o` Output path of the resulted simulation, where you want to save them.
* `-f` Check VoxSim documentation about this argument, the same fitness function applies here as well.
* `-fp` Check VoxSim documentation about this argument, the same fitness penalty function applies here as well.
* `-t` Type of the simulation, currently two types, 0,1.
* Type `0`: start from a random point on the floor, and add a voxel there. Next, choose random one already inserted voxel and connect to it a new voxel of the same or different material. Continue.
* Type `1`: same as the previous but constructs softbot only in the half bounding box, next whatever has been built will be mirrored to the other side.
* `-n` Number of softbots you want to be created

## How to change the experiment:
You can change the default simulation template in `Soft-Robots-Evolution/SimFiles/default/default.vxa` to change most of the settings, but for the probabilities of adding new voxels and changing the material of the connection you can change them in `RanGenEncoding.cpp`:

`// probability of adding a new voxel`

`const double PROB_ADD = 0.95;`


`// probability that the new voxel will be from the same material`

`const double PROB_SAME = 0.5;`

