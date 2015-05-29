#include "VoxBotCreator.h"
#include <vector>
#include <iostream>
#include <string>
#include <QString>
#include <ctime>
#include <QCoreApplication>
#include <QThreadPool>
#include <QDir>
#include <QMap>
#include <QMutex>
#include <tinyxml.h>

enum SimulationType{
    ST_NORMAL = 0,
    ST_MIRROR = 1
};

QMutex mutex;
// probability of adding a new voxel
const double PROB_ADD = 0.95;
// probability that the new voxel will be from the same material
const double PROB_SAME = 0.5;
// simulation type
SimulationType simType = ST_NORMAL;
// fitness to be used by the simulator
std::string fitnessType = "2";
std::string penaltyType = "1";
int BotNum = 1000;
// source simulation
QString defaultSource = "../../SimFiles/default/default.vxa";
QString defaultOutputPath;
// output directory
QString defaultOutputDir;
// global variable for the simulation number to be used from each thread
int CurrentSimulationNum = 0;

// returns a uniform number in [0,1].
double unifRand(){return rand() / double(RAND_MAX);}
//returns a uniform integer number from 0 to n
int unifRand(int n){
    if (n < 0) n = -n;
    if (n==0) return 0;
    int guard = (int) (unifRand() * n) +1;
    return (guard > n)? n : guard;
}

// seed function
void seed(){ srand(time(0)); for (int i = 0; i < 10; ++i) unifRand();}

inline void fillPossiblePositions(Vec3D<int> p, int xDim, int yDim, int zDim, int ***strArr, std::vector<Vec3D<int> > &pvp)
{
    // x axis -- check neighbors
    if(p.getX() + 1 < xDim){if(strArr[p.getX()+1][p.getY()][p.getZ()] == 0) pvp.push_back(Vec3D<int>(p.getX()+1, p.getY(), p.getZ()));}
    if(p.getX() - 1 >= 0)  {if(strArr[p.getX()-1][p.getY()][p.getZ()] == 0) pvp.push_back(Vec3D<int>(p.getX()-1, p.getY(), p.getZ()));}
    // y axis -- check neighbors
    if(p.getY() + 1 < yDim){if(strArr[p.getX()][p.getY()+1][p.getZ()] == 0) pvp.push_back(Vec3D<int>(p.getX(), p.getY()+1, p.getZ()));}
    if(p.getY() - 1 >= 0)  {if(strArr[p.getX()][p.getY()-1][p.getZ()] == 0) pvp.push_back(Vec3D<int>(p.getX(), p.getY()-1, p.getZ()));}
    // z axis -- check neighbors
    if(p.getZ() + 1 < zDim){if(strArr[p.getX()][p.getY()][p.getZ()+1] == 0) pvp.push_back(Vec3D<int>(p.getX(), p.getY(), p.getZ()+1));}
    if(p.getZ() - 1 >= 0)  {if(strArr[p.getX()][p.getY()][p.getZ()-1] == 0) pvp.push_back(Vec3D<int>(p.getX(), p.getY(), p.getZ()-1));}
}

