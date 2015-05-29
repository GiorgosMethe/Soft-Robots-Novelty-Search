/* ----------------------------------------------------------------------------
 *
  GaDirectEncoding.cpp
---------------------------------------------------------------------------- */
#include <QDir>
#include <QCoreApplication>
#include <stdio.h>
#include <ga/ga.h>
#include <ga/std_stream.h>
#include "PetriDish/PetriDish.h" // for multithreading
#define INSTANTIATE_REAL_GENOME
#include "VoxBotCreator.h" // to create robots
#include <ga/GARealGenome.h>
#include <ga/GA2DArrayGenome.h>
#include <ga/GARealGenome.h>
#include <iostream>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <fstream>
#include <string>
#include <queue>
#include <list>
#include <vector>
#include <math.h>
#include <tinyxml.h>

#define GA_CLASS GASteadyStateGA

using namespace std;

int num_x_voxels;
int num_y_voxels;
int num_z_voxels;
int length;
int matNum;
int indNum;
float Objective(GAGenome &);

// Functions taken from: http://datadryad.org/resource/doi:10.5061/dryad.m3s84
int leftExists(std::vector<int> ArrayForVoxelyze, int currentPosition);
int rightExists(std::vector<int> ArrayForVoxelyze, int currentPosition);
int upExists(std::vector<int> ArrayForVoxelyze, int currentPosition);
int downExists(std::vector<int> ArrayForVoxelyze, int currentPosition);
int forwardExists(std::vector<int> ArrayForVoxelyze, int currentPosition);
int backExists(std::vector<int> ArrayForVoxelyze, int currentPosition);
std::vector<int> makeOneShapeOnly(std::vector<int> ArrayForVoxelyze);
std::vector<int> createArrayForVoxelyze(std::vector<float> ContinuousArray , std::vector<std::vector<float> > ContinuousArrayMaterial);
// ...End functions taken from the original work

//
// parse xml and get fitness, penalty, behavior
//
void readVFOfitnessValue(std::string filename, bool &valid, int &fitnessType, double &fitness, int &penaltyType, double &penalty)
{
    TiXmlDocument doc( filename );
    doc.LoadFile();
    TiXmlElement* root = doc.FirstChildElement();
    if(root == NULL){std::cerr << "Failed to load file: No root element." << std::endl;doc.Clear(); exit(45);}
    for(TiXmlElement* elem = root->FirstChildElement(); elem != NULL; elem = elem->NextSiblingElement()){
        std::string elemName = elem->Value();
        if(elemName == "Valid")
        {
            istringstream buffer(elem->GetText()); buffer >> valid;
        }
        else if(elemName == "Fitness")
        {
            TiXmlElement* tmp = elem->FirstChildElement();
            for(TiXmlElement* FitnessElement = tmp; FitnessElement != NULL; FitnessElement = FitnessElement->NextSiblingElement())
            {
                std::string FitnessElementName = FitnessElement->Value();
                if(FitnessElementName == "FitnessType") fitnessType = std::atoi(FitnessElement->GetText());
                else if(FitnessElementName == "Value") fitness = std::atof(FitnessElement->GetText());
            }
        }
        else if(elemName == "Penalty")
        {
            TiXmlElement* tmp = elem->FirstChildElement();
            for(TiXmlElement* PenaltyElement = tmp; PenaltyElement != NULL; PenaltyElement = PenaltyElement->NextSiblingElement())
            {
                std::string PenaltyElementName = PenaltyElement->Value();
                if(PenaltyElementName == "PenaltyType") penaltyType = std::atoi(PenaltyElement->GetText());
                else if(PenaltyElementName == "Value") penalty = std::atof(PenaltyElement->GetText());
            }
        }
    }
}

int main(int argc, char** argv)
{
    // See if we've been given a seed to use (for testing purposes).  When you
    // specify a random seed, the evolution will be exactly the same each time
    // you use that seed number.
    unsigned int seed = 0;
    for(int ii=1; ii<argc; ii++) {
        if(strcmp(argv[ii++],"seed") == 0) {
            seed = atoi(argv[ii]);
        }
    }
    // First make a bunch of genomes.  We'll use each one in turn for a genetic
    // algorithm later on.  Each one illustrates a different method of using the
    // allele set object.  Each has its own objective function.
    VoxBotCreator creator;
    // Loading default simulation we are based on...
    std::cout << "Loading default simulation" << std::endl;
    creator.LoadVXASimulation("../../SimFiles/default/default.vxa");
    num_x_voxels = creator.getXDim();
    num_y_voxels = creator.getYDim();
    num_z_voxels = creator.getZDim();

    length = num_x_voxels*num_x_voxels*num_x_voxels;
    matNum = creator.GetNumMaterials() - 1;

    GARealAlleleSet alleles2(0, 1);
    GARealGenome genome2(length * (matNum + 1), alleles2, Objective);

    GAParameterList params;
    GASteadyStateGA::registerDefaultParameters(params);
    params.set(gaNnGenerations, 1000);
    params.set(gaNpopulationSize, 30);
    params.set(gaNscoreFrequency, 1);
    params.set(gaNpMutation, 0.001);
    params.set(gaNpCrossover, 0);
    params.set(gaNflushFrequency, 1);	// how often to dump scores to file
    params.set(gaNselectScores, (int)GAStatistics::AllScores);
    params.parse(argc, argv, gaTrue);

    string call = "mkdir softBots";
    system(call.c_str());

    // Now do a genetic algorithm for each one of the genomes that we created.
    PetriDish ga2(genome2);
    ga2.parameters(params);
    ga2.set(gaNscoreFilename, "stats.dat");
    cout << "\nStarting Direct Encoding Softbots..." << endl << endl;
    ga2.evolve();
    cout << "the ga generated:\n" << ga2.statistics().bestIndividual() << endl;
    cout << endl << "GA stats: " << ga2.statistics() << endl;

    std::ofstream myfile;
    myfile.open ("winner.txt");
    myfile << ga2.statistics().bestIndividual();
    myfile.close();
    return 0;
}

