Check wiki on CPPN-NEAT page to find out how you can run an experiment


HyperNEAT library documentation and readme.
----------------------------------------

HyperNEAT v4.0 C++,
By Jason Gauci
http://ucfpawn.homelinux.com/joomla/
jgauci@cs.ucf.edu

Documentation for this package is included in this README file.  

-------------
1. LICENSE
-------------

This program is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License version 2 as published
by the Free Software Foundation (LGPL may be granted upon request). This 
program is distributed in the hope that it will be useful, but without any 
warranty; without even the implied warranty of merchantability or fitness for 
a particular purpose. See the GNU General Public License for more details.

---------------------
2. USAGE and SUPPORT
---------------------

We hope that this software will be a useful starting point for your own
explorations in interactive evolution with NEAT. The software is provided 
as is; however, we will do our best to maintain it and accommodate
suggestions. If you want to be notified of future releases of the
software or have questions, comments, bug reports or suggestions, send
an email to jgauci@cs.ucf.edu

Alternatively, you may post your questions on the NEAT Users Group at :
http://tech.groups.yahoo.com/group/neat/.

The following explains how to use HyperNEAT.  For information on compiling
HyperNEAT, please see the section on compiling below.

INTRO
-----
HyperNEAT is an extension of NEAT (NeuroEvolution of Augmenting Topologies)
that evolves CPPNs (Compositional Pattern Producing Networks) that
encode large-scale neural network connectivity patterns.  A complete
explanation of HyperNEAT is available here:

@InProceedings{gauci:aaai08,
  author       = "Jason Gauci and Kenneth O. Stanley",
  title        = "A Case Study on the Critical Role of Geometric Regularity in Machine Learning",
  booktitle    = "Proceedings of the Twenty-Third AAAI Conference on Artificial Intelligence (AAAI-2008).",
  year         = 2008,
  publisher    = "AAAI Press",
  address      = "Menlo Park, CA",
  site         = "Chicago",
  url          = "http://eplex.cs.ucf.edu/publications.html#gauci.aaai08",
}

@InProceedings{gauci:gecco07,
  author       = "Jason Gauci and Kenneth O. Stanley",
  title        = "Generating Large-Scale Neural Networks Through Discovering Geo
metric Regularities",
  booktitle    = "Proceedings of the Genetic and Evolutionary
                  Computation Conference (GECCO 2007)",
  year         = 2007,
  publisher    = "ACM Press",
  address      = "New York, NY",
  site         = "London",
  url          = "http://eplex.cs.ucf.edu/index.php?option=com_content&task=view&id=14&Itemid=28#gauci.gecco07",
}

The version of HyperNEAT distributed in this package executed the experiments
in the above papers.

For more information, please visit the EPlex website at:
http://eplex.cs.ucf.edu/

or see more of our publications on HyperNEAT and CPPNs at:
http://eplex.cs.ucf.edu/publications.html


COMMAND LINE
------------
To use the command line interface, simply pass the data file and the output
file as parameters to the HyperNEAT executable, and it will process the
data file and put the resulting population as an XML file in the output file
location.

GUI
---
To load an experiment, go to file->load_experiment and then select the .dat
file that corresponds to the experiment you would like to load.

When the experiment is loaded, go to file->run_experiment to begin the
experiment.  In the console, you can see the results for each generation.

If you wish to pause the experiment, go to file->pause_experiment.  This
command will finish the current generation and then halt the experiment.  You
can continue by going to file->continue_experiment.

At the moment, the restart_experiment function has not been implemented
and you can't load another experiment without first closing the program
(although you may "continue experiment" after you have paused).

It is possible to view an individual interactively by choosing the
individual you want from the spinners and then clicking "view".
This brings up a separate window where you can interact with the
individual (if the experiment supports it) and you can also see
the CPPN that created the individual.

Some experiments contain a post-hoc analysis that does a more complete
test of an individual.  This analysis can be achieved by clicking the "Analyze"
button and the results are typically displayed in the console.

--------------
3. EXPERIMENTS
--------------

FIND CLUSTER EXPERIMENT
-----------------------
This is the experiment presented in the GECCO 2007 paper (see above).  The
goal of the experiment is for the network to generate the highest amount of
activation at the center of the larger square, regardless of the positions.

While in the view panel, a number of functions are possible:

To position a square, click on the substrate (the grid).  It will place one 
square and then the other.

To view the connectivity, right-click.  As you move the mouse across the
grid, each square in the grid now represents the connection going from
the grid square under your mouse to that location.  A cross-hatch pattern
represents a negative weight.  You can return to the input layer by 
right-clicking again.

To view the substrate output, middle-click.  The resulting grid represents
the output plane of the state space sandwich.  The darker colors represent
a higher activation level, and a cross-hatch pattern represents a negative
level of activation. you can return to the input layer by middle-clicking.

To save a series of images of your state at several resolutions, click on
"Save Images" at the top.  *NOTE* This takes a very long time!

