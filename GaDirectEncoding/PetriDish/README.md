<pre>
PetriDish v0.9
==============

A multithreaded Evaluator for the C++ Genetic Algorithm library: GAlib

Requires:
  GALib             2.4.7+  http://lancet.mit.edu/ga/
  
Optional:
  Boost (threads)   1.4.9   http://www.boost.org/

CHANGES:  2013.04.17  Boost threads are now optional (provided support for pthreads).
                      ** this still needs some testing! **

IMPORTANT:  This framework is intended for Genomes that take a long time to
            process individually (I.E., their Objective() function takes a long time to
            compute). If you are "evolving" dozens of genomes over many generations very
            rapidly, you will see an overall increase in time it takes to reach the
            Termination point. I created this framework in response to a number of people
            asking me how I multithreaded the library for some research I was doing (in
            which my Objective() function takes minutes to compute for each Genome).

TESTING:  I have tested each of the examples included with GALib, and they
          result in the exact same final output (although I had to tweak some of the
          examples due to the RNG being seeded after some of the initial numbers were
          generated). I haven't included the examples because of: licensing simplicity,
          the modifications are very minor (below), and they were created with speed in
          mind (simple Objective functions) - they take longer to compute than their
          single-threaded, serial versions. I will post some examples that take advantage
          of multiple cores ASAP.

Step by step instructions:

  1) Download and install the above libraries (if you don't have them already).

     I.E., Make sure that you can compile and execute at least one of the GALib
      examples.

  2) Git the latest version of PetriDish, and make it accessible to your
     development environment.

     I.E., You can #include the "PetriDish.h" header and compile/link "PetriDish.cpp"

       I wanted to try and automate the following steps, but I couldn't find a clean
       way to do it without changing the GAlib-rary itself. This was the least
       invasive / simplest method I could come up with:

  3) Change the GA_CLASS #definition in PetriDish.h to match the GeneticAlgorithm
     that you are using.

     E.G., in ex1.C: make sure the #definition is GASimpleGA.

  4) Set the USING_GASIMPLEGA #definition to match whether or not you are using
     the GASimpleGA Class. This is necessary for a workaround that is documented in
     the comments in the code.

     I.E., Set it to 1 if you are using GASimpleGA, otherwise set it to 0 if you aren't.

  5) Now simply change the type of your GA variable to "PetriDish"

     E.G., in ex1.C,  line 63: instead of "GASimpleGA ga(genome);" you would use
     "PetriDish ga(genome);"
     
  6) #include "PetriDish.h" in the source files that require it.
  
     E.G. at the top of ex1.C

                              That's (basically) it!

NOTES: The Evaluator shell itself isn't threaded. A call to "evolve()" (or
       "step()," or even "initialize()") will execute on the main thread (I.E. it will
       finish the "step()" before continuing to the next instruction). While it is
       possible to have everything run completely in the background (on another
       thread), it isn't a very efficient use of a background core at all. The
       Evaluator is just basically spinning while waiting for all of the Genomes'
       Objective()s to finish. If there is enough interest, I will include a compiler
       flag (along with some polling or callback code) as an option in the future. For
       now, feel free to do some work between steps (I personally plot stats so I can
       watch the progress).

       There is an "interrupt" method (that can be invoked via "ga.interrupt()")
       that will: halt the Evaluator, try to interrupt the threads, and clean up
       the memory that has been allocated. I haven't thoroughly torture-tested this
       yet, so YMMV a bit.

       By compiling with the DEBUG symbol defined (as 1, for instance), you'll
       get some statistics output'd to STD_COUT. It can be a lot of text, so the
       compiler flag is used (for, say, release versions) to drop the verbosity
       levels.


(POSSIBLE)     - Automate a Genome's Objective function's handling of a (global)
FUTURE PLANS:    GA interrupt.
               - Provide ways to share data among the Genomes (my versions do
                 this, and it isn't trivial).
               - Order of difficulty: Preload shared data, load shared data
                 dynamically, data modification
               - A threaded (background) version of the Evaluator itself.
               - Performance instrumentation
               - Time to completion estimation
</pre>
