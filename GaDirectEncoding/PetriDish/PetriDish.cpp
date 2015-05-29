//
//  PetriDish.cpp
//  PetriDish v0.9
//
//  Replaced by Glenn Sugden on 2012.05.10.
//  Created by Glenn Sugden on 2012.05.05.
//  This source is licensed under the Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.
//  To view a copy of this license, visit http://creativecommons.org/licenses/by-nc-sa/3.0/ or send a letter to Creative Commons, 444 Castro Street, Suite 900, Mountain View, California, 94041, USA.
//

//================================================================================
#pragma mark Headers
//================================================================================

#include "PetriDish.h"
#include <ga/std_stream.h>

#define cout STD_COUT   // MUST go after std_stream
#define cerr STD_CERR   // MUST go after std_stream

//================================================================================
#pragma mark Local Classes
//================================================================================

class BackgroundEvaluator
{
public:

    BackgroundEvaluator( GAGenome* indiv, const unsigned int idx ) : _individual( indiv ), _index( idx ), _finished(false) {}

    ~BackgroundEvaluator() {};

    void newIndividual( GAGenome* indiv, const unsigned int idx )
        { _individual = indiv; _index = idx; _finished = false; }

    bool finished() const                                           { return _finished; }
    void finished(const bool finished)                              { _finished = finished; }

    GAGenome* individual( ) const                                   { return _individual; }
    const unsigned int index( ) const                               { return _index; }

#if USE_BOOST
    void operator( )( );
#endif//USE_BOOST

private:

    GAGenome* _individual;  // The individual associated with the Evaluator
    int _index;             // The (internal) index (1..numThreads) for housekeeping
    bool _finished;         // A flag indicating that the Evaluation is finished
};

#if USE_BOOST
#else//USE_BOOST
void* backgroundEvaluate(void *backgroundEvaluatorData );
#endif//USE_BOOST

//================================================================================
#pragma mark Methods
//================================================================================

PetriDish::PetriDish(const GAGenome& genomeToClone) : GA_CLASS(genomeToClone), _interrupt(false)
{
    GAPopulation& thisPop = const_cast<GAPopulation&>(this->population());

    // Install our own evaluator that kicks off evaluations in the background (below)
    thisPop.evaluator( GAPDEvaluator );

#if USING_GASIMPLEGA
    // A necessary workaround flag (see below)
    _oldPopInitialized = false;
#endif//USING_GASIMPLEGA
}

PetriDish::~PetriDish()
{
#if DEBUG
    cout << statistics();
#endif//DEBUG
}

