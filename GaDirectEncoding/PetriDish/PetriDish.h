//
//  PetriDish.h
//  PetriDish v0.9
//
//  Replaced by Glenn Sugden on 2012.05.10.
//  Created by Glenn Sugden on 2012.05.05.
//  This source is licensed under the Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.
//  To view a copy of this license, visit http://creativecommons.org/licenses/by-nc-sa/3.0/ or send a letter to Creative Commons, 444 Castro Street, Suite 900, Mountain View, California, 94041, USA.
//

#ifndef GALib_PetriDish_h
#define GALib_PetriDish_h

//================================================================================
#pragma mark Definitions
//================================================================================

//#define GA_CLASS            GASimpleGA
//#define                     USING_GASIMPLEGA

#define GA_CLASS              GASteadyStateGA
//#define GA_CLASS            GAIncrementalGA
//#define GA_CLASS            GADemeGA

#define USE_BOOST           0   // Turn use of boost libraries on or off

//================================================================================
#pragma mark Headers
//================================================================================

#include <ga/ga.h>

#if USE_BOOST
#   include <boost/thread.hpp>
#else//USE_BOOST
#include <pthread.h>
#include <unistd.h>
#endif//USE_BOOST

//================================================================================
#pragma mark Classes
//================================================================================

class PetriDish : public GA_CLASS
{

public:

    // CONSTRUCTOR
    PetriDish(const GAGenome& genomeToClone);

    // EVALUATOR
    static void GAPDEvaluator( GAPopulation& pop );

    // TERMINATOR (interrupt)
    static GABoolean InterruptTerminator(GAGeneticAlgorithm & ga);

    // DESTRUCTOR
    ~PetriDish();

    // UTILITY
    void interrupt()     { _interrupt = true; }

private:

    bool _interrupt;            // A flag indicating we need to bail on the Evaluation

#if USING_GASIMPLEGA
    bool _oldPopInitialized;    // A flag for a GASimpleGA workaround (see GAPDEvaluator)
#endif//USING_GASIMPLEGA

};

#endif