float Objective(GAGenome& g)
{
    GARealGenome& genome = (GARealGenome&)g;

    std::vector<float> ContinuousArray (length,0.0);
    std::vector<std::vector<float> > MaterialArray;
#ifdef SINGLE_GENOME_LENGTH
    for(int i=0; i<length; i++) {
        ContinuousArray[i] = genome.gene(i);
    }
#else
    for(int i=0; i<length; i++)
    {
        ContinuousArray[i] = genome.gene(i);
        std::vector<float> matTmp;
        for (int m = 0; m < matNum; ++m)
        {
            matTmp.push_back(genome.gene(i + length * (m+1)));
        }
        MaterialArray.push_back(matTmp);
    }
#endif

    stringstream ss;
    ss << pthread_self();

    string individualID =  ss.str() + "_test.vxa";
    VoxBotCreator creator;
    creator.LoadVXASimulation("../../SimFiles/default/default.vxa");
    vector<int> structure = createArrayForVoxelyze(ContinuousArray, MaterialArray);
    creator.clearStructure();
    creator.InsertVoxBot(structure);
    creator.writeVXASimulation(individualID);
    string call = "./../../VoxSim/build/VoxSim -f 2 -fp 0 -i " + individualID + " -o .";
    system(call.c_str());
    std::string outFitness = ss.str() + "_test.vfo";
    double penalty, fitness; bool valid; int fitnessType, penaltyType;
    readVFOfitnessValue(outFitness, valid, fitnessType, fitness, penaltyType, penalty);
    if(!valid) fitness = 0.0;
    else fitness = ( 1.0 - penalty ) * fitness;
    stringstream sss;
    sss << fitness;
    string call1 = "mv " + individualID + " " + "softBots/" + sss.str() + "--" + individualID;
    system(call1.c_str());
    QFile::remove(individualID.c_str());
    QFile::remove(outFitness.c_str());
    return fitness;
}

std::vector<int> createArrayForVoxelyze(std::vector<float> ContinuousArray ,std::vector<std::vector<float> > ContinuousArrayMaterial)
{
    std::vector<int> ArrayForVoxSim (length,0.0); //std::vector denoting type of material
    int v = 0;

    for (int z=0; z<num_z_voxels; z++)
    {
        for (int y=0; y<num_y_voxels; y++)
        {
            for (int x=0; x<num_x_voxels; x++)
            {
                if(ContinuousArray[v] > 0.5)
                {
                    int materialSelected = -1;
                    for (int matIndex = 0; matIndex < matNum; ++matIndex)
                    {
                        int greaterThanNum = 0;
                        for (int matIndexComp = 0; matIndexComp < matNum; ++matIndexComp)
                        {
                            if(matIndex != matIndexComp && ContinuousArrayMaterial[v][matIndex] > ContinuousArrayMaterial[v][matIndexComp])
                                greaterThanNum++;
                        }
                        if(greaterThanNum == matNum - 1) // this material has the largest value of all
                        {
                            materialSelected = matIndex;
                            break;
                        }
                    }
                    // +1 because material ID starts from 1
                    ArrayForVoxSim[v] = materialSelected + 1;
                }
                else
                {
                    ArrayForVoxSim[v] = 0;
                }
                v++;
            }
        }
    }
    ArrayForVoxSim = makeOneShapeOnly(ArrayForVoxSim);
    //    for (int i = 0; i < ArrayForVoxSim.size(); ++i) {
    //        cout << ArrayForVoxSim.at(i) ;
    //    }
    //    cout << endl;
    return ArrayForVoxSim;
}


int leftExists(std::vector<int> ArrayForVoxelyze, int currentPosition)
{
    if (currentPosition % num_x_voxels == 0)return -999;
    else if (ArrayForVoxelyze[currentPosition - 1] == 0)return -999;
    else return (currentPosition - 1);
}

int rightExists(std::vector<int> ArrayForVoxelyze, int currentPosition)
{
    if (currentPosition % num_x_voxels == num_x_voxels-1) return -999;
    else if (ArrayForVoxelyze[currentPosition + 1] == 0) return -999;
    else { return (currentPosition + 1);}
}

