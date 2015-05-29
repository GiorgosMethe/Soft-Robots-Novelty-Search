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
#include <queue>


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

QMutex mutex;
// probability of adding a new voxel
const double PROB_ADD = 0.5;
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

int num_x_voxels, num_y_voxels, num_z_voxels;

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

inline bool isAlone(Vec3D<int> p, int xDim, int yDim, int zDim, int ***strArr)
{
    // x axis -- check neighbors
    if(p.getX() + 1 < xDim){if(strArr[p.getX()+1][p.getY()][p.getZ()] == 1) return false;}
    if(p.getX() - 1 >= 0)  {if(strArr[p.getX()-1][p.getY()][p.getZ()] == 1) return false;}
    // y axis -- check neighbors
    if(p.getY() + 1 < yDim){if(strArr[p.getX()][p.getY()+1][p.getZ()] == 1) return false;}
    if(p.getY() - 1 >= 0)  {if(strArr[p.getX()][p.getY()-1][p.getZ()] == 1) return false;}
    // z axis -- check neighbors
    if(p.getZ() + 1 < zDim){if(strArr[p.getX()][p.getY()][p.getZ()+1] == 1) return false;}
    if(p.getZ() - 1 >= 0)  {if(strArr[p.getX()][p.getY()][p.getZ()-1] == 1) return false;}
    return true;
}

void makeRandomVoxBot(int xDim, int yDim, int zDim, int nMat, std::vector<int> &structure, VoxBotCreator &creator)
{
    // declare structure array
    // initialization, empty voxels
    int*** strArr = new int**[xDim];
    for(int x = 0; x < xDim; ++x){strArr[x] = new int*[yDim];for(int y = 0; y < yDim; ++y){strArr[x][y] = new int[zDim]; for(int z = 0; z < yDim; ++z){strArr[x][y][z] = 0;}}}
    // holds every voxel that it is set to a non-zero value
    for(int x = 0; x < xDim; ++x)
    {
        for(int y = 0; y < yDim; ++y)
        {
            for(int z = 0; z < yDim; ++z)
            {
                strArr[x][y][z] = (unifRand() < PROB_ADD);
                if(strArr[x][y][z] == 1)
                {
                    strArr[x][y][z] += unifRand(creator.GetNumMaterials() - 1) -1;
                }
            }
        }
    }
    // set the created structure in the std::vector.
    std::vector<int> structureTmp;
    for (int x1 = 0; x1 < xDim; ++x1){for (int y1 = 0; y1 < yDim; ++y1){for (int z1 = 0; z1 < zDim; ++z1){structureTmp.push_back(strArr[x1][y1][z1]);}}}

    structure = makeOneShapeOnly(structureTmp);
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
        int xDim = num_x_voxels = creator.getXDim(), yDim = num_y_voxels = creator.getYDim(),zDim = num_z_voxels = creator.getZDim(), nMat = creator.GetNumMaterials();
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

int main(int argc, char* argv[])
{
    seed();
    if(argc == 9){
        int var = 1;
        while(var < argc){
            if(std::string(argv[var]) ==  "-o"){defaultOutputPath = argv[++var];}
            else if(std::string(argv[var]) ==  "-f"){fitnessType = argv[++var];}
            else if(std::string(argv[var]) ==  "-fp"){penaltyType = argv[++var];}
            else if(std::string(argv[var]) ==  "-n"){std::stringstream ss;ss << argv[++var];ss >> BotNum; }
            else{ std::cout << argv[var]; std::cerr << "Invalid argument\n"; return -1;}
            var++;
        }
    }else
    {
        std::cerr << "input argument error!" << std::endl;
        std::cout << "Command format: ./RandomSoftBots -f <fitnessType> -fp <fitnessPenaltyType>  -n <numberOfSimulations> -o <outputPath>\n";
        return -1;
    }
    if(!defaultOutputPath.endsWith('/')){defaultOutputPath += '/';}
    if(!QDir(defaultOutputPath).exists()){QDir().mkdir(defaultOutputPath);}

    QThreadPool *thread_pool = QThreadPool::globalInstance();
    Simulation *task = new Simulation();

    thread_pool->setMaxThreadCount(std::min(8, BotNum));
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