void makeRandomVoxBot(int xDim, int yDim, int zDim, int nMat, std::vector<int> &structure, VoxBotCreator &creator)
{
    // declare structure array
    // initialization, empty voxels
    int*** strArr = new int**[xDim];
    for(int x = 0; x < xDim; ++x){strArr[x] = new int*[yDim];for(int y = 0; y < yDim; ++y){strArr[x][y] = new int[zDim]; for(int z = 0; z < yDim; ++z){strArr[x][y][z] = 0;}}}
    // holds every voxel that it is set to a non-zero value
    std::vector<Vec3D<int> > strVox;

    // initialize
    if(simType == ST_NORMAL){
        int x = 0;
        int y = unifRand(yDim) - 1;
        int z = unifRand(zDim) - 1;
        strArr[x][y][z] = 4; // material
        strVox.push_back(Vec3D<int>(x, y, z));
    }
    else if(simType == ST_MIRROR){
        int x = 0;
        int y = yDim/2 -1;
        int z = unifRand(zDim-1);
        strArr[x][y][z] = 4; // material
        strVox.push_back(Vec3D<int>(x, y, z));
    }

    int numberOfAddedVoxels = 0;
    double randomAdd = 0.0;
    while(randomAdd < PROB_ADD){
        std::vector<Vec3D<int> > pvp;
        // add all possible voxel positions that can be added
        Vec3D<int> sourceVoxel = strVox.at(unifRand(strVox.size())-1);
        int a = 0;
        do{
            a++;
            pvp.clear();
            if(simType == ST_MIRROR) fillPossiblePositions(sourceVoxel, xDim, yDim / 2, zDim, strArr, pvp);
            else if(simType == ST_NORMAL) fillPossiblePositions(sourceVoxel, xDim, yDim, zDim, strArr, pvp);
            sourceVoxel = strVox.at(unifRand(strVox.size())-1);
        }while(pvp.size() == 0 && a < 100);
        if(pvp.size() == 0) break;
        numberOfAddedVoxels++;
        int randomSelection = unifRand(pvp.size());
        Vec3D<int> newVoxel = pvp.at(randomSelection-1);

        if(unifRand() < PROB_SAME) strArr[newVoxel.getX()][newVoxel.getY()][newVoxel.getZ()] = strArr[sourceVoxel.getX()][sourceVoxel.getY()][sourceVoxel.getZ()];
        else strArr[newVoxel.getX()][newVoxel.getY()][newVoxel.getZ()] = unifRand(creator.GetNumMaterials() - 1);
        strVox.push_back(newVoxel);
        randomAdd = unifRand();
    }
    if(simType == ST_MIRROR){for (int x0 = 0; x0 < xDim; ++x0){for (int y0 = 0; y0 < yDim/2; ++y0){for (int z0 = 0; z0 < zDim; ++z0){strArr[x0][yDim - y0 - 1][z0] = strArr[x0][y0][z0];}}}}
    // set the created structure in the std::vector.
    for (int x1 = 0; x1 < xDim; ++x1){for (int y1 = 0; y1 < yDim; ++y1){for (int z1 = 0; z1 < zDim; ++z1){structure.push_back(strArr[x1][y1][z1]);}}}
    // free up memory
    for (int x2 = 0; x2 < xDim; ++x2) {for (int y2 = 0; y2 < yDim; ++y2)delete [] strArr[x2][y2]; delete [] strArr[x2];}delete [] strArr;
}

//
// parse xml and get fitness, penalty, behavior
//
void readVFOfitnessValue(std::string filename, bool &valid, double &fitness, double &penalty)
{
    TiXmlDocument doc( filename );
    doc.LoadFile();
    TiXmlElement* root = doc.FirstChildElement();
    if(root == NULL){std::cerr << "Failed to load file: No root element." << std::endl;doc.Clear(); exit(45); }
    for(TiXmlElement* elem = root->FirstChildElement(); elem != NULL; elem = elem->NextSiblingElement()){
        std::string elemName = elem->Value();
        if(elemName == "Valid")
        {
            std::istringstream buffer(elem->GetText()); buffer >> valid;
        }
        else if(elemName == "Fitness")
        {
            TiXmlElement* tmp = elem->FirstChildElement();
            for(TiXmlElement* FitnessElement = tmp; FitnessElement != NULL; FitnessElement = FitnessElement->NextSiblingElement())
            {
                std::string FitnessElementName = FitnessElement->Value();
                if(FitnessElementName == "Value") fitness = std::atof(FitnessElement->GetText());
            }
        }
        else if(elemName == "Penalty")
        {
            TiXmlElement* tmp = elem->FirstChildElement();
            for(TiXmlElement* PenaltyElement = tmp; PenaltyElement != NULL; PenaltyElement = PenaltyElement->NextSiblingElement())
            {
                std::string PenaltyElementName = PenaltyElement->Value();
                if(PenaltyElementName == "Value") penalty = std::atof(PenaltyElement->GetText());
            }
        }
    }
    doc.Clear(); return;
}