To increase/decrease the resolution of the substrate, click the 
appropriate wording on the screen. *NOTE* This also takes a very long time!

FIND CLUSTER NO GEOM EXPERIMENT
-------------------------------
This is the "Find Cluster Experiment" but using P-NEAT (perceptron NEAT).
This takes a very long time because it's trying to use NEAT on a
14641-link network.

FIND POINT EXPERIMENT
---------------------
This was the precursor to the FindClusterExperiment.  It looks for a point
on the input substrate and tries to put a similar point on the output
substrate.  I'm not sure if it still works.

TIC TAC TOE EXPERIMENT
----------------------
This was designed as a proof of concept that we could solve Tic Tac Toe.  It tries to set an output value high when there is a tic tac toe (i.e., someone has won).

TIC TAC TOE GAME EXPERIMENT
----------------------
This is a Tic Tac Toe implementation in HyperNEAT.  It can consistently produce a perfect tic tac toe player.  During training, an individual is exposed to every possible tic tac toe game.  Even so, it runs fairly quickly.

CHECKERS EXPERIMENT
-------------------
This is the HyperNEAT implementation of Checkers, as discussed in the AAAI 08 
paper (see above).  The goal of the experiment is to defeat the Simplech 
heuristic in checkers at a ply depth of 4.

If you view an individual, you can play a game of checkers against any
individual at any generation you specify. When you see "Black's turn. Click 
to see black's move", simply click on the board and it will show you 
the move that the computer made. When you see "Move from?", click on the 
white piece that you would like to move, and when you see "Move to?", click 
on the destination.  If you would like to jump a piece or several pieces, 
click on the destination after all the jumps.  If the move was valid, 
you will see the board change and then you can click to see HyperNEAT's 
response.  If the move wasn't valid, you will be asked "Move from?" and 
you must pick a valid move.

*NOTE*:
There is nothing in the fitness function to prevent HyperNEAT from deciding 
that a board evaluation of 0.9999 is completely winning for white and  
0.99999999 is completely winning for black.  As a result, all of the board 
evaluation functions will be in the range [0.9999, 0.99999999].  As a 
result of this, the evaluation process could become dependent on the 
specific architecture of your computer and floating point implementation 
of your compiler.  I have found that changing this line:

typedef float CheckersNEATDatatype;

to:

typedef double CheckersNEATDatatype;

allows you to evaluate a run from one architecture on another architecture 
without losing performance. I have only tested this on a few runs.  
Ideally, if you want to see the real performance of an individual, the 
best thing is to evolve your own player on your machine.

CHECKERS ORIGINAL FOGEL EXPERIMENT
----------------------------------
This is called NEAT-EI in the paper.  It is an implementation of the NEAT 
Checkers player as inspired by Charapella & Fogel (see AAAI 08 paper above). 
This player does not use HyperNEAT, but uses a direct encoding with 
engineered inputs.

*NOTE*: Please see *NOTE* above, as it applies to this method as well.


--------------
4. RESULTS
--------------
A number of completed runs have been placed in the "out/Results/" folder.  
To load one of these runs, select "file->load_population" from the menu bar.

--------------
5. COMPILING
--------------

DEPENDENCIES
--------------
HyperNEAT depends on the TinyXMLDLL Library version 2.0
(http://ucfpawn.homelinux.com/joomla/tinyxmldll/3.html),
JG Template Library (http://sourceforge.net/projects/jgtl/),
Boost c++ libraries (http://www.boost.org/) and
WxWidgets *Only if building in GUI mode* (http://www.wxwidgets.org/).

For information on how
to build & install those libraries, please see their respective websites.

HyperNEAT uses cmake as it's build system, and requires cmake 2.6 or later.

BUILD INSTRUCTIONS:
---------------
UNIX/LINUX/CYGWIN/MACOSX:

Inside the HyperNEAT/build directory, create two subdirectories, one for debug and one for release:

mkdir build/cygwin_debug
mkdir build/cygwin_release

Now, run cmake on each of those directories with the -i option, and put the appropriate configuration (debug for *_debug, release for *_release):

cmake -i ../../

The USE_GUI setting can be set to turn on/off the GUI.  If the GUI is 
disabled, the code does not depend on WxWidgets.

Now, just run "make" on the build/*/ directories and the dlls & libraries should be created in the out/ folder


WINDOWS:

Inside the build directory, create a subdirectory:

build/msvc8 (or whatever version you have)

Now, run cmake on that subdirectory.

The USE_GUI setting can be set to turn on/off the GUI.  If the GUI is 
disabled, the code does not depend on WxWidgets.

Then, open the project and build a debug and release version.

If the compiler is unable to find certain header files, that probably means
that you need to correctly set the CMake build variables that have to
do with the location of include files for the dependencies.

If the linker is unable to find certain library files, that means
that you either need to fix the CMake build variables for libraries,
or that the names of the libraries have changed for some reason.


--------------
6. FORUM
--------------

We are available to answer questions at the NEAT Users Group:

http://tech.groups.yahoo.com/group/neat/