int forwardExists(std::vector<int> ArrayForVoxelyze, int currentPosition)
{
    if (int(currentPosition/num_x_voxels) % num_y_voxels == num_y_voxels-1) return -999;
    else if (ArrayForVoxelyze[currentPosition + num_x_voxels] == 0) return -999;
    else { return (currentPosition + num_x_voxels);}
}

int backExists(std::vector<int> ArrayForVoxelyze, int currentPosition)
{
    if (int(currentPosition/num_x_voxels) % num_y_voxels == 0) return -999;
    else if (ArrayForVoxelyze[currentPosition - num_x_voxels] == 0) return -999;
    else { return (currentPosition - num_x_voxels);}
}

int upExists(std::vector<int> ArrayForVoxelyze, int currentPosition)
{
    if (int(currentPosition/(num_x_voxels*num_y_voxels)) % num_z_voxels == num_z_voxels-1) return -999;
    else if (ArrayForVoxelyze[currentPosition + num_x_voxels*num_y_voxels] == 0) return -999;
    else { return (currentPosition + num_x_voxels*num_y_voxels);}
}

int downExists(std::vector<int> ArrayForVoxelyze, int currentPosition)
{
    if (int(currentPosition/(num_x_voxels*num_y_voxels)) % num_z_voxels == 0) return -999;
    else if (ArrayForVoxelyze[currentPosition - num_x_voxels*num_y_voxels] == 0) return -999;
    else { return (currentPosition - num_x_voxels*num_y_voxels); }
}


std::pair< std::queue<int>, std::list<int> > circleOnce(std::vector<int> ArrayForVoxelyze, std::queue<int> queueToCheck, std::list<int> alreadyChecked)
{
    int currentPosition = queueToCheck.front();
    queueToCheck.pop();
    int index;
    index = leftExists(ArrayForVoxelyze, currentPosition);

    if (index>0 && find(alreadyChecked.begin(),alreadyChecked.end(),index)==alreadyChecked.end())
    {queueToCheck.push(index);}
    alreadyChecked.push_back(index);
    index = rightExists(ArrayForVoxelyze, currentPosition);

    if (index>0 && find(alreadyChecked.begin(),alreadyChecked.end(),index)==alreadyChecked.end())
    {queueToCheck.push(index);}
    alreadyChecked.push_back(index);
    index = forwardExists(ArrayForVoxelyze, currentPosition);

    if (index>0 && find(alreadyChecked.begin(),alreadyChecked.end(),index)==alreadyChecked.end())
    {queueToCheck.push(index);}
    alreadyChecked.push_back(index);
    index = backExists(ArrayForVoxelyze, currentPosition);

    if (index>0 && find(alreadyChecked.begin(),alreadyChecked.end(),index)==alreadyChecked.end())
    {queueToCheck.push(index);}
    alreadyChecked.push_back(index);
    index = upExists(ArrayForVoxelyze, currentPosition);

    if (index>0 && find(alreadyChecked.begin(),alreadyChecked.end(),index)==alreadyChecked.end())
    {queueToCheck.push(index);}
    alreadyChecked.push_back(index);
    index = downExists(ArrayForVoxelyze, currentPosition);

    if (index>0 && find(alreadyChecked.begin(),alreadyChecked.end(),index)==alreadyChecked.end())
    {queueToCheck.push(index);}
    alreadyChecked.push_back(index);
    return make_pair (queueToCheck, alreadyChecked);
}

std::vector<int> makeOneShapeOnly(std::vector<int> ArrayForVoxelyze)
{
    //find start value:
    std::queue<int> queueToCheck;
    std::list<int> alreadyChecked;
    std::pair<  std::queue<int>, std::list<int> > queueAndList;
    int startVoxel = int(ArrayForVoxelyze.size()/2);
    queueToCheck.push(startVoxel);
    alreadyChecked.push_back(startVoxel);

    while (ArrayForVoxelyze[startVoxel] == 0 && not queueToCheck.empty())
    {
        startVoxel = queueToCheck.front();
        queueAndList = circleOnce(ArrayForVoxelyze, queueToCheck, alreadyChecked);
        queueToCheck = queueAndList.first;
        alreadyChecked = queueAndList.second;

    }
    // create only one shape:
    alreadyChecked.clear();
    alreadyChecked.push_back(startVoxel);
    //queueToCheck.clear();
    while (!queueToCheck.empty()) {queueToCheck.pop();}
    queueToCheck.push(startVoxel);
    while (!queueToCheck.empty())
    {
        startVoxel = queueToCheck.front();
        queueAndList = circleOnce(ArrayForVoxelyze, queueToCheck, alreadyChecked);
        queueToCheck = queueAndList.first;
        alreadyChecked = queueAndList.second;
    }

    for (int v=0; v<ArrayForVoxelyze.size(); v++)
    {
        if (find(alreadyChecked.begin(),alreadyChecked.end(),v)==alreadyChecked.end())
        {
            ArrayForVoxelyze[v]=0;
        }
    }
    return ArrayForVoxelyze;
}

