### Genetic algorithm experiment

## How to compile
To compile this experiment you need Boost and Qtcore libraries installed at your system.

First go to `GaDirectEncoding/galib247/` and run `$ make install`, this is going to install GAlib.

Before compiling the actual GA experiment make sure you have compiled VoxSim project.

Then, in `GaDirectEncoding` run:

> mkdir build

> cd build

> cmake ..

> make

## How to run
To run the simple genetic algorithm experiment.

In you build directory with the command:
> ./GaDirectEncoding

This command will start the evolution on all threads available. 

The default simulation that its settings are loaded from the Genetic Algorithm encoding is located in: `SimFiles/default/default.vxa`

The results of the evolution are written in file `stats.dat` in the same directory as the executable.