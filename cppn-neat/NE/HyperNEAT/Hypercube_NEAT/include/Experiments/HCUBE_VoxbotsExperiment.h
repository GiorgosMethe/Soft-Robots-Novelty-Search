/*
[1] code from:
Cheney, Nick, Robert MacCurdy, Jeff Clune, and Hod Lipson.
"Unshackling evolution: evolving soft robots with multiple materials and a powerful generative encoding."
In Proceeding of the fifteenth annual conference on Genetic and evolutionary computation conference, pp. 167-174. ACM, 2013.
*/
#ifndef HCUBE_SOFTBOTSCPPNNEAT_H_INCLUDED
#define HCUBE_SOFTBOTSCPPNNEAT_H_INCLUDED

#include "HCUBE_Experiment.h"
#include "Array3D.h"
#include "opencv2/opencv.hpp"
#include "opencv2/core/core.hpp"
#include "NEAT_GeneticIndividualBehavior.h"
#include "HCUBE_Defines.h"
#include "VoxBotCreator.h"
#include <string>
#include <iostream>
#include <ctime>

namespace HCUBE
{
struct IndividualData
{
    double Fitness;
    double OrFitness;
    NEAT::GeneticIndividualBehavior Behavior;
    unsigned int BehaviorType;
    bool hasValidBehavior;
};

class SoftBotExperiment : public Experiment
{
protected:
    unsigned int num_x_voxels;
    unsigned int num_y_voxels;
    unsigned int num_z_voxels;
    unsigned int num_materials;
    unsigned int numConnections;
    unsigned int fitnessType;
    unsigned int fitnessPenaltyType;
    unsigned int fitnessPenaltyExp;
    unsigned int behaviorType;
    bool symmetryInputs;
    bool progressSimulationInfo;
    double behaviorRecordInterval;
    double behaviorRecordStartInterval;
    bool allBehaviors;
    bool evolveMaterials;
    int voxSimAllBehaviors;
    double bestFit;
    string outDir;

    float readVfo(std::string filename);

    map<std::string, IndividualData > IndividualLookUpTable;

    void processEvaluation(shared_ptr<NEAT::GeneticIndividual> individual, string individualID, int genNum, double &bestFit);

    double mapXYvalToNormalizedGridCoord(const int & r_xyVal, const int & r_numVoxelsXorY);

    void readVfo(std::string filename, bool &valid, double &fitness, double &penalty, NEAT::GeneticIndividualBehavior &behavior);

    void writeVoxFile(shared_ptr<NEAT::GeneticIndividual> individual, string individualID, CArray3Df ContinuousArray, vector<CArray3Df> ContinuousArrayMaterial);

    void writeVoxFile(shared_ptr<NEAT::GeneticIndividual> individual, string individualID, CArray3Df ContinuousArray, vector<CArray3Df> ContinuousArrayMaterial,
                                         float MaterialsCTE[], float MaterialsDensity[], float MaterialsTempPhase[], float MaterialsPoissons[]);

    std::string ReadMd5sum();

    vector<int> createArrayForVoxSim(CArray3Df ContinuousArray,vector<CArray3Df> ContinuousArrayMaterial);

    void createDirTimeStamp();

    // [1]
    CArray3Df makeOneShapeOnly(CArray3Df ArrayForVoxelyze);
    // [1]
    int leftExists(CArray3Df ArrayForVoxelyze, int currentPosition);
    // [1]
    int rightExists(CArray3Df ArrayForVoxelyze, int currentPosition);
    // [1]
    int forwardExists(CArray3Df ArrayForVoxelyze, int currentPosition);
    // [1]
    int backExists(CArray3Df ArrayForVoxelyze, int currentPosition);
    // [1]
    int upExists(CArray3Df ArrayForVoxelyze, int currentPosition);
    // [1]
    int downExists(CArray3Df ArrayForVoxelyze, int currentPosition);
    // [1]
    pair< queue<int>, list<int> > circleOnce(CArray3Df ArrayForVoxelyze, queue<int> queueToCheck, list<int> alreadyChecked);

public:

    SoftBotExperiment(string _experimentName,int _threadID);

    virtual ~SoftBotExperiment()
    {}

    virtual NEAT::GeneticPopulation* createInitialPopulation(int populationSize);

    virtual void processGroup(shared_ptr<NEAT::GeneticGeneration> generation);

    virtual void processIndividualPostHoc(shared_ptr<NEAT::GeneticIndividual> individual);

    virtual bool performUserEvaluations()
    {
        return false;
    }

    virtual inline bool isDisplayGenerationResult()
    {
        return displayGenerationResult;
    }

    virtual inline void setDisplayGenerationResult(bool _displayGenerationResult)
    {
        displayGenerationResult=_displayGenerationResult;
    }

    virtual inline void toggleDisplayGenerationResult()
    {
        displayGenerationResult=!displayGenerationResult;
    }

    virtual Experiment* clone();

    virtual void resetGenerationData(shared_ptr<NEAT::GeneticGeneration> generation)
    {}

    virtual void addGenerationData(shared_ptr<NEAT::GeneticGeneration> generation,shared_ptr<NEAT::GeneticIndividual> individual)
    {}

};
}


#endif // HCUBE_SOFTBOTS-CPPN-NEAT_H_INCLUDED