void PetriDish::GAPDEvaluator( GAPopulation & pop )
{
    assert(pop.size() > 0);

    // Since this is a static method, get this population's associated PetriDish
    PetriDish* thisPetriDish = dynamic_cast<PetriDish*>(pop.geneticAlgorithm());
    assert(thisPetriDish);

#ifdef USING_GASIMPLEGA
    // A workaround for  oldPop not copying our Evaluator to the next set of Genomes (as opposed to hacking GASimpleGA.C)
    if ( thisPetriDish->_oldPopInitialized == false )
    {
        thisPetriDish->_oldPopInitialized = true;
        thisPetriDish->oldPop->initialize();
    }
#endif//USING_GASIMPLEGA

    // Use all of the available cores (as reported by the processor) for threads
    unsigned numberOfThreadsToUse;

#if USE_BOOST
    numberOfThreadsToUse = boost::thread::hardware_concurrency();
#else//USE_BOOST
    numberOfThreadsToUse = (unsigned)sysconf( _SC_NPROCESSORS_ONLN );
    int errorCode;
#endif//USE_BOOST

    // Allocate an array of pointers for the threads, as well as the associated background information
#if USE_BOOST
    boost::thread** backgroundThreads = new boost::thread*[numberOfThreadsToUse];
#else//USE_BOOST
    pthread_t* backgroundThreads = new pthread_t[numberOfThreadsToUse];
#endif//USE_BOOST
    assert( backgroundThreads );
    memset( backgroundThreads, 0, numberOfThreadsToUse * sizeof( void* ) );

    BackgroundEvaluator** backgroundEvaluators = new BackgroundEvaluator*[numberOfThreadsToUse];
    assert( backgroundEvaluators );
    memset( backgroundEvaluators, 0, numberOfThreadsToUse * sizeof( BackgroundEvaluator* ) );

#if DEBUG
    cout << "Evaluating new GAPopulation ( " << pop.size( ) << " )... ( using " << numberOfThreadsToUse << " threads )" << std::endl;
#endif//DEBUG

    int completed = 0;
    int indIndex = 0;
    u_int32_t tIndex = 0U;

#if DEBUG
    float highestScoreSoFar = 0.0f;
    float total = 0.0f;
#endif//DEBUG

    // Loop until every population member has been evaluated.
    while ( completed < pop.size( ) && ( thisPetriDish->_interrupt == false ) )
    {
        // If we still have members to evaluate, and there's a free thread open
        if ( ( indIndex < pop.size( ) ) && ( backgroundThreads[tIndex] == NULL ) )
        {
            // If we haven't allocated space for the evaluator thread information
            if( backgroundEvaluators[tIndex] == NULL )
            {
                backgroundEvaluators[tIndex] = new BackgroundEvaluator( &pop.individual( indIndex ), indIndex );
                assert( backgroundEvaluators[tIndex] != NULL );
            }
            else
            {
                assert(backgroundEvaluators[tIndex]->finished() == true);

                backgroundEvaluators[tIndex]->newIndividual( &pop.individual( indIndex ), indIndex);
            }
#if DEBUG
            cout << "Starting individual #" << indIndex + 1 << " ( of " << pop.size( ) << " )" << std::endl;
#endif//DEBUG

            // Kick off the thread
#if USE_BOOST
            try
            {
                backgroundThreads[tIndex] = new boost::thread( boost::ref(*backgroundEvaluators[tIndex]) );
            }
            catch(const std::exception& e)
            {
                cerr << "boost::thread exception: " << e.what() << std::endl;
                return;
            }
#else//USE_BOOST
            errorCode = pthread_create(&backgroundThreads[tIndex], NULL, backgroundEvaluate, backgroundEvaluators[tIndex]);
            if (errorCode!=0)
            {
                cerr << "pthread_create: Error #"<< errorCode << " (" << strerror(errorCode) << ")" << std::endl;
                return;
            }
#endif//USE_BOOST
            assert(backgroundThreads[tIndex]);

            indIndex++;
        }

        // Our cyclic thread index checks for the completion of a running thread
        tIndex = ( tIndex+1 ) % numberOfThreadsToUse;

        // A (possibly still running) thread exists...
        if ( backgroundThreads[tIndex] != NULL )
        {
            assert( backgroundEvaluators[tIndex] != NULL );

            // Check to see if it is finished
            if ( backgroundEvaluators[tIndex]->finished() )
            {
                // The thread has finished. Gather some stats (if DEBUGging) and free the thread up for the next individual
                completed++;
#if DEBUG
                GAGenome* genome = backgroundEvaluators[tIndex]->individual( );

                float score = genome->score( );

                total += score;

                cout << "Received results from individual #" << backgroundEvaluators[tIndex]->index( ) << " ( " << completed << " of " << pop.size( ) <<
                " finished ): Score: " << genome->score( ) << " (" << total / (float)completed << " is average so far)" << std::endl;

                if ( score > highestScoreSoFar )
                {
                    cout << "Found new high score so far: ( " << score << " > " << highestScoreSoFar <<" )" << std::endl;
                    highestScoreSoFar = score;
                }
#endif//DEBUG
#if USE_BOOST
                delete backgroundThreads[tIndex];
#else//USE_BOOST
                errorCode = pthread_join(backgroundThreads[tIndex], NULL);
                assert(errorCode==0);
#endif//USE_BOOST
                backgroundThreads[tIndex] = NULL;
            }
            else
            {
                // If it hasn't finished yet, give up some main() thread processor time to the background evaluators
#if USE_BOOST
                boost::thread::yield();
#else//USE_BOOST
                pthread_yield();
#endif//USE_BOOST
            }
        }
    }

    if ( thisPetriDish->_interrupt == true )
    {
        cerr << "...evaluation interrupted!" << std::endl;

        thisPetriDish->terminator(InterruptTerminator);
    }
#if DEBUG
    else
    {
        cout << "...finished evaluating this population." << std::endl;
    }
#endif//DEBUG

    for ( tIndex = 0; tIndex < numberOfThreadsToUse; tIndex++ )
    {
        if ( thisPetriDish->_interrupt == false )
        {
            // Double (sanity) check that all of the threads have actually been processed
            assert( backgroundThreads[tIndex] == NULL );

            if ( backgroundEvaluators[tIndex] != NULL ) // This can happen if all cores aren't used
            {
                assert( backgroundEvaluators[tIndex]->finished() == true );
            }
        }
        else
        {
            // If the GA has been interrupted, try and clean up the background threads
            if ( backgroundThreads[tIndex] != NULL )
            {
#if USE_BOOST
                backgroundThreads[tIndex]->interrupt();
                delete backgroundThreads[tIndex];
#else//USE_BOOST
                pthread_cancel(backgroundThreads[tIndex]);
                errorCode = pthread_join(backgroundThreads[tIndex], NULL);
                assert(errorCode==0);
#endif//USE_BOOST
                backgroundThreads[tIndex] = NULL;
            }
        }

        if ( backgroundEvaluators[tIndex] != NULL ) // This can happen if all cores aren't used
        {
            delete backgroundEvaluators[tIndex]; backgroundEvaluators[tIndex] = NULL;
        }
    }

    // Free up the thread and background information arrays
    if (backgroundThreads) { delete[] backgroundThreads; backgroundThreads = NULL; }
    if (backgroundEvaluators) { delete[] backgroundEvaluators; backgroundEvaluators = NULL; }
}

// Simply call the genome's evaluate function, then mark the threaded genome with an "all finished" flag
// (Simply relying on the thread being "done" turns out to not be very reliable)
#if USE_BOOST
void BackgroundEvaluator::BackgroundEvaluator::operator( )( )
{
    _individual->evaluate( );
    _finished = true;
}
#else//USE_BOOST
void* backgroundEvaluate(void* backgroundEvaluatorData )
{
    assert(backgroundEvaluatorData!=NULL);
    GAGenome* individual = ((BackgroundEvaluator*)backgroundEvaluatorData)->individual();

    assert(individual);

    individual->evaluate( );
    ((BackgroundEvaluator*)backgroundEvaluatorData)->finished(true);
    return (backgroundEvaluatorData);
}
#endif//USE_BOOST

// Force the done() call to always report true (for an interruption)
GABoolean PetriDish::InterruptTerminator(GAGeneticAlgorithm & ga)
{
    // Since this is a static method, coerce the PetriDish
    PetriDish& thisPetriDish = dynamic_cast<PetriDish&>(ga);

    assert(thisPetriDish._interrupt == true);

    return gaTrue;
}
