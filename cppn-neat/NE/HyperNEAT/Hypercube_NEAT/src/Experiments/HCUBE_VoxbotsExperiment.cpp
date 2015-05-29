#include "Experiments/HCUBE_VoxbotsExperiment.h"

using namespace NEAT;

namespace HCUBE
{
SoftBotExperiment::SoftBotExperiment(string _experimentName,int _threadID) : Experiment(_experimentName,_threadID)
{
    bestFit = 0.0;
    num_x_voxels = uint(NEAT::Globals::getSingleton()->getParameterValue("NumVoxelsX"));
    num_y_voxels = uint(NEAT::Globals::getSingleton()->getParameterValue("NumVoxelsY"));
    num_z_voxels = uint(NEAT::Globals::getSingleton()->getParameterValue("NumVoxelsZ"));
    num_materials = uint(NEAT::Globals::getSingleton()->getParameterValue("MaterialsNum"));
    fitnessType = uint(NEAT::Globals::getSingleton()->getParameterValue("FitnessType"));
    fitnessPenaltyType = uint(NEAT::Globals::getSingleton()->getParameterValue("FitnessPenaltyType"));
    fitnessPenaltyExp = uint(NEAT::Globals::getSingleton()->getParameterValue("FitnessPenaltyExp"));
    behaviorType = uint(NEAT::Globals::getSingleton()->getParameterValue("BehaviorType"));
    progressSimulationInfo = bool(NEAT::Globals::getSingleton()->getParameterValue("ProgressSimulationInfo"));
    behaviorRecordInterval = double(NEAT::Globals::getSingleton()->getParameterValue("BehaviorRecordInterval"));
    allBehaviors = bool(NEAT::Globals::getSingleton()->getParameterValue("AllBehaviors"));
    voxSimAllBehaviors = int(NEAT::Globals::getSingleton()->getParameterValue("VoxSimAll"));
    behaviorRecordStartInterval = double(NEAT::Globals::getSingleton()->getParameterValue("BehaviorRecordStartInterval"));
    evolveMaterials = bool(NEAT::Globals::getSingleton()->getParameterValue("EvolveMaterialProperties"));
    symmetryInputs = bool(NEAT::Globals::getSingleton()->getParameterValue("SymmetryInputs"));
}

NEAT::GeneticPopulation* SoftBotExperiment::createInitialPopulation(int populationSize)
{
    NEAT::GeneticPopulation* population = new NEAT::GeneticPopulation();
    vector<GeneticNodeGene> genes;

    // Network inputs
    genes.push_back(GeneticNodeGene("Bias","NetworkSensor",0,false));
    genes.push_back(GeneticNodeGene("x","NetworkSensor",0,false));
    genes.push_back(GeneticNodeGene("y","NetworkSensor",0,false));
    genes.push_back(GeneticNodeGene("z","NetworkSensor",0,false));

    if(symmetryInputs)
    {
        genes.push_back(GeneticNodeGene("d","NetworkSensor",0,false));
        genes.push_back(GeneticNodeGene("dxy","NetworkSensor",0,false));
        genes.push_back(GeneticNodeGene("dxz","NetworkSensor",0,false));
        genes.push_back(GeneticNodeGene("dyz","NetworkSensor",0,false));
    }

    // Network outputs
    genes.push_back(GeneticNodeGene("Output","NetworkOutputNode",1,false,ACTIVATION_FUNCTION_SIGMOID)); // is there a voxel?
    for (int i = 0; i < this->num_materials; ++i)
    {
        std::stringstream ss; ss << i;
        genes.push_back(GeneticNodeGene("OutputMat"+ss.str(),"NetworkOutputNode",1,false,ACTIVATION_FUNCTION_SIGMOID)); // Material to be used
        if(evolveMaterials)
        {
            genes.push_back(GeneticNodeGene("OutputMatCTE"+ss.str(),"NetworkOutputNode",1,false,ACTIVATION_FUNCTION_SIGMOID)); // Material's CTE
            genes.push_back(GeneticNodeGene("OutputMatDensity"+ss.str(),"NetworkOutputNode",1,false,ACTIVATION_FUNCTION_SIGMOID)); // Material's density
            genes.push_back(GeneticNodeGene("OutputMatTempPhase"+ss.str(),"NetworkOutputNode",1,false,ACTIVATION_FUNCTION_SIGMOID)); // Material's temporary phase
            genes.push_back(GeneticNodeGene("OutputMatPoissonsRatio"+ss.str(),"NetworkOutputNode",1,false,ACTIVATION_FUNCTION_SIGMOID)); // Material's poissons ratio
        }
    }
    // Create random initial population
    for (size_t a=0;a<populationSize;a++)
    {
        shared_ptr<GeneticIndividual> individual(new GeneticIndividual(genes,true,1.0));
        population->addIndividual(individual);
    }
    return population;
}

void SoftBotExperiment::processGroup(shared_ptr<NEAT::GeneticGeneration> generation)
{
    int genNum = generation->getGenerationNumber() + 1;
    char buffer2 [50];
    sprintf(buffer2, "%04i", genNum);

    for(int z=0;z< group.size();z++)
    {
        std::ostringstream mkGenDir;
        if (genNum % (int)NEAT::Globals::getSingleton()->getParameterValue("RecordEntireGenEvery") == 0)
        {
            mkGenDir << "mkdir -p Gen_" << buffer2;
        }
        std::system(mkGenDir.str().c_str());

        shared_ptr<NEAT::GeneticIndividual> individual = group[z];
        char buffer1 [50]; sprintf(buffer1, "%04i", genNum);
        std::ostringstream tmp;
        tmp << "Softbot" << "--Gen_" << buffer1 << "--Ind_" << individual;
        string individualID = tmp.str();
        cout_ << individualID << endl;
        processEvaluation(individual, individualID, genNum, bestFit);
    }
}

void SoftBotExperiment::processEvaluation(shared_ptr<NEAT::GeneticIndividual> individual, string individualID, int genNum, double &bestFit)
{
    double fitness;
    //initializes continuous space array with zeros. +1 is because we need to sample
    // these points at all corners of each voxel, leading to n+1 points in any dimension
    float MaterialsCTE[this->num_materials];
    float MaterialsPoissons[this->num_materials];
    float MaterialsTempPhase[this->num_materials];
    float MaterialsDensity[this->num_materials];
    CArray3Df ContinuousArray(num_x_voxels, num_y_voxels, num_z_voxels);
    vector<CArray3Df> ContinuousArrayMaterials;
    for (int mat = 0; mat < this->num_materials; ++mat)
    {
        CArray3Df ContinuousArrayMat(num_x_voxels, num_y_voxels, num_z_voxels);
        ContinuousArrayMaterials.push_back(ContinuousArrayMat);
    }
    // CPPN network for the individual
    NEAT::FastNetwork <double> network = individual->spawnFastPhenotypeStack<double>();
    float xNor, yNor, zNor;
    int x, y, z;
    //iterate through each location of the continuous matrix
    for (int i=0; i<ContinuousArray.GetFullSize(); i++)
    {
        // get the coordinates from the continuous array
        ContinuousArray.GetXYZ(&x, &y, &z, i);
        xNor = mapXYvalToNormalizedGridCoord(x, num_x_voxels);
        yNor = mapXYvalToNormalizedGridCoord(y, num_y_voxels);
        zNor = mapXYvalToNormalizedGridCoord(z, num_z_voxels);
        // reset cppn
        network.reinitialize();
        // set normalized values to inputs
        network.setValue("x",xNor); network.setValue("y",yNor); network.setValue("z",zNor); network.setValue("Bias",0.3);
        // set symmetrical inputs
        if(symmetryInputs)
        {
            network.setValue("d",sqrt(pow(xNor, 2) + pow(yNor, 2) + pow(zNor, 2)));
            network.setValue("dxy",sqrt(pow(xNor, 2) + pow(yNor, 2)));
            network.setValue("dxz",sqrt(pow(xNor, 2) + pow(zNor, 2)));
            network.setValue("dyz",sqrt(pow(yNor, 2) + pow(zNor, 2)));
        }
        // update the network after initializing the inputs
        network.update();
        // get value for voxel empty or not
        ContinuousArray[i] = network.getValue("Output");
        // get values for materials
        for (int matIndex = 0; matIndex < this->num_materials; ++matIndex)
        {
            std::stringstream ss; ss << matIndex;
            ContinuousArrayMaterials[matIndex][i] = network.getValue("OutputMat"+ss.str());
            if(i==0 && evolveMaterials) // we get properties at zero point
            {
                MaterialsCTE[matIndex] = network.getValue("OutputMatCTE"+ss.str());
                MaterialsDensity[matIndex] = network.getValue("OutputMatDensity"+ss.str());
                MaterialsTempPhase[matIndex] = network.getValue("OutputMatTempPhase"+ss.str());
                MaterialsPoissons[matIndex] = network.getValue("OutputMatPoissonsRatio"+ss.str());
            }
        }
    }
    // Write the vox simulation file given the individual
    if(evolveMaterials)
        writeVoxFile(individual, individualID, ContinuousArray, ContinuousArrayMaterials, MaterialsCTE, MaterialsDensity, MaterialsTempPhase, MaterialsPoissons);
    else
        writeVoxFile(individual, individualID, ContinuousArray, ContinuousArrayMaterials);
    // Open md5sum file to create a key for the map with the saved fitness
    string md5sumString = ReadMd5sum();
    if(progressSimulationInfo) cout_ << "md5sum: " << md5sumString << "\n";
    // Read behavior variables, only if there is behavior

    NEAT::GeneticIndividualBehavior behaviorSignature;
    behaviorSignature.preProcessType = static_cast<BehaviorPreprocessingType>(int(NEAT::Globals::getSingleton()->getParameterValue("PreProcessType")));
    behaviorSignature.evaluationType = static_cast<BehaviorSimilarityEvaluationType>(int(NEAT::Globals::getSingleton()->getParameterValue("SimilarityEvaluationType")));

    double noPenaltyFitness; // Fitness without penalty impact
    bool validIndividual; // valid individual, invalid when number of voxels too small
    // If individual already evaluated
    if ((IndividualLookUpTable.find(md5sumString) != IndividualLookUpTable.end()) && !evolveMaterials) // only when we don't evolve materials
    {
        IndividualData tmp = IndividualLookUpTable[md5sumString];
        fitness = tmp.Fitness;
        noPenaltyFitness = tmp.OrFitness;
        if (behaviorType != 0.0) behaviorSignature = tmp.Behavior;
        if(progressSimulationInfo) cout_ << "This individual was already simulated --> Its fitness is: " << fitness << "\n";
    }
    else
    {
        std::ostringstream simCmd;
        simCmd << "../../../../VoxSim/build/VoxSim"
               << " -f " << fitnessType
               << " -fp "  << fitnessPenaltyType
               << " -fpe " << fitnessPenaltyExp
               << " -bi " << behaviorRecordInterval
               << " -bs " << behaviorRecordStartInterval
               << " -b " << (allBehaviors ? voxSimAllBehaviors : behaviorType)
               << (!progressSimulationInfo ? " -q" : "")
               << " -i " << individualID << ".vxa"
               << " -o .";

        std::system(simCmd.str().c_str());
        // Read fitness, penalty, behavior
        double penalty;
        readVfo(individualID + ".vfo", validIndividual, noPenaltyFitness, penalty, behaviorSignature);
        if(behaviorType!= 0) behaviorSignature.preProcessing();
        // Fitness is set to zero if simulation was not valid, otherwise we multiply fitness by 1 - penalty
        if(!validIndividual) fitness = 0.0;
        else fitness = ( 1.0 - penalty ) * noPenaltyFitness;
        // Make individual object to store it in the hashtable
        IndividualData tmp;
        tmp.Fitness = fitness; tmp.OrFitness = noPenaltyFitness;
        if(behaviorType != 0) tmp.Behavior = behaviorSignature;
        IndividualLookUpTable[md5sumString] = tmp;
        // Remove Simulation-output file
        std::ostringstream removeFitFileCmd;
        removeFitFileCmd << "rm " << individualID << ".vfo";
        std::system(removeFitFileCmd.str().c_str());
    }
    // Set best fitness
    if (bestFit<fitness) bestFit = fitness;

    if(fitness < 0.0 || fitness > std::numeric_limits<double>::max())
    {
        cerr << "ERROR : fitness greater than size of the double or less than zero." << "\n";
        exit(9);
    }
    fitness += 10e-5; // Add a small number so zero fitness will not stop the evolution.

    if(progressSimulationInfo) cout_ << "Individual Evaluation complete, fitness: " << fitness << "\n";
    // set individual fitness
    individual->setFitness(fitness);
    individual->originalFitness = noPenaltyFitness;
    // set behavior
    individual->behavior = behaviorSignature;

    // Store files when it needs to
    if (genNum % (int)NEAT::Globals::getSingleton()->getParameterValue("RecordEntireGenEvery")==0)
    {
        std::ostringstream moveFitFileCmd;
        char gen [100]; sprintf(gen, "%04i", genNum);
        char fit[100]; sprintf(fit, "%.8lf", fitness);
        char orFit[100]; sprintf(orFit, "%.8lf", noPenaltyFitness);
        moveFitFileCmd << "mv " << individualID << ".vxa" << " Gen_" << gen << "/" << individualID << "--Fitness_" << fit << "--orFit_" << orFit << ".vxa";
        std::system(moveFitFileCmd.str().c_str());

        if(validIndividual && behaviorType != 0 && (int)NEAT::Globals::getSingleton()->getParameterValue("RecordBehaviorSignatureCSV") == 1)
        {
            vector<string> outNames;
            individual->behavior.writeSignature(individualID, ".csv", allBehaviors, outNames);
            for (int i=0;i < outNames.size(); i++)
            {
                std::ostringstream moveSigFileCmd;
                moveSigFileCmd << "mv " << outNames[i] << ".csv" << " Gen_" << gen << "/" << outNames[i] << "--Fitness_" << fit << ".csv";
                std::system(moveSigFileCmd.str().c_str());
            }
        }
        else if(validIndividual && behaviorType != 0 && (int)NEAT::Globals::getSingleton()->getParameterValue("RecordBehaviorSignaturePDF") == 1)
        {
            vector<string> outNames;
            individual->behavior.writeSignature(individualID, ".csv", allBehaviors, outNames);
            std::ostringstream makePdfFromCsv;
            makePdfFromCsv << "python ../../../../drawing_utils/mergeSave.py . " << individualID;
            std::system(makePdfFromCsv.str().c_str());
            for (int i=0;i < outNames.size(); i++)
            {
                std::ostringstream removeCSVfile;
                removeCSVfile << "rm " << outNames[i] << ".csv";
                std::system(removeCSVfile.str().c_str());
            }
            std::ostringstream moveSigFileCmd;
            moveSigFileCmd << "mv " << individualID << ".pdf" << " Gen_" << gen << "/" << individualID << "--Fitness_" << fit << ".pdf";
            std::system(moveSigFileCmd.str().c_str());
        }
    }else
    {
        // Delete simulation file
        std::ostringstream removeSimFileCmd;
        removeSimFileCmd << "rm " << individualID << ".vxa";
        std::system(removeSimFileCmd.str().c_str());
    }
}

void SoftBotExperiment::writeVoxFile(shared_ptr<NEAT::GeneticIndividual> individual, string individualID, CArray3Df ContinuousArray, vector<CArray3Df> ContinuousArrayMaterial)
{
    ofstream md5file;
    md5file.open ("md5sumTMP.txt");
    std::ostringstream myFileName;
    myFileName << individualID << ".vxa";
    vector<int> voxbot = createArrayForVoxSim(ContinuousArray, ContinuousArrayMaterial);
    individual->VoxBotSimFile = voxbot;
    VoxBotCreator vxbc;
    vxbc.LoadVXASimulation("../../../../SimFiles/default/default.vxa");
    if(vxbc.getXDim() != num_x_voxels || vxbc.getYDim() != num_y_voxels || vxbc.getZDim() != num_z_voxels)
        vxbc.resizeStructure(num_x_voxels, num_y_voxels, num_z_voxels);
    vxbc.clearStructure();
    vxbc.InsertVoxBot(voxbot);
    vxbc.writeVXASimulation(myFileName.str());
    for (int i = 0; i < voxbot.size(); ++i) md5file << voxbot.at(i);
    md5file.close();
}

void SoftBotExperiment::writeVoxFile(shared_ptr<NEAT::GeneticIndividual> individual, string individualID, CArray3Df ContinuousArray, vector<CArray3Df> ContinuousArrayMaterial,
                                     float MaterialsCTE[], float MaterialsDensity[], float MaterialsTempPhase[], float MaterialsPoissons[])
{
    ofstream md5file;
    md5file.open ("md5sumTMP.txt");
    std::ostringstream myFileName;
    myFileName << individualID << ".vxa";
    vector<int> voxbot = createArrayForVoxSim(ContinuousArray, ContinuousArrayMaterial);
    individual->VoxBotSimFile = voxbot;
    VoxBotCreator vxbc;
    vxbc.LoadVXASimulation("../../../../SimFiles/default/default.vxa");
    if(vxbc.getXDim() != num_x_voxels || vxbc.getYDim() != num_y_voxels || vxbc.getZDim() != num_z_voxels)
        vxbc.resizeStructure(num_x_voxels, num_y_voxels, num_z_voxels);
    vxbc.clearStructure();
    vxbc.InsertVoxBot(voxbot);
    for (int matIndex = 0; matIndex < this->num_materials; ++matIndex)
    {
        float cte = (MaterialsCTE[matIndex]) * 0.01;
        float density = ((((MaterialsDensity[matIndex] + 1.0) / 2.0) * 6.0) + 2) * 10e+6;
        float poissons = ((MaterialsPoissons[matIndex] + 1.0) / 2.0) * 0.45;
        float tempPhase = ((MaterialsTempPhase[matIndex] + 1.0) / 2.0) * (2 * PI);

        vxbc.setMatProperties(matIndex + 1, cte, density, poissons, tempPhase);
//        cout << "cte: " << cte << " ,density: " << density << " ,poissons: " << poissons << " ,tempPhase: " << tempPhase << endl;
        individual->MaterialProperties[matIndex][0] = cte; // + 1 to matIndex because ids start from 1
        individual->MaterialProperties[matIndex][1] = density;
        individual->MaterialProperties[matIndex][2] = poissons;
        individual->MaterialProperties[matIndex][3] = tempPhase;
    }
    vxbc.writeVXASimulation(myFileName.str());
    for (int i = 0; i < voxbot.size(); ++i) md5file << voxbot.at(i);
    md5file.close();
}


vector<int> SoftBotExperiment::createArrayForVoxSim(CArray3Df ContinuousArray,vector<CArray3Df> ContinuousArrayMaterial)
{
    vector<int> materialVector;
    CArray3Df ArrayForVoxSim(num_x_voxels, num_y_voxels, num_z_voxels);
    int v=0;
    for (int z=0; z<num_z_voxels; z++)
    {
        for (int y=0; y<num_y_voxels; y++)
        {
            for (int x=0; x<num_x_voxels; x++)
            {
                if(ContinuousArray[v] > 0)
                {
                    int materialSelected = -1;
                    for (int matIndex = 0; matIndex < num_materials; ++matIndex)
                    {
                        int greaterThanNum = 0;
                        for (int matIndexComp = 0; matIndexComp < num_materials; ++matIndexComp)
                        {
                            if(matIndex != matIndexComp && ContinuousArrayMaterial[matIndex][v] > ContinuousArrayMaterial[matIndexComp][v])
                                greaterThanNum++;
                        }
                        if(greaterThanNum == num_materials - 1) // this material has the largest value of all
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
    for (int i=0; i < ArrayForVoxSim.GetFullSize(); i++)
    {
        materialVector.push_back(ArrayForVoxSim[i]);
    }
    return materialVector;
}


CArray3Df SoftBotExperiment::makeOneShapeOnly(CArray3Df ArrayForVoxelyze)
{
    //find start value:
    queue<int> queueToCheck;
    list<int> alreadyChecked;
    pair<  queue<int>, list<int> > queueAndList;
    int startVoxel = int(ArrayForVoxelyze.GetFullSize()/2);
    queueToCheck.push(startVoxel);
    alreadyChecked.push_back(startVoxel);
    while (ArrayForVoxelyze[startVoxel] == 0 && !queueToCheck.empty())
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
    for (int v=0; v<ArrayForVoxelyze.GetFullSize(); v++)
        if (find(alreadyChecked.begin(),alreadyChecked.end(),v)==alreadyChecked.end()) ArrayForVoxelyze[v]=0;
    return ArrayForVoxelyze;
}

pair< queue<int>, list<int> > SoftBotExperiment::circleOnce(CArray3Df ArrayForVoxelyze, queue<int> queueToCheck, list<int> alreadyChecked)
{
    int currentPosition = queueToCheck.front();
    queueToCheck.pop();

    int index = leftExists(ArrayForVoxelyze, currentPosition);

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

    return make_pair(queueToCheck, alreadyChecked);
}

int SoftBotExperiment::leftExists(CArray3Df ArrayForVoxelyze, int currentPosition)
{
    if (currentPosition % num_x_voxels == 0) return -999;
    else if (ArrayForVoxelyze[currentPosition - 1] == 0) return -999;
    else return (currentPosition - 1);
}

int SoftBotExperiment::rightExists(CArray3Df ArrayForVoxelyze, int currentPosition)
{
    if (currentPosition % num_x_voxels == num_x_voxels-1) return -999;
    else if (ArrayForVoxelyze[currentPosition + 1] == 0) return -999;
    else {numConnections++; return (currentPosition + 1);}
}

int SoftBotExperiment::forwardExists(CArray3Df ArrayForVoxelyze, int currentPosition)
{
    if (int(currentPosition/num_x_voxels) % num_y_voxels == num_y_voxels-1) return -999;
    else if (ArrayForVoxelyze[currentPosition + num_x_voxels] == 0) return -999;
    else {numConnections++; return (currentPosition + num_x_voxels);}
}

int SoftBotExperiment::backExists(CArray3Df ArrayForVoxelyze, int currentPosition)
{
    if (int(currentPosition/num_x_voxels) % num_y_voxels == 0) return -999;
    else if (ArrayForVoxelyze[currentPosition - num_x_voxels] == 0) return -999;
    else {numConnections++; return (currentPosition - num_x_voxels);}
}

int SoftBotExperiment::upExists(CArray3Df ArrayForVoxelyze, int currentPosition)
{
    if (int(currentPosition/(num_x_voxels*num_y_voxels)) % num_z_voxels == num_z_voxels-1) return -999;
    else if (ArrayForVoxelyze[currentPosition + num_x_voxels*num_y_voxels] == 0) return -999;
    else {numConnections++; return (currentPosition + num_x_voxels*num_y_voxels);}
}

int SoftBotExperiment::downExists(CArray3Df ArrayForVoxelyze, int currentPosition)
{
    if (int(currentPosition/(num_x_voxels*num_y_voxels)) % num_z_voxels == 0) return -999;
    else if (ArrayForVoxelyze[currentPosition - num_x_voxels*num_y_voxels] == 0) return -999;
    else {numConnections++; return (currentPosition - num_x_voxels*num_y_voxels);}
}

double SoftBotExperiment::mapXYvalToNormalizedGridCoord(const int & r_xyVal, const int & r_numVoxelsXorY)
{
    // turn the xth or yth node into its coordinates on a grid from -1 to 1, e.g. x values (1,2,3,4,5) become (-1, -.5 , 0, .5, 1)
    // this works with even numbers, and for x or y grids only 1 wide/tall, which was not the case for the original
    // e.g. see findCluster for the orignal versions where it was not a funciton and did not work with odd or 1 tall/wide
    if(r_numVoxelsXorY==1)
        return 0;
    return (-1 + ( r_xyVal * 2.0 / ( r_numVoxelsXorY - 1 )));
}

std::string SoftBotExperiment::ReadMd5sum()
{
    FILE* pipe = popen("md5sum md5sumTMP.txt", "r");
    if (!pipe) {cout_ << "ERROR 1, exiting." << "\n"; exit(1);}
    char buffer[128];
    std::string result = "";
    while(!feof(pipe))
    {
        if(fgets(buffer, 128, pipe) != NULL)
            result += buffer;
    }
    pclose(pipe);
    return result.substr(0,32);
}

//
// parse xml and get fitness, penalty, behavior
//
void SoftBotExperiment::readVfo(std::string filename, bool &valid, double &fitness, double &penalty, NEAT::GeneticIndividualBehavior &behavior)
{
    TiXmlDocument doc( filename );
    doc.LoadFile();
    TiXmlElement* root = doc.FirstChildElement();
    if(root == NULL){std::cerr << "Failed to load file: No root element." << "\n";doc.Clear(); exit(45);}
    for(TiXmlElement* elem = root->FirstChildElement(); elem != NULL; elem = elem->NextSiblingElement()){
        std::string elemName = elem->Value();
        if(elemName == "Valid")
        {
            istringstream buffer(elem->GetText()); buffer >> valid;
            if(!valid){
                behavior.validBehavior = false;
                return; // Not valid, no need for signature
            }
            behavior.validBehavior = true;
        }
        else if(elemName == "Fitness")
        {
            TiXmlElement* tmp = elem->FirstChildElement();
            for(TiXmlElement* FitnessElement = tmp; FitnessElement != NULL; FitnessElement = FitnessElement->NextSiblingElement())
            {
                std::string FitnessElementName = FitnessElement->Value();
                if(FitnessElementName == "FitnessType"){
                    int fT = std::atoi(FitnessElement->GetText());
                    if(fT != fitnessType){cerr << "Fitness types do not match!" << endl; exit(65);}
                }
                else if(FitnessElementName == "Value") fitness = std::atof(FitnessElement->GetText());
            }
        }
        else if(elemName == "Penalty")
        {
            TiXmlElement* tmp = elem->FirstChildElement();
            for(TiXmlElement* PenaltyElement = tmp; PenaltyElement != NULL; PenaltyElement = PenaltyElement->NextSiblingElement())
            {
                std::string PenaltyElementName = PenaltyElement->Value();
                if(PenaltyElementName == "PenaltyType"){
                    int fpT = std::atoi(PenaltyElement->GetText());
                    if(fpT != fitnessPenaltyType){cerr << "Penalty types do not match!" << endl; exit(66);}
                }
                else if(PenaltyElementName == "Value")
                    penalty = std::atof(PenaltyElement->GetText());
            }
        }
        else if(elemName == "Behavior")
        {
            TiXmlElement* tmp = elem->FirstChildElement();
            std::vector<std::string> strValueVector;
            int _behaviorType;
            for(TiXmlElement* BehaviorElement = tmp; BehaviorElement != NULL; BehaviorElement = BehaviorElement->NextSiblingElement()){
                std::string BehaviorElementName = BehaviorElement->Value();
                if(BehaviorElementName == "BehaviorType"){
                    _behaviorType = std::atoi(BehaviorElement->GetText());
                    if(_behaviorType != behaviorType && !allBehaviors){cerr << "Behavior types do not match!" << endl; exit(67);}
                }
                else if(BehaviorElementName == "Record"){
                    TiXmlElement* behtmp = BehaviorElement->FirstChildElement();
                    for(TiXmlElement* BehaviorRecordElement = behtmp; BehaviorRecordElement != NULL; BehaviorRecordElement = BehaviorRecordElement->NextSiblingElement()){
                        std::string BehaviorRecordElementName = BehaviorRecordElement->Value();
                        if(BehaviorRecordElementName == "Value")
                            strValueVector.push_back(BehaviorRecordElement->GetText());
                    }
                }
            }
            behavior.makeBehaviorFromStr(strValueVector, _behaviorType);
        }
    }
}

void SoftBotExperiment::createDirTimeStamp()
{
    // Create directory for the experiment
    std::time_t rawtime;
    std::tm* timeinfo;
    char buffer [80];
    std::time(&rawtime);
    timeinfo = std::localtime(&rawtime);
    std::strftime(buffer,80,"%d%m%H%M",timeinfo);

    boost::filesystem::path dir(buffer);
    if(boost::filesystem::create_directory(dir)) {
        std::cout_ << "Create directory to hold this experiment" << "\n";
    }
    outDir = buffer;
}

void SoftBotExperiment::processIndividualPostHoc(shared_ptr<NEAT::GeneticIndividual> individual)
{
}

Experiment* SoftBotExperiment::clone()
{
    SoftBotExperiment* experiment = new SoftBotExperiment(*this);
    return experiment;
}

}
