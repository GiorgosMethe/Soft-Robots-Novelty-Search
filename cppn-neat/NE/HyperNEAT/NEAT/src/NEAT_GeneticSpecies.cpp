#include "NEAT_Defines.h"

#include "NEAT_GeneticSpecies.h"

#include "NEAT_Globals.h"
#include "NEAT_Random.h"

#include "NEAT_GeneticIndividual.h"

namespace NEAT
{

GeneticSpecies::GeneticSpecies(shared_ptr<GeneticIndividual> firstIndividual)
    :
      age(0),
      ageOfLastImprovement(0),
      speciesFitness(0),
      oldAverageFitness(0)
{
    ID = Globals::getSingleton()->generateSpeciesID();
    firstIndividual->setSpeciesID(ID);
    bestIndividualEver = firstIndividual;
    Competition = int(Globals::getSingleton()->getParameterValue("SpeciesCompetition"));
    CompetitionSize = int(Globals::getSingleton()->getParameterValue("SpeciesCompetitionSize"));
}

GeneticSpecies::~GeneticSpecies()
{}

void GeneticSpecies::setBestIndividual(shared_ptr<GeneticIndividual> ind)
{
    bestIndividualEver = ind;
    // Base improvement age off of best individual update.
    ageOfLastImprovement=age;
}

void GeneticSpecies::setMultiplier()
{
    int dropoffAge = int(Globals::getSingleton()->getParameterValue("DropoffAge"));

    multiplier = 1;

    int ageSinceImprovement = age-ageOfLastImprovement;

    if (ageSinceImprovement>=dropoffAge)
    {
        multiplier *= 0.001;
    }

    //Give a fitness boost up to some young age (niching)
    //The age_significance parameter is a system parameter
    //  if it is 1, then young species get no fitness boost
    if (age<10)
        multiplier *= Globals::getSingleton()->getParameterValue("AgeSignificance");

    //Share fitness with the species
    multiplier /= currentIndividuals.size();
}

void GeneticSpecies::dump(TiXmlElement *speciesElement)
{
    //Not implemented.  Not sure what I would want to dump.
}

void GeneticSpecies::setFitness()
{
    speciesFitness=0;
    for (int a=0;a<(int)currentIndividuals.size();a++)
    {
        shared_ptr<GeneticIndividual> individual = currentIndividuals[a];

        speciesFitness+=individual->getFitness();
    }

    // Base improvement age off of average.  Rarely executed because average is always going up/down
    //if(speciesFitness/currentIndividuals.size() > oldAverageFitness)
    // ageSinceLastImprovement=age;

    oldAverageFitness = speciesFitness/currentIndividuals.size();
}

void GeneticSpecies::incrementAge()
{
    age++;
    for (int a=0;a<(int)currentIndividuals.size();a++)
    {
        currentIndividuals[a]->incrementAge();
    }
}

void GeneticSpecies::makeBabies(vector<shared_ptr<GeneticIndividual> > &babies)
{
    // How many will survive
    int lastIndex;
    if(Competition == 0) // When no competition only a proportion of the population will
    {
        lastIndex = int(Globals::getSingleton()->getParameterValue("SurvivalThreshold") * currentIndividuals.size());
        // In case of no competition set all others that they cannot reproduce
        for (int a=lastIndex+1;a<(int)currentIndividuals.size();a++)
        {
            currentIndividuals[a]->setCanReproduce(false);
        }
    }
    else
    {
        // All can reproduce
        lastIndex = currentIndividuals.size()-1;
    }

    double mutateOnlyProb = Globals::getSingleton()->getParameterValue("MutateOnlyProbability");
    for (int a=0;offspringCount>0;a++)
    {
        if (a>=1000000)
        {
            cerr << "Error while making babies, need to choose the best of the species and bail!\n";
            exit(35);
        }

        bool onlyOneParent = (int(lastIndex)==0);
        // Has only one parent or choose to mutate
        if (onlyOneParent || Globals::getSingleton()->getRandom().getRandomDouble() < mutateOnlyProb)
        {
            // Takes a random parent from the list
            shared_ptr<GeneticIndividual> parent;
            int parentIndex;
            if(Competition == 0)  parentIndex = Globals::getSingleton()->getRandom().getRandomWithinRange(0,int(lastIndex)); // No competition we return a random parent
            else parentIndex = chooseParentViaCompetition(lastIndex);

            parent = currentIndividuals[parentIndex];
            babies.push_back(shared_ptr<GeneticIndividual>(new GeneticIndividual(parent,true)));
            offspringCount--;
        }
        else // Two parents no mutation
        {
            shared_ptr<GeneticIndividual> parent1,parent2;
            std::pair<int, int> parentsIndexes;
            if(Competition == 0)  parentsIndexes =  chooseParentsRandomly(lastIndex);// No competition we return a random parent
            else parentsIndexes = chooseParentsViaCompetition(lastIndex);

            parent1 = currentIndividuals[parentsIndexes.first]; // assign first parent
            parent2 = currentIndividuals[parentsIndexes.second]; // assign seconds parent
            if (parent1==parent2) { cout_ << "Asexual reproduction\n"; babies.push_back(shared_ptr<GeneticIndividual>(new GeneticIndividual(parent1,true)));} // Make asexual reproduction
            else babies.push_back(shared_ptr<GeneticIndividual>(new GeneticIndividual(parent1,parent2))); // Normal reproduction
            offspringCount--;
        }
    }
}

int GeneticSpecies::chooseParentViaCompetition(int lastIndex)
{
    switch (Competition)
    {
    case 1: // Select parent based on its global fitness
    {
        double bestFitness = numeric_limits<double>::min();
        int bestParent = 0;
        for (int i = 0; i < CompetitionSize; ++i)
        {
            int parent = Globals::getSingleton()->getRandom().getRandomWithinRange(0,int(lastIndex));
            double fitness = 0.0;
            if( int(Globals::getSingleton()->getParameterValue("NoveltySearch")) == 1) fitness = currentIndividuals[parent]->getFitness();
            else fitness = currentIndividuals[parent]->getFitnessBackUp();
            if(fitness > bestFitness)
            {
                bestFitness = fitness;
                bestParent = parent;
            }
        }
        return bestParent;
    }
    case 2: //Novelty, Select parent based on its global novelty
    case 3: //Novelty, Select parent based on its global novelty
    {
        if( int(Globals::getSingleton()->getParameterValue("NoveltySearch")) != 1)
        {
            cerr << "ERROR : COMPETITION ERROR -- Tried to get novelty when novelty search is not enabled" << endl;
            exit(35);
        }
        double bestNovelty = 0.0;
        int bestParent;
        for (int i = 0; i < CompetitionSize; ++i)
        {
            int parent = Globals::getSingleton()->getRandom().getRandomWithinRange(0,int(lastIndex));
            double novelty = 0.0;
            novelty = currentIndividuals[parent]->getFitness();
            if(novelty > bestNovelty)
            {
                bestNovelty = novelty;
                bestParent = parent;
            }
        }
        return bestParent;
    }
    default:
        cerr << "ERROR : COMPETITION ERROR" << endl;
        exit(35);
    }
}

std::pair<int, int> GeneticSpecies::chooseParentsRandomly(int lastIndex)
{
    shared_ptr<GeneticIndividual> parent1,parent2;
    // First take a random parent
    int parentIndex1 = Globals::getSingleton()->getRandom().getRandomWithinRange(0,int(lastIndex));
    int parentIndex2 = 0;
    parent1 = currentIndividuals[parentIndex1];
    // Choose second different from the first one parent
    int iteration=0;
    do
    {
        iteration++;
        parentIndex2 = Globals::getSingleton()->getRandom().getRandomWithinRange(0,int(lastIndex));
        parent2 = currentIndividuals[parentIndex2];
    }
    while (parent2 == parent1 && iteration <= 1000000);
    return make_pair(parentIndex1, parentIndex2);
}

std::pair<int, int> GeneticSpecies::chooseParentsViaCompetition(int lastIndex)
{
    switch (Competition)
    {
    case 1: //Novelty, Select a pair of parents based on their global fitness
    {
        double bestCoupleFitness = 0.0;
        std::pair<int, int> bestCouple(0,0);
        for (int i = 0; i < CompetitionSize; ++i) {
            // Take two random individuals
            int parent1 = Globals::getSingleton()->getRandom().getRandomWithinRange(0,int(lastIndex));
            int parent2 = Globals::getSingleton()->getRandom().getRandomWithinRange(0,int(lastIndex));
            if(parent1 == parent2) { i--; continue; } // if they same, continue to next iteration
            double coupleFitness = 0.0;
            if( int(Globals::getSingleton()->getParameterValue("NoveltySearch")) == 1)
                coupleFitness += currentIndividuals[parent1]->getFitnessBackUp() + currentIndividuals[parent2]->getFitnessBackUp();
            else
                coupleFitness += currentIndividuals[parent1]->getFitness() + currentIndividuals[parent2]->getFitness();
            if(coupleFitness > bestCoupleFitness)
            {
                bestCoupleFitness = coupleFitness;
                bestCouple = std::make_pair(parent1, parent2);
            }
        }
        return bestCouple;
    }
    case 2: //Novelty, Select a pair of parents based on their global novelty, two most novel parent of the specie population
    {
        if( int(Globals::getSingleton()->getParameterValue("NoveltySearch")) != 1)
        {
            cerr << "ERROR : COMPETITION ERROR -- novelty search is not enabled" << endl;
            exit(35);
        }
        double bestCoupleNovelty = 0.0;
        std::pair<int, int> bestCouple(0,0);
        for (int i = 0; i < CompetitionSize; ++i) {
            // Take two random individuals
            int parent1 = Globals::getSingleton()->getRandom().getRandomWithinRange(0,int(lastIndex));
            int parent2 = Globals::getSingleton()->getRandom().getRandomWithinRange(0,int(lastIndex));
            if(parent1 == parent2) { i--; continue; }
            double coupleNovelty = currentIndividuals[parent1]->getFitness() + currentIndividuals[parent2]->getFitness();
            if(coupleNovelty > bestCoupleNovelty)
            {
                bestCoupleNovelty = coupleNovelty;
                bestCouple = std::make_pair(parent1, parent2);
            }
        }
        return bestCouple;
    }
    case 3: //Novelty, Select a pair of parents based on their local novelty, two most different parents in the specie population
    {
        if( int(Globals::getSingleton()->getParameterValue("NoveltySearch")) != 1)
        {
            cerr << "ERROR : COMPETITION ERROR -- novelty search is not enabled" << endl;
            exit(35);
        }
        double bestCoupleNovelty = 0.0;
        std::pair<int, int> bestCouple(0,0);
        for (int i = 0; i < CompetitionSize; ++i) {
            // Take two random individuals
            int parent1 = Globals::getSingleton()->getRandom().getRandomWithinRange(0,int(lastIndex));
            int parent2 = Globals::getSingleton()->getRandom().getRandomWithinRange(0,int(lastIndex));
            if(parent1 == parent2) { i--; continue; }
            double coupleNovelty = 0.0;
            double parent1novelty = numeric_limits<double>::max();
            double parent2novelty = numeric_limits<double>::max();
            for(int i = 0; i < lastIndex; i++) // Get the minimum different these parents have within the specie population
            {
                if(i != parent1) parent1novelty = min(parent1novelty, currentIndividuals[parent1]->getSignatureDifference(currentIndividuals[i]));
                if(i != parent2) parent2novelty = min(parent2novelty, currentIndividuals[parent2]->getSignatureDifference(currentIndividuals[i]));
            }
            coupleNovelty = parent1novelty + parent2novelty;
            if(coupleNovelty > bestCoupleNovelty) // Choose the couple with the maximum novelty within the specie population
            {
                bestCoupleNovelty = coupleNovelty;
                bestCouple = std::make_pair(parent1, parent2);
            }
        }
        return bestCouple;
    }
    default:
        cerr << "ERROR : COMPETITION ERROR" << endl;
        exit(35);
    }
}
}
