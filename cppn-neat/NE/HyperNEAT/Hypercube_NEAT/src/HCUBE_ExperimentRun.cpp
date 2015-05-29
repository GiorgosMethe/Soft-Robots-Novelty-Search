#include "HCUBE_Defines.h"

#include "HCUBE_ExperimentRun.h"

#include "Experiments/HCUBE_Experiment.h"
#include "Experiments/HCUBE_XorExperiment.h"
#include "Experiments/HCUBE_VoxbotsExperiment.h"

#ifndef HCUBE_NOGUI
#include "HCUBE_MainFrame.h"
#include "HCUBE_UserEvaluationFrame.h"

#include "HCUBE_ExperimentPanel.h"
#endif
#include "HCUBE_EvaluationSet.h"

namespace HCUBE
{
ExperimentRun::ExperimentRun()
    :
      running(false),
      started(false),
      cleanup(false),
      populationMutex(new mutex()),
      frame(NULL)
{
}

ExperimentRun::~ExperimentRun()
{
    delete populationMutex;
}

void ExperimentRun::setupExperiment(
        int _experimentType,
        string _outputFileName
        )
{
    experimentType = _experimentType;
    outputFileName = _outputFileName;

    cout_ << "SETTING UP EXPERIMENT TYPE: " << experimentType << endl;

    for(int a=0;a<NUM_THREADS;a++)
    {
        switch (experimentType)
        {
        case EXPERIMENT_XOR:
            experiments.push_back(shared_ptr<Experiment>(new XorExperiment("",a)));
            break;
        case EXPERIMENT_SOFTBOT_CPPN_NEAT:
            experiments.push_back(shared_ptr<Experiment>(new SoftBotExperiment("",a)));
            break;
        default:
            cout_ << string("ERROR: Unknown Experiment Type!\n");
            throw CREATE_LOCATEDEXCEPTION_INFO("ERROR: Unknown Experiment Type!");
        }

    }

}

void ExperimentRun::createPopulation(string populationString)
{
    if (iequals(populationString,""))
    {
        int popSize = (int)NEAT::Globals::getSingleton()->getParameterValue("PopulationSize");
        population = shared_ptr<NEAT::GeneticPopulation>(
                    experiments[0]->createInitialPopulation(popSize)
                );
    }
    else
    {
#ifdef EPLEX_INTERNAL
        try
        {
            if (dynamic_cast<NEAT::CoEvoExperiment*>(experiments[0].get()))
            {
                population = shared_ptr<NEAT::GeneticPopulation>(
                            new NEAT::GeneticPopulation(
                                populationString,
                                shared_ptr<NEAT::CoEvoExperiment>((NEAT::CoEvoExperiment*)experiments[0]->clone())
                            )
                        );
                return;
            }
        }
        catch (const std::exception &ex)
        {
            throw CREATE_LOCATEDEXCEPTION_INFO(string("EXCEPTION ON DYNAMIC CAST: ")+string(ex.what()));
        }
#endif

        population = shared_ptr<NEAT::GeneticPopulation>(
                    new NEAT::GeneticPopulation(populationString)
                    );
    }
}

void ExperimentRun::setupExperimentInProgress(
        string populationFileName,
        string _outputFileName
        )
{
    outputFileName = _outputFileName;

    {
        TiXmlDocument doc(populationFileName);

        bool loadStatus;

        if (iends_with(populationFileName,".gz"))
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

        TiXmlElement *element = doc.FirstChildElement();

        NEAT::Globals* globals = NEAT::Globals::init(element);

        //Destroy the document
    }

    int experimentType = int(NEAT::Globals::getSingleton()->getParameterValue("ExperimentType")+0.001);

    cout_ << "Loading Experiment: " << experimentType << endl;

    setupExperiment(experimentType,_outputFileName);

    cout_ << "Experiment set up.  Creating population...\n";

    createPopulation(populationFileName);

    cout_ << "Population Created\n";
}

void ExperimentRun::start()
{
    cout_ << "Experiment started\n";
#ifndef _DEBUG
    try
    {
#endif
        int maxGenerations = int(NEAT::Globals::getSingleton()->getParameterValue("MaxGenerations"));

        started=running=true;

        int firstGen = (population->getGenerationCount()-1);
        for (int generations=firstGen;generations<maxGenerations;generations++)
        {
            if (generations>firstGen)
            {
                //Even if we are loading an existing population,
                //Re-evaluate all of the individuals
                //as a sanity check
                mutex::scoped_lock scoped_lock(*populationMutex);
                cout_ << "PRODUCING NEXT GENERATION\n";
                produceNextGeneration();
                cout_ << "DONE PRODUCING\n";
            }

            if (experiments[0]->performUserEvaluations())
            {
#ifdef HCUBE_NOGUI
                throw CREATE_LOCATEDEXCEPTION_INFO("ERROR: TRIED TO USE INTERACTIVE EVOLUTION WITH NO GUI!");
#else
                frame->getUserEvaluationFrame()->updateEvaluationPanels();
                running=false;
                while (!running)
                {
                    boost::xtime xt;
#if BOOST_VERSION > 105000
                    boost::xtime_get(&xt, boost::TIME_UTC_);
#else
                    boost::xtime_get(&xt, boost::TIME_UTC);
#endif
                    xt.sec += 1;
                    boost::thread::sleep(xt); // Sleep for 1/2 second
                    //cout_ << "Sleeping while user evaluates!\n";
                }
#endif
            }
            else
            {
                while (!running)
                {
                    boost::xtime xt;
#if BOOST_VERSION > 105000
                    boost::xtime_get(&xt, boost::TIME_UTC_);
#else
                    boost::xtime_get(&xt, boost::TIME_UTC);
#endif
                    xt.sec += 1;
                    boost::thread::sleep(xt); // Sleep for 1/2 second
                }
                preprocessPopulation();
                evaluatePopulation();
            }

            cout_ << "Finishing evaluations\n";
            finishEvaluations();
            cout_ << "Evaluations Finished\n";
        }
        cout_ << "Experiment finished\n";

        cout_ << "Saving best individuals...";
        string bestFileName = outputFileName.substr(0,outputFileName.length()-4)+string("_best.xml");
        population->dumpBest(bestFileName,true,true);
        cout_ << "Done!\n";

        cout_ << "Deleting backup file...";
        boost::filesystem::remove(outputFileName+string(".backup.xml.gz"));
        cout_ << "Done!\n";

#ifndef _DEBUG
    }
    catch (const std::exception &ex)
    {
        cout_ << "CAUGHT ERROR AT " << __FILE__ << " : " << __LINE__ << endl;
        CREATE_PAUSE(ex.what());
    }
    catch (...)
    {
        cout_ << "CAUGHT ERROR AT " << __FILE__ << " : " << __LINE__ << endl;
        CREATE_PAUSE("AN UNKNOWN EXCEPTION HAS BEEN THROWN!");
    }
#endif
}

void ExperimentRun::preprocessPopulation()
{
    cout_ << "Preprocessing population\n";
    shared_ptr<NEAT::GeneticGeneration> generation = population->getGeneration();

    for(int a=0;a<generation->getIndividualCount();a++)
    {
        experiments[0]->preprocessIndividual(generation,generation->getIndividual(a));
    }
}

void ExperimentRun::evaluatePopulation()
{
    shared_ptr<NEAT::GeneticGeneration> generation = population->getGeneration();
    //Randomize population order for evaluation
    generation->randomizeIndividualOrder();

    int populationSize = population->getIndividualCount();
    cout_ << "Processing group..." << endl;
    if(NUM_THREADS==1)
    {
        //Bypass the threading logic for a single thread

        EvaluationSet evalSet(
                    experiments[0],
                generation,
                population->getIndividualIterator(0),
                populationSize
                );
        evalSet.run();
    }
    else
    {
        int populationPerProcess = populationSize/NUM_THREADS;

        boost::thread** threads = new boost::thread*[NUM_THREADS];
        EvaluationSet** evaluationSets = new EvaluationSet*[NUM_THREADS];

        for (int i = 0; i < NUM_THREADS; ++i)
        {
            if (i+1==NUM_THREADS)
            {
                //Fix for uneven distribution
                int populationIteratorSize =
                        populationSize
                        - populationPerProcess*(NUM_THREADS-1);
                evaluationSets[i] =
                        new EvaluationSet(
                            experiments[i],
                            generation,
                            population->getIndividualIterator(populationPerProcess*i),
                            populationIteratorSize
                            );
            }
            else
            {

                evaluationSets[i] =
                        new EvaluationSet(
                            experiments[i],
                            generation,
                            population->getIndividualIterator(populationPerProcess*i),
                            populationPerProcess
                            );
            }

            threads[i] =
                    new boost::thread(
                        boost::bind(
                            &EvaluationSet::run,
                            evaluationSets[i]
                            )
                        );
        }

        //loop through each thread, making sure it is finished before we move on
        for (int i=0;i<NUM_THREADS;++i)
        {
            /*if (!evaluationSets[i]->isFinished())
                  {
                  --i;
                  boost::xtime xt;
                  boost::xtime_get(&xt, boost::TIME_UTC);
                  xt.sec += 1;
                  boost::thread::sleep(xt); // Sleep for 1/2 second
                  }*/
            threads[i]->join();
        }

        for (int i = 0; i < NUM_THREADS; ++i)
        {
            delete threads[i];
            delete evaluationSets[i];
        }

        delete[] threads;
        delete[] evaluationSets;
    }
    cout_ << "Done processing group..." << endl;
}

void ExperimentRun::finishEvaluations()
{
    if(bool(NEAT::Globals::getSingleton()->getParameterValue("NoveltySearch")))
    {
        cout_ << "Novelty search..." << endl;
        population->noveltySearchComputation();
        cout_ << "Done" << endl;
    }

    if(bool(NEAT::Globals::getSingleton()->getParameterValue("FitnessNovelty")) && !bool(NEAT::Globals::getSingleton()->getParameterValue("NoveltySearch")))
    {
        cout_ << "Novel individuals for fitness based..." << endl;
        population->fitnessSearchNoveltyComputation();
        cout_ << "Done" << endl;
    }

    if(bool(NEAT::Globals::getSingleton()->getParameterValue("ComputeSparsity")) &&
            (bool(NEAT::Globals::getSingleton()->getParameterValue("FitnessNovelty")) ||
             bool(NEAT::Globals::getSingleton()->getParameterValue("NoveltySearch"))))
    {
        population->computeBehaviorSparsity();
    }

    // prints stats and writes them in a file
    population->generationData();

    if(bool(NEAT::Globals::getSingleton()->getParameterValue("NoveltySearch")) && bool(NEAT::Globals::getSingleton()->getParameterValue("StoreNovelIndividuals")))
    {
        if(!boost::filesystem::exists("NovelIndividuals"))
        {
            boost::filesystem::path dir("NovelIndividuals");
            boost::filesystem::create_directory(dir);
        }
        cout_ << "Storing novel individuals\n";
        shared_ptr<NEAT::GeneticGeneration> generation = population->getGeneration();
        for(int a=0;a<generation->getIndividualCount();a++)
        {
            shared_ptr<NEAT::GeneticIndividual> tmpInd = generation->getIndividual(a);
            std::vector<std::string> outNames;
            if(tmpInd->isNovel())
            {
                char buffer1 [50]; sprintf(buffer1, "%04i", generation->getGenerationNumber());
                char nov[100]; sprintf(nov, "%.8lf", tmpInd->getFitness());
                char fit[100]; sprintf(fit, "%.8lf", tmpInd->getFitnessBackUp());
                std::ostringstream tmp;
                tmp << "Softbot" << "--Gen_" << buffer1 << "--Ind_" << tmpInd << "--Nov_" << nov << "--Fit_" << fit;
                string individualID = tmp.str();
                tmpInd->behavior.writeSignature(individualID, ".csv", false, outNames);
                std::ostringstream moveSigFileCmd;
                moveSigFileCmd << "mv " << individualID << ".csv " << "NovelIndividuals/";
                std::system(moveSigFileCmd.str().c_str());
            }
        }
        cout_ << "Storing novel individuals finished\n";
    }

    if(bool(NEAT::Globals::getSingleton()->getParameterValue("FitnessNovelty")) && bool(NEAT::Globals::getSingleton()->getParameterValue("FitnessStoreNovelIndividuals")))
    {
        if(!boost::filesystem::exists("NovelIndividuals"))
        {
            boost::filesystem::path dir("NovelIndividuals");
            boost::filesystem::create_directory(dir);
        }
        cout_ << "Storing novel individuals\n";
        shared_ptr<NEAT::GeneticGeneration> generation = population->getGeneration();
        for(int a=0;a<generation->getIndividualCount();a++)
        {
            shared_ptr<NEAT::GeneticIndividual> tmpInd = generation->getIndividual(a);
            std::vector<std::string> outNames;
            if(tmpInd->isNovel())
            {
                char buffer1 [50]; sprintf(buffer1, "%04i", generation->getGenerationNumber());
                char fit[100]; sprintf(fit, "%.8lf", tmpInd->getFitness());
                std::ostringstream tmp;
                tmp << "Softbot" << "--Gen_" << buffer1 << "--Ind_" << tmpInd << "--Fit_" << fit;
                string individualID = tmp.str();
                tmpInd->behavior.writeSignature(individualID, ".csv", false, outNames);
                std::ostringstream moveSigFileCmd;
                moveSigFileCmd << "mv " << individualID << ".csv " << "NovelIndividuals/";
                std::system(moveSigFileCmd.str().c_str());
            }
        }
        cout_ << "Storing novel individuals finished\n";
    }

    if(bool(NEAT::Globals::getSingleton()->getParameterValue("StoreBestIndividuals")))
    {
        if(!boost::filesystem::exists("BestIndividuals"))
        {
            boost::filesystem::path dir("BestIndividuals");
            boost::filesystem::create_directory(dir);
        }
        cout_ << "Storing best individuals\n";
        shared_ptr<NEAT::GeneticGeneration> generation = population->getGeneration();
        shared_ptr<NEAT::GeneticIndividual> GenerationBestIndividual;


        if(bool(NEAT::Globals::getSingleton()->getParameterValue("NoveltySearch")))
            GenerationBestIndividual = generation->getGenerationChampionFitnessBackUp();
        else
            GenerationBestIndividual = generation->getGenerationChampion();


        if(GenerationBestIndividual->isValid())
        {
            char buffer1 [50]; sprintf(buffer1, "%04i", generation->getGenerationNumber());
            char fit[100];
            char orFit[100];

            if(bool(NEAT::Globals::getSingleton()->getParameterValue("NoveltySearch")))
                sprintf(fit, "%.8lf", GenerationBestIndividual->getFitnessBackUp());
            else
                sprintf(fit, "%.8lf", GenerationBestIndividual->getFitness());

            sprintf(orFit, "%.8lf", GenerationBestIndividual->originalFitness);
            std::ostringstream tmp;
            tmp << "Softbot" << "--Fit_" << fit << "--orFit_" << orFit << "--Gen_" << buffer1 << "--Ind_" << GenerationBestIndividual << ".vxa";
            string individualID = tmp.str();

            int num_x_voxels = uint(NEAT::Globals::getSingleton()->getParameterValue("NumVoxelsX"));
            int num_y_voxels = uint(NEAT::Globals::getSingleton()->getParameterValue("NumVoxelsY"));
            int num_z_voxels = uint(NEAT::Globals::getSingleton()->getParameterValue("NumVoxelsZ"));

            VoxBotCreator vxbc;
            vxbc.LoadVXASimulation("../../../../SimFiles/default/default.vxa");
            if(vxbc.getXDim() != num_x_voxels || vxbc.getYDim() != num_y_voxels || vxbc.getZDim() != num_z_voxels)
                vxbc.resizeStructure(num_x_voxels, num_y_voxels, num_z_voxels);
            vxbc.clearStructure();
            vxbc.InsertVoxBot(GenerationBestIndividual->VoxBotSimFile);
            if(bool(NEAT::Globals::getSingleton()->getParameterValue("EvolveMaterialProperties"))){
                for (int matIndex = 0; matIndex < vxbc.GetNumMaterials(); ++matIndex)
                {
                    vxbc.setMatProperties(matIndex+1, GenerationBestIndividual->MaterialProperties[matIndex][0], GenerationBestIndividual->MaterialProperties[matIndex][1], GenerationBestIndividual->MaterialProperties[matIndex][2], GenerationBestIndividual->MaterialProperties[matIndex][3]);
                }
            }
            vxbc.writeVXASimulation(individualID.c_str());

            ostringstream moveBestIndividual;
            moveBestIndividual << "mv " << individualID.c_str() << " BestIndividuals/";
            std::system(moveBestIndividual.str().c_str());
        }


        cout_ << "Storing best individuals finished\n";
    }

    // Clear vectors with structures
    shared_ptr<NEAT::GeneticGeneration> generation = population->getGeneration();
    for(int a=0;a<generation->getIndividualCount();a++)
    {
        generation->getIndividual(a)->VoxBotSimFile.clear();
    }

    cout_ << "Adjusting fitness...\n";
    population->adjustFitness(); // Does not change individuals' fitnesses, only species fitnesses
    cout_ << "Cleaning up...\n";

    if (cleanup)
        population->cleanupOld(INT_MAX/2);
    cout_ << "Dumping best individuals...\n";
    population->dumpBest(outputFileName+string(".backup.xml"),true,true);
#ifndef HCUBE_NOGUI
    if (frame)
    {
        frame->updateNumGenerations(population->getGenerationCount());
    }
#endif

    cout_ << "Resetting generation data...\n";
    experiments[0]->resetGenerationData(generation);

    for (int a=0;a<population->getIndividualCount();a++)
    {
        //cout_ << __FILE__ << ":" << __LINE__ << endl;
        experiments[0]->addGenerationData(generation,population->getIndividual(a));
    }
}

void ExperimentRun::produceNextGeneration()
{
    cout_ << "Producing next generation.\n";
    try
    {
        population->produceNextGeneration();
    }
    catch (const std::exception &ex)
    {
        cout_ << "EXCEPTION DURING POPULATION REPRODUCTION: " << endl;
        CREATE_PAUSE(ex.what());
    }
    catch (...)
    {
        cout_ << "Unhandled Exception\n";
    }
}
}
