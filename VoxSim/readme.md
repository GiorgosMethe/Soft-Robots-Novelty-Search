## VoxSim

VOXelyze SIMulator is an interface to voxelyze physics engine in which simulations can re-produced.


###fitnessTypes:
----------------------
FT_NONE,		0

FT_COM_DIST,		1	( Displacement from initial position in body lengths )

FT_COM_DIST_XY,		2  	( Displacement from initial position in XY axes in body lengths)

FT_COM_TOTAL_TRAJ_LENGTH,	3 	( Total trajectory distance travelled by SoftBot in body lengths)


###Penalty types
----------------------
PT_NONE, 		0

PT_VOX_NUM, 		1 (=number of voxels in the structure)

PT_ACT_VOX_NUM, 	2 (=number of actuated voxels in the structure)

PT_CONN_NUM, 		3 (=number of connections in the structure)


###BehaviorTypes
----------------------
BT_NONE,		0 (=records nothing)

BT_TRAJECTORY,		1 (=records the 3-D point trajectory) 

BT_TRAJECTORY_XY,	2 (=records 2-D point trajectory at axis parallel to the ground)

BT_PACE,		3 (=records pace at a specific moment)

BT_SHAPE,		4 (=returns a bitstream with the shape)

BT_VOXELS_GROUND,	5 (=returns num of voxels touching the ground per timestep)

BT_KINETIC_ENERGY,	6 (=returns maximum kinetic energy of voxels per timestep)

BT_PRESSURE,	7 (=returns maximum pressure in the structure per timestep)

BT_ALL,	8 (=returns num of voxels touching the ground every timestep)


###Options:
----------------------
-p:	prints stats during simulation

-o:	writes fitness in a file after the simulation is over in the specific path, do not give path "." if you want to write output in the same dir as the input simulation file.

-f:	fitness type

-fp:	fitness penalty function

-fpe:	fitness penalty exponential value

-b:	behavior type to record

-bi:	seconds between each recording

-bs:	seconds from simulation start recording starts

-i:	input simulation file "*.vxa"


###Example format:
----------------------

./VoxSim -i input.vxa -f 1 -fp 3 -fpe 1.5 -b 1 -bi 0.1 -o .

./VoxSim -i input.vxa -f 1 -fp 3 -fpe 1.5 -b 1 -bi 0.1 -o data/


###Notes:
----------------------
The following variables have default values:

-f: 	fitness types				= FT_COM_DIST

-fp: 	fitness penalty function		= PT_NONE

-fpe: 	fitness penalty exponential values	= 1.5

-b: 	behavior type to record			= BT_NONE

-bi: 	seconds between each recording		= 0.01