class Simulation : public QRunnable
{
    void run()
    {
        VoxBotCreator creator;
        creator.LoadVXASimulation(defaultSource.toStdString());
        // **** lock mutex
        mutex.lock();
        CurrentSimulationNum++;
        std::cout << "CurrentSimulationNum: " << CurrentSimulationNum << std::endl;
        std::stringstream ss;
        ss << CurrentSimulationNum;
        mutex.unlock();
        // **** unlock

        std::vector<int> structure;
        int xDim = creator.getXDim(), yDim = creator.getYDim(),zDim = creator.getZDim(), nMat = creator.GetNumMaterials();
        makeRandomVoxBot(xDim, yDim, zDim, nMat, structure, creator);
        if(!creator.InsertVoxBot(structure)) return;
        std::string experiment_str = ss.str();
        QString tmp = defaultOutputPath; tmp.append(defaultOutputDir);
        std::string out = tmp.toStdString() + "individual_"+ experiment_str +".vxa";
        creator.writeVXASimulation(out);
        std::string call = "./../../VoxSim/build/VoxSim -f " + fitnessType + " -i " + out + " -q -fp " + penaltyType + " -o " + tmp.toStdString();
        if(system(call.c_str()) == -1) return;
        std::string outFitness = tmp.toStdString() + "individual_"+ experiment_str +".vfo";
        double fitnessFromFile = -13.0f, penalty; bool valid;
        readVFOfitnessValue(outFitness, valid, fitnessFromFile, penalty);

        if(valid)
            fitnessFromFile = ( 1.00 - penalty ) * fitnessFromFile;
        else
            fitnessFromFile = 0.0f;
        std::string newVxaName = tmp.toStdString() + "fit_" + QString::number(fitnessFromFile, 'f', 6).toStdString() + "--" + "individual_"+ experiment_str +".vxa";
        creator.writeVXASimulation(newVxaName);
        QFile::remove(out.c_str());
        QFile::remove(outFitness.c_str());
    }
};

int main(int argc, char* argv[])
{
    seed();
    int checkType = 0;
    if(argc == 11){
        int var = 1;
        while(var < argc){
            if(std::string(argv[var]) ==  "-t"){std::stringstream ss;ss << argv[++var];ss >> checkType;}
            else if(std::string(argv[var]) ==  "-o"){defaultOutputPath = argv[++var];}
            else if(std::string(argv[var]) ==  "-f"){fitnessType = argv[++var];}
            else if(std::string(argv[var]) ==  "-fp"){penaltyType = argv[++var];}
            else if(std::string(argv[var]) ==  "-n"){std::stringstream ss;ss << argv[++var];ss >> BotNum; }
            else{ std::cout << argv[var]; std::cerr << "Invalid argument\n"; return -1;}
            var++;
        }
    }else
    {
        std::cerr << "input argument error!" << std::endl;
        std::cout << "Command format: ./RandomVoxBotCreator -f <fitnessType> -fp <fitnessPenaltyType> -t <type> -n <numberOfSimulations> -o <outputPath>\n";
        return -1;
    }
    simType = static_cast<SimulationType>(checkType);

    if(!defaultOutputPath.endsWith('/')){defaultOutputPath += '/';}
    if(!QDir(defaultOutputPath).exists()){QDir().mkdir(defaultOutputPath);}

    QThreadPool *thread_pool = QThreadPool::globalInstance();
    Simulation *task = new Simulation();

    thread_pool->setMaxThreadCount(std::min(4, BotNum));
    std::cout << "Threads are gonna be used: " << thread_pool->maxThreadCount() << std::endl;
    task->setAutoDelete(false);
    while(CurrentSimulationNum < BotNum){
        thread_pool->tryStart(task);
    }
    thread_pool->waitForDone();
    delete task;
    std::cout << "Experiments done...!" << std::endl;
    return 0;
}
