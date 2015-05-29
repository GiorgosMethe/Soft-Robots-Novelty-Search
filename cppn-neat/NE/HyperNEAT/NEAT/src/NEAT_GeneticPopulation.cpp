#include "NEAT_Defines.h"

#include "NEAT_GeneticPopulation.h"

#include "NEAT_GeneticGeneration.h"

#ifdef EPLEX_INTERNAL
#include "NEAT_CoEvoGeneticGeneration.h"
#endif

#include "NEAT_GeneticIndividual.h"
#include "NEAT_Random.h"
#include <queue>

namespace NEAT
{

GeneticPopulation::GeneticPopulation() : onGeneration(0)
{
    BestEverFitnessBackUp = 0.0;
    generations.push_back(shared_ptr<GeneticGeneration>(new GeneticGeneration(0)));
}

GeneticPopulation::GeneticPopulation(string fileName)
    : onGeneration(-1)
{
    TiXmlDocument doc(fileName);

    bool loadStatus;

    if (iends_with(fileName,".gz"))
    {
        loadStatus = doc.LoadFileGZ();
    }
    else
    {
        loadStatus = doc.LoadFile();
    }

    if (!loadStatus)
    {
        throw CREATE_LOCATEDEXCEPTION_INFO("Error trying to load the XML file!");
    }

    TiXmlElement *root;

    root = doc.FirstChildElement();

    {
        TiXmlElement *generationElement = root->FirstChildElement("GeneticGeneration");

        while (generationElement)
        {
            generations.push_back(shared_ptr<GeneticGeneration>(new GeneticGeneration(generationElement)));

            generationElement = generationElement->NextSiblingElement("GeneticGeneration");
            onGeneration++;
        }
    }

    if (onGeneration<0)
    {
        throw CREATE_LOCATEDEXCEPTION_INFO("Tried to load a population with no generations!");
    }
    cout_ << "Loaded " << (onGeneration+1) << " Generations\n";

    adjustFitness();
}

#ifdef EPLEX_INTERNAL
GeneticPopulation::GeneticPopulation(shared_ptr<CoEvoExperiment> experiment)
    : onGeneration(0)
{
    if (experiment)
    {
        generations.push_back(
                    static_pointer_cast<GeneticGeneration>(
                        shared_ptr<CoEvoGeneticGeneration>(
                            new CoEvoGeneticGeneration(0,experiment)
                            )
                        )
                    );
    }
    else
    {
        generations.push_back(
                    shared_ptr<GeneticGeneration>(
                        new GeneticGeneration(0)
                        )
                    );
    }
}

GeneticPopulation::GeneticPopulation(
        string fileName,
        shared_ptr<CoEvoExperiment> experiment
        )
    : onGeneration(-1)
{
    TiXmlDocument doc(fileName);

    bool loadStatus;

    if (iends_with(fileName,".gz"))
    {
        loadStatus = doc.LoadFileGZ();
    }
    else
    {
        loadStatus = doc.LoadFile();
    }

    if (!loadStatus)
    {
        throw CREATE_LOCATEDEXCEPTION_INFO("Error trying to load the XML file!");
    }

    TiXmlElement *root;

    root = doc.FirstChildElement();

    /**NOTE**
        Do not mix GeneticGeneration objects with CoEvoGeneticGeneration objects in the same xml file.\
        */

    {
        TiXmlElement *generationElement = root->FirstChildElement("CoEvoGeneticGeneration");

        while (generationElement)
        {
            if (!experiment)
            {
                throw CREATE_LOCATEDEXCEPTION_INFO("ERROR: TRIED TO CREATE A COEVOLUTION WITHOUT AN EXPERIMENT!");
            }

            generations.push_back(shared_ptr<GeneticGeneration>(
                                      new CoEvoGeneticGeneration(generationElement,experiment))
                                  );
            generationElement = generationElement->NextSiblingElement("CoEvoGeneticGeneration");
            onGeneration++;
        }
    }

    if (onGeneration<0)
    {
        throw CREATE_LOCATEDEXCEPTION_INFO("Tried to load a population with no generations!");
    }

    adjustFitness();
}
#endif

GeneticPopulation::~GeneticPopulation()
{
    while (!species.empty())
        species.erase(species.begin());

    while (!extinctSpecies.empty())
        extinctSpecies.erase(extinctSpecies.begin());
}

int GeneticPopulation::getIndividualCount(int generation)
{
    if (generation==-1)
        generation=int(onGeneration);

    return generations[generation]->getIndividualCount();
}

shared_ptr<GeneticIndividual> GeneticPopulation::getIndividual(int individualIndex,int generation)
{
    //cout_ << a << ',' << generation << endl;

    if (generation==-1)
    {
        //cout_ << "NO GEN GIVEN: USING onGeneration\n";
        generation=int(onGeneration);
    }

    if (generation>=int(generations.size())||individualIndex>=generations[generation]->getIndividualCount())
    {
        cout_ << "GET_INDIVIDUAL: GENERATION OUT OF RANGE!\n";
        throw CREATE_LOCATEDEXCEPTION_INFO("GET_INDIVIDUAL: GENERATION OUT OF RANGE!\n");
    }

    return generations[generation]->getIndividual(individualIndex);
}

// Compute novelty for fitness based search
void GeneticPopulation::fitnessSearchNoveltyComputation()
{
    for (int a=0;a<generations[onGeneration]->getIndividualCount();a++)
    {
        // Individual
        double NoveltyValue = 0.0;
        shared_ptr<GeneticIndividual> individual = generations[onGeneration]->getIndividual(a);
        individual->isNovel(false); // Not novel
        individual->setFitnessBackUp(10e-5);
        if(!individual->behavior.validBehavior || !individual->isValid()) continue;

        if(novelIndividuals.size() == 0){ // Novel individual size is zero, so this individual is the first novel we found
            NoveltyValue = double(NEAT::Globals::getSingleton()->getParameterValue("NoveltyThreshold")); // Value to be assigned to the first individual entering the novel individuals' group
        }else{
            NoveltyValue = getIndividualNovelty(individual, generations[onGeneration]);
        }
        NoveltyValue += 10e-5; // to avoid having zero fitness
        individual->setFitnessBackUp(NoveltyValue);
        if(NoveltyValue > double(NEAT::Globals::getSingleton()->getParameterValue("NoveltyThreshold")))
        {
            individual->isNovel(true);
            novelIndividuals.push_back(individual);
        }
    }
    return;
}

// Compute novelty for novelty search
void GeneticPopulation::noveltySearchComputation()
{
    if(onGeneration+1 > int(NEAT::Globals::getSingleton()->getParameterValue("ForceCopyFitChampOptimizeAt")) &&
            int(NEAT::Globals::getSingleton()->getParameterValue("ForceCopyFitChampOptimizeAt")) != 0)
    {
        for (int a=0;a<generations[onGeneration]->getIndividualCount();a++)
        {
            // Individual
            double NoveltyValue = 0.0;
            shared_ptr<GeneticIndividual> individual = generations[onGeneration]->getIndividual(a);
            // Back up its original fitness before change it into novelty measure
            individual->setFitnessBackUp(individual->getFitness());
            individual->isNovel(false); // Not novel
            BestEverFitnessBackUp = std::max(BestEverFitnessBackUp, individual->getFitness());
        }
        return;
    }
    for (int a=0;a<generations[onGeneration]->getIndividualCount();a++)
    {
        // Individual
        double NoveltyValue = 0.0;
        shared_ptr<GeneticIndividual> individual = generations[onGeneration]->getIndividual(a);
        // Back up its original fitness before change it into novelty measure
        individual->setFitnessBackUp(individual->getFitness());
        individual->isNovel(false); // Not novel
        if(!individual->behavior.validBehavior || !individual->isValid()) continue;
        // Hold the best ever fitness
        BestEverFitnessBackUp = std::max(BestEverFitnessBackUp, individual->getFitness());

        if(novelIndividuals.size() == 0){ // Novel individual size is zero, so this individual is the first novel we found
            NoveltyValue = double(NEAT::Globals::getSingleton()->getParameterValue("NoveltyThreshold")); // Value to be assigned to the first individual entering the novel individuals' group
        }else{
            NoveltyValue = getIndividualNovelty(individual, generations[onGeneration]);
        }

        NoveltyValue += 10e-5; // to avoid having zero fitness
        if(NoveltyValue < 0.0 || NoveltyValue > numeric_limits<double>::max())
        {
            cerr << "ERROR : Novelty value is less than zero or greater than the maximum double value." << endl;
            exit(82);
        }

        individual->setFitness(NoveltyValue);

        // Just for experimentation
        if(bool(NEAT::Globals::getSingleton()->getParameterValue("Random")))
        {
            NoveltyValue = 0.0;
            individual->setFitness(0.01);
        }
        // end

        if(NoveltyValue > double(NEAT::Globals::getSingleton()->getParameterValue("NoveltyThreshold")))
        {
            individual->isNovel(true);
            novelIndividuals.push_back(individual);
        }
    }
    return;
}


void GeneticPopulation::computeBehaviorSparsity()
{
    shared_ptr<GeneticGeneration> generation = generations[onGeneration];

    double avgGenSparsity = 0.0;
    int numOfValidInd = 0;

    int k = int(NEAT::Globals::getSingleton()->getParameterValue("NoveltyDistanceKNNvalue"));

    std::priority_queue<double> sortedNovelIndividuals;

    for (int i=0;i<generation->getIndividualCount();i++)
    {
        shared_ptr<GeneticIndividual> individual = generation->getIndividual(i);
        if(!individual->isValid() || !individual->behavior.validBehavior) continue;
        for (int a=0;a<generation->getIndividualCount();a++) // Difference with all individuals in the same generation
        {
            if(generation->getIndividual(a) != individual && generation->getIndividual(a)->isValid() && generation->getIndividual(a)->behavior.validBehavior)
            {
                double tmp = individual->getSignatureDifference(generation->getIndividual(a));
                sortedNovelIndividuals.push(tmp);
                if(sortedNovelIndividuals.size() > k) sortedNovelIndividuals.pop();
            }
        }
        double SparsityMeasure = 0.0;
        int size = sortedNovelIndividuals.size();
        while(sortedNovelIndividuals.size() > 0){
            SparsityMeasure += (double) sortedNovelIndividuals.top();
            sortedNovelIndividuals.pop();
        }
        avgGenSparsity += SparsityMeasure /= (double) size;
        numOfValidInd++;
    }
    generation->sparsity = avgGenSparsity/numOfValidInd;
}

double GeneticPopulation::getIndividualNovelty(shared_ptr<GeneticIndividual> individual, shared_ptr<GeneticGeneration> generation)
{
    double NoveltyFitness= 0.0;
    // Keep only K smallest differences in behavior space
    std::priority_queue<double> sortedNovelIndividuals;
    int k = int(NEAT::Globals::getSingleton()->getParameterValue("NoveltyDistanceKNNvalue"));

    for(int i = 0; i < novelIndividuals.size(); i++){
        double tmp = individual->getSignatureDifference(novelIndividuals.at(i));
        sortedNovelIndividuals.push(tmp);
        if(sortedNovelIndividuals.size() > k) sortedNovelIndividuals.pop();
    }

    for (int a=0;a<generation->getIndividualCount();a++) // Difference with all individuals in the same generation
    {
        if(generation->getIndividual(a) != individual && generation->getIndividual(a)->isValid() && generation->getIndividual(a)->behavior.validBehavior)
        {
            double tmp = individual->getSignatureDifference(generation->getIndividual(a));
            sortedNovelIndividuals.push(tmp);
            if(sortedNovelIndividuals.size() > k) sortedNovelIndividuals.pop();
        }
    }

    int size = sortedNovelIndividuals.size();

    while(sortedNovelIndividuals.size() > 0){
        NoveltyFitness += (double) sortedNovelIndividuals.top();
        sortedNovelIndividuals.pop();
    }
    NoveltyFitness /= (double) size;

    return NoveltyFitness;
}

vector<shared_ptr<GeneticIndividual> >::iterator GeneticPopulation::getIndividualIterator(int a,int generation)
{
    if (generation==-1)
        generation=int(onGeneration);

    if (generation>=int(generations.size())||a>=generations[generation]->getIndividualCount())
    {
        throw CREATE_LOCATEDEXCEPTION_INFO("ERROR: Generation out of range!\n");
    }

    return generations[generation]->getIndividualIterator(a);
}

shared_ptr<GeneticIndividual> GeneticPopulation::getBestAllTimeIndividual()
{
    shared_ptr<GeneticIndividual> bestIndividual;

    for (int a=0;a<(int)generations.size();a++)
    {
        for (int b=0;b<generations[a]->getIndividualCount();b++)
        {
            shared_ptr<GeneticIndividual> individual = generations[a]->getIndividual(b);
            if (bestIndividual==NULL||bestIndividual->getFitness()<=individual->getFitness())
                bestIndividual = individual;
        }
    }
    return bestIndividual;
}

shared_ptr<GeneticIndividual> GeneticPopulation::getBestAllTimeIndividualFitnessBackUp()
{
    shared_ptr<GeneticIndividual> bestIndividual;

    for (int a=0;a<(int)generations.size();a++)
    {
        for (int b=0;b<generations[a]->getIndividualCount();b++)
        {
            shared_ptr<GeneticIndividual> individual = generations[a]->getIndividual(b);
            if (bestIndividual==NULL||bestIndividual->getFitnessBackUp()<=individual->getFitnessBackUp())
                bestIndividual = individual;
        }
    }
    return bestIndividual;
}

shared_ptr<GeneticIndividual> GeneticPopulation::getBestIndividualOfGeneration(int generation)
{
    shared_ptr<GeneticIndividual> bestIndividual;

    if (generation==-1)
        generation = int(generations.size())-1;

    for (int b=0;b<generations[generation]->getIndividualCount();b++)
    {
        shared_ptr<GeneticIndividual> individual = generations[generation]->getIndividual(b);
        if (bestIndividual==NULL||bestIndividual->getFitness()<individual->getFitness())
            bestIndividual = individual;
    }

    return bestIndividual;
}

void GeneticPopulation::speciate()
{
    double compatThreshold = Globals::getSingleton()->getParameterValue("CompatibilityThreshold");

    for (int a=0;a<generations[onGeneration]->getIndividualCount();a++)
    {
        shared_ptr<GeneticIndividual> individual = generations[onGeneration]->getIndividual(a);

        bool makeNewSpecies=true;
        // For each individual look if there is any species that has small difference with
        // If yes, assign the specie id to this individual
        for (int b=0;b<(int)species.size();b++)
        {
            double compatibility = species[b]->getBestIndividual()->getCompatibility(individual);
            if (compatibility<compatThreshold)
            {
                //Found a compatible species
                individual->setSpeciesID(species[b]->getID());
                makeNewSpecies=false;
                break;
            }
        }
        // If no, make a new specie
        if (makeNewSpecies)
        {
            //Make a new species.  The process of making a new species sets the ID for the individual.
            shared_ptr<GeneticSpecies> newSpecies(new GeneticSpecies(individual));
            species.push_back(newSpecies);
        }
    }

    int speciesTarget = int(Globals::getSingleton()->getParameterValue("SpeciesSizeTarget"));

    double compatMod;

    if ((int)species.size()<speciesTarget)
    {
        compatMod = -Globals::getSingleton()->getParameterValue("CompatibilityModifier");
    }
    else if ((int)species.size()>speciesTarget)
    {
        compatMod = +Globals::getSingleton()->getParameterValue("CompatibilityModifier");
    }
    else
    {
        compatMod=0.0;
    }

    if (compatThreshold<(fabs(compatMod)+0.3)&&compatMod<0.0)
    {
        //This is to keep the compatibility threshold from going ridiculusly small.
        if (compatThreshold<0.001)
            compatThreshold = 0.001;

        compatThreshold/=2.0;
    }
    else if (compatThreshold<(compatMod+0.3))
    {
        compatThreshold*=2.0;
    }
    else
    {
        compatThreshold+=compatMod;
    }
    Globals::getSingleton()->setParameterValue("CompatibilityThreshold",compatThreshold);
}

void GeneticPopulation::setSpeciesMultipliers()
{}

void GeneticPopulation::addIndividual(shared_ptr<GeneticIndividual> individual)
{
    generations[onGeneration]->addIndividual(individual);
}

void GeneticPopulation::adjustFitness()
{
    //This function sorts the individuals by fitness
    generations[onGeneration]->sortByFitness();

    speciate();

    for (int a=0;a<(int)species.size();a++)
    {
        species[a]->resetIndividuals();
    }

    //This function sorts the individuals by fitness
    generations[onGeneration]->sortByFitness();

    for (int a=0;a<generations[onGeneration]->getIndividualCount();a++)
    {
        shared_ptr<GeneticIndividual> individual = generations[onGeneration]->getIndividual(a);

        getSpecies(individual->getSpeciesID())->addIndividual(individual);
    }

    for (int a=0;a<(int)species.size();a++)
    {
        if (species[a]->getIndividualCount()==0)
        {
            extinctSpecies.push_back(species[a]);
            species.erase(species.begin()+a);
            a--;
        }
    }

    for (int a=0;a<(int)species.size();a++)
    {
        species[a]->setMultiplier();
        species[a]->setFitness();
        species[a]->incrementAge();
    }

    //This function sorts the individuals by fitness again, now that speciation has taken place.
    generations[onGeneration]->sortByFitness();
}

void GeneticPopulation::generationData()
{
    double totalIndividualFitness = 0.0;
    double totalIndividualFitnessBackUp = 0.0;

    for (int a=0;a<generations[onGeneration]->getIndividualCount();a++)
    {
        shared_ptr<GeneticIndividual> ind = generations[onGeneration]->getIndividual(a);
        totalIndividualFitness += ind->getFitness();
        totalIndividualFitnessBackUp += ind->getFitnessBackUp();
    }

    double averageFitness = totalIndividualFitness/generations[onGeneration]->getIndividualCount();
    double averageFitnessBackUp = totalIndividualFitnessBackUp/generations[onGeneration]->getIndividualCount();
    cout_ << "Generation "<<int(onGeneration)<<": "<<"overall_average = "<<averageFitness<<endl;
    cout_ << "Champion fitness: " << generations[onGeneration]->getIndividual(0)->getFitness() << endl;

    if(bool(NEAT::Globals::getSingleton()->getParameterValue("RecordStats")))
    {
        // Write statistics about current generation
        ofstream statsFile("stats.dat", std::ios_base::app);
        std::ostringstream output;
        if (statsFile.is_open())
        {
            double GenerationChampionFitness = generations[onGeneration]->getGenerationChampion()->getFitness();
            if (bool(NEAT::Globals::getSingleton()->getParameterValue("NoveltySearch")))
            {
                output << "Gen: "<< int(onGeneration+1)
                       << ", Avg. Nov: " << averageFitness
                       << ", Champ. Nov: " << GenerationChampionFitness
                       << ", Novel individuals: " <<  novelIndividuals.size()
                       << ", Avg. Fit: " << averageFitnessBackUp
                       << ", Best ever Fit: " << BestEverFitnessBackUp;

                if(bool(NEAT::Globals::getSingleton()->getParameterValue("ComputeSparsity")))
                    output << ", Sparsity: " <<  generations[onGeneration]->sparsity;

                statsFile << int(onGeneration+1)
                          << "," << averageFitness
                          << "," << GenerationChampionFitness
                          << "," <<  novelIndividuals.size()
                          << "," << averageFitnessBackUp
                          << "," << BestEverFitnessBackUp;
                if(bool(NEAT::Globals::getSingleton()->getParameterValue("ComputeSparsity"))) statsFile << "," <<  generations[onGeneration]->sparsity;
                statsFile << "\n";
            }
            else
            {
                output << "Gen: "<< int(onGeneration+1)
                       << ", Avg. Fit: " << averageFitness
                       << ", Champ. Fit: " << GenerationChampionFitness
                       << ", Best ever Fit: " << getBestAllTimeIndividual()->getFitness();

                if(bool(NEAT::Globals::getSingleton()->getParameterValue("FitnessNovelty")))
                    output << ", Novel individuals: " <<  novelIndividuals.size();

                if(bool(NEAT::Globals::getSingleton()->getParameterValue("ComputeSparsity")))
                    output << ", Sparsity: " <<  generations[onGeneration]->sparsity;

                statsFile << int(onGeneration+1)
                          << "," << averageFitness
                          << "," << GenerationChampionFitness
                          << "," << getBestAllTimeIndividual()->getFitness();

                if(bool(NEAT::Globals::getSingleton()->getParameterValue("FitnessNovelty"))) statsFile << "," <<  novelIndividuals.size();
                if(bool(NEAT::Globals::getSingleton()->getParameterValue("ComputeSparsity"))) statsFile << "," <<  generations[onGeneration]->sparsity;
                statsFile << "\n";
            }
            cout << output.str() << endl;
            statsFile.close();
        }
    }
    return;
}

void GeneticPopulation::produceNextGeneration()
{
#ifdef EPLEX_INTERNAL
    if (equals(generations[onGeneration]->getTypeName(),"CoEvoGeneticGeneration"))
    {
        //We are dealing with coevolution, update tests
        static_pointer_cast<CoEvoGeneticGeneration>(generations[onGeneration])->updateTests();
    }
#endif

    cout_ << "In produce next generation loop...\n";
    //This clears the link history so future links with the same toNode and fromNode will have different IDs
    Globals::getSingleton()->clearLinkHistory();
    //Take parents, all the individuals from current generation
    int numParents = int(generations[onGeneration]->getIndividualCount());
    //Check if fitness is below a threshold
    for(int a=0;a<numParents;a++)
    {
        if(generations[onGeneration]->getIndividual(a)->getFitness() < 1e-6)
        {
            throw CREATE_LOCATEDEXCEPTION_INFO("ERROR: Fitness must be a positive number!\n");
        }
    }

    double totalFitness=0;

    for (int a=0;a<(int)species.size();a++)
    {
        totalFitness += species[a]->getAdjustedFitness();
    }
    int totalOffspring=0;
    for (int a=0;a<(int)species.size();a++)
    {
        double adjustedFitness = species[a]->getAdjustedFitness();
        int offspring = int(adjustedFitness/totalFitness*numParents);
        totalOffspring+=offspring;
        species[a]->setOffspringCount(offspring);
    }
    //cout_ << "Pausing\n";
    //system("PAUSE");
    //Some offspring were truncated.  Give these to the best individuals
    while (totalOffspring<numParents)
    {
        for (int a=0;totalOffspring<numParents&&a<generations[onGeneration]->getIndividualCount();a++)
        {
            shared_ptr<GeneticIndividual> ind = generations[onGeneration]->getIndividual(a);
            shared_ptr<GeneticSpecies> gs = getSpecies(ind->getSpeciesID());
            gs->setOffspringCount(gs->getOffspringCount()+1);
            totalOffspring++;
        }
    }
    for (int a=0;a<(int)species.size();a++)
    {
        cout_ << "Species ID: " << species[a]->getID() << " Age: " << species[a]->getAge() << " last improv. age: " << species[a]->getAgeOfLastImprovement() << " Fitness: " << species[a]->getFitness() << "*" << species[a]->getMultiplier() << "=" << species[a]->getAdjustedFitness() <<  " Size: " << int(species[a]->getIndividualCount()) << " Offspring: " << int(species[a]->getOffspringCount()) << endl;
    }
    //This is the new generation
    vector<shared_ptr<GeneticIndividual> > babies;

    double totalIndividualFitness=0;
    // Initially all species cannot reproduce
    for (int a=0;a<(int)species.size();a++){species[a]->setReproduced(false);}

    int smallestSpeciesSizeWithElitism = int(Globals::getSingleton()->getParameterValue("SmallestSpeciesSizeWithElitism"));
    double mutateSpeciesChampionProbability = Globals::getSingleton()->getParameterValue("MutateSpeciesChampionProbability");
    // Probability of copying the best individual from the last generation
    bool forceCopyGenerationChampion = (Globals::getSingleton()->getParameterValue("ForceCopyGenerationChampion") > Globals::getSingleton()->getRandom().getRandomDouble());

    for (int a=0;a<generations[onGeneration]->getIndividualCount();a++)
    {
        //Go through and add the species champions
        shared_ptr<GeneticIndividual> ind = generations[onGeneration]->getIndividual(a);
        shared_ptr<GeneticSpecies> species = getSpecies(ind->getSpeciesID());
        if (!species->isReproduced())
        {
            species->setReproduced(true);
            //This is the first and best organism of this species to be added, so it's the species champion
            //of this generation
            if (ind->getFitness() > species->getBestIndividual()->getFitness())
            {
                //We have a new all-time species champion!
                species->setBestIndividual(ind);
                cout_ << "Species " << species->getID() << " has a new champ with fitness " << species->getBestIndividual()->getFitness() << endl;
            }
            if ((a==0 && forceCopyGenerationChampion) || (species->getOffspringCount() >= smallestSpeciesSizeWithElitism))
            {
                //Copy species champion.
                bool mutateChampion = false;
                if (Globals::getSingleton()->getRandom().getRandomDouble() < mutateSpeciesChampionProbability)
                    mutateChampion = true;

                babies.push_back(shared_ptr<GeneticIndividual>(new GeneticIndividual(ind,mutateChampion)));
                species->decrementOffspringCount();
            }
            if (a==0)
            {
                species->updateAgeOfLastImprovement();
            }
        }
        totalIndividualFitness += ind->getFitness();
    }


    if(bool(Globals::getSingleton()->getParameterValue("CopyGenerationChampionFitnessBackUp")))
    {
        // We get the best in the secondary objective function and copy it in the next generation
        shared_ptr<GeneticIndividual> bestInFitnessBackUp;
        double MaxFitnessBackUp = 0.0;
        for (int a=0;a<generations[onGeneration]->getIndividualCount();a++)
        {
            //Go through and add the species champions
            shared_ptr<GeneticIndividual> ind = generations[onGeneration]->getIndividual(a);
            if((ind->getFitnessBackUp() >= MaxFitnessBackUp) && ind->isValid() && getSpecies(ind->getSpeciesID())->getOffspringCount() > 0)
            {
                MaxFitnessBackUp = ind->getFitnessBackUp();
                bestInFitnessBackUp = ind;
            }
        }
        shared_ptr<GeneticSpecies> specie = getSpecies(bestInFitnessBackUp->getSpeciesID());
        bool mutateChampion = false;
        if (Globals::getSingleton()->getRandom().getRandomDouble() < mutateSpeciesChampionProbability)
            mutateChampion = true;
        babies.push_back(shared_ptr<GeneticIndividual>(new GeneticIndividual(bestInFitnessBackUp,mutateChampion)));
        specie->decrementOffspringCount();
        specie->setReproduced(true);

        // Go all over current species and copy their champion if their size is more than species with elitism
        for (int a=0;a<(int)species.size();a++)
        {
            shared_ptr<GeneticSpecies> specie = species[a];
            if(specie->getOffspringCount() >= smallestSpeciesSizeWithElitism && !specie->isReproduced())
            {
                shared_ptr<GeneticIndividual> bestInFitnessBackUp;
                double MaxFitnessBackUp = 0.0;
                for(int i = 0; i < specie->getIndividualCount(); i++)
                {
                    shared_ptr<GeneticIndividual> ind = specie->getIndividualAt(i);
                    if((ind->getFitnessBackUp() >= MaxFitnessBackUp) && ind->isValid() && getSpecies(ind->getSpeciesID())->getOffspringCount() > 0)
                    {
                        MaxFitnessBackUp = ind->getFitnessBackUp();
                        bestInFitnessBackUp = ind;
                    }
                }
                bool mutateChampion = false;
                if (Globals::getSingleton()->getRandom().getRandomDouble() < mutateSpeciesChampionProbability)
                    mutateChampion = true;
                babies.push_back(shared_ptr<GeneticIndividual>(new GeneticIndividual(bestInFitnessBackUp,mutateChampion)));
                specie->decrementOffspringCount();
                specie->setReproduced(true);
            }
        }
    }

    if (generations[onGeneration]->getIndividual(0)->getUserData().length())
    {
        cout_ << "Champion data: " << generations[onGeneration]->getIndividual(0)->getUserData() << endl;
    }
    cout_ << "# of Species: " << int(species.size()) << endl;
    cout_ << "compat threshold: " << Globals::getSingleton()->getParameterValue("CompatibilityThreshold") << endl;

    for (int a=0;a<(int)species.size();a++)
    {
        //cout_ << "Making babies\n";
        species[a]->makeBabies(babies);
    }

    if ((int)babies.size()!=generations[onGeneration]->getIndividualCount())
    {
        cout_ << "Population size changed!\n";
        throw CREATE_LOCATEDEXCEPTION_INFO("Population size changed!");
    }

    // If the evolution changes from novelty to fitness, we have to copy mutations of the allTimeChampion in the current generation
    if(onGeneration+1 == int(NEAT::Globals::getSingleton()->getParameterValue("ForceCopyFitChampOptimizeAt")) &&
            int(NEAT::Globals::getSingleton()->getParameterValue("ForceCopyFitChampOptimizeAt")) != 0)
    {
        shared_ptr<GeneticIndividual> tmp = getBestAllTimeIndividualFitnessBackUp();
        cout_ << "Copying champion with fitness: " << tmp->getFitnessBackUp() << endl;
        int numberOfBabies = babies.size();
        babies.clear();
        for(int i=0; i < numberOfBabies; i++)
        {
            babies.push_back(shared_ptr<GeneticIndividual>(new GeneticIndividual(tmp,false)));
        }
        cout_ << "...done" << endl;
    }

    //cout_ << "Making new generation\n";
    shared_ptr<GeneticGeneration> newGeneration(generations[onGeneration]->produceNextGeneration(babies,onGeneration+1));
    //cout_ << "Done Making new generation!\n";
    generations.push_back(newGeneration);
    onGeneration++;
}


void GeneticPopulation::dump(string filename,bool includeGenes,bool doGZ)
{
    TiXmlDocument doc( filename );

    TiXmlElement *root = new TiXmlElement("Genetics");

    Globals::getSingleton()->dump(root);

    doc.LinkEndChild(root);

    for (int a=0;a<(int)generations.size();a++)
    {

        TiXmlElement *generationElementPtr = new TiXmlElement(generations[a]->getTypeName());

        root->LinkEndChild(generationElementPtr);

        generations[a]->dump(generationElementPtr,includeGenes);
    }

    if (doGZ)
    {
        doc.SaveFileGZ();
    }
    else
    {
        doc.SaveFile();
    }
}

void GeneticPopulation::dumpBest(string filename,bool includeGenes,bool doGZ)
{
    TiXmlDocument doc( filename );

    TiXmlElement *root = new TiXmlElement("Genetics");

    Globals::getSingleton()->dump(root);

    doc.LinkEndChild(root);

    for (int a=0;a<int(generations.size())-1;a++)
    {

        TiXmlElement *generationElementPtr = new TiXmlElement(generations[a]->getTypeName());

        root->LinkEndChild(generationElementPtr);

        generations[a]->dumpBest(generationElementPtr,includeGenes);
    }

    if (generations.size())
    {
        //Always dump everyone from the final generation
        TiXmlElement *generationElementPtr = new TiXmlElement(generations[generations.size()-1]->getTypeName());
        generations[generations.size()-1]->dump(generationElementPtr,includeGenes);
        root->LinkEndChild(generationElementPtr);
    }

    if (doGZ)
    {
        doc.SaveFileGZ();
    }
    else
    {
        doc.SaveFile();
    }
}

void GeneticPopulation::cleanupOld(int generationSkip)
{
    for (int a=0;a<onGeneration;a++)
    {
        if ( (a%generationSkip) == 0 )
            continue;

        generations[a]->cleanup();
    }
}
}
