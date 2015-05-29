#include "Voxelyze/VX_Sim.h"
#include "Voxelyze/VX_SimGA.h"
#include "boost/algorithm/string/split.hpp"
#include "boost/algorithm/string/classification.hpp"
#include "boost/timer.hpp"
#include <boost/filesystem.hpp>
#include <string>
#include <iostream>
#include <fstream>

using namespace std;

void printHelp()
{
    std::ifstream ifs("help.txt");
    std::string str((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
    std::cout << str;
}

int main(int argc, char* argv[])
{
    bool print= false, outputFile = false, fitness = false, input = false, behavior = false, penalty = false, interval = false, penaltyExp = false, quiet = false, recordStart = false;
    int fitnessType, behaviorType, penaltyType;
    double behaviorRecInterval, fitnessPenaltyExp, recordStartTime;

    std::string pRetMessage, inputFileName, inputPath, outputPath;

    CVX_SimGA Simulation;
    CVX_Environment Environment;
    CVX_Object Object;

    Environment.pObj = &Object;
    Simulation.pEnv = &Environment;
    Environment.AddObject(&Object);

    if(argc == 2 && std::string(argv[1]) == "--help"){
        printHelp();
        return -1;
    }
    else if(argc > 2){
        int var = 1;
        while(var < argc){
            if(std::string(argv[var]) == "-p"){ print = true;}
            else if(std::string(argv[var]) == "-q"){ quiet = true;}
            else if(std::string(argv[var]) ==  "-f"){ fitness = true; if(var < argc - 1){stringstream ss;ss << argv[++var];ss >> fitnessType;}else return -1;}
            else if(std::string(argv[var]) ==  "-fp"){ penalty = true; if(var < argc - 1){stringstream ss;ss << argv[++var];ss >> penaltyType;}else return -1;}
            else if(std::string(argv[var]) ==  "-fpe"){ penaltyExp = true; if(var < argc - 1){stringstream ss;ss << argv[++var];ss >> fitnessPenaltyExp;}else return -1;}
            else if(std::string(argv[var]) ==  "-b"){ behavior = true; if(var < argc - 1){stringstream ss;ss << argv[++var];ss >> behaviorType;}else return -1;}
            else if(std::string(argv[var]) ==  "-bi"){ interval = true; if(var < argc - 1){stringstream ss;ss << argv[++var];ss >> behaviorRecInterval;}else return -1;}
            else if(std::string(argv[var]) ==  "-bs"){ recordStart = true; if(var < argc - 1){stringstream ss;ss << argv[++var];ss >> recordStartTime;}else return -1;}
            else if(std::string(argv[var]) ==  "-o"){ outputFile = true; if(var < argc - 1){ outputPath = argv[++var];}else return -1; }
            else if(std::string(argv[var]) ==  "-i"){ input = true; if(var < argc - 1){ inputPath = argv[++var];}else return -1; }
            else{ cout << argv[var]; std::cerr << "Invalid argument\n"; return -1;}
            var++;
        }
        if(!input){std::cerr << "Error in input! type --help!\n"; return -1;}
        QString _Path = inputPath.c_str();
        bool isPathValid = Simulation.LoadVXAFile(_Path.toStdString().c_str(), &pRetMessage);
        if(!quiet)std::cout << "Loading simulation" << _Path.toStdString() << ".." <<  (isPathValid ? ".Done\n" : "False\n");
        if(!isPathValid){ std::cerr << "Error message: " <<  pRetMessage; return -1;}
    }
    else{std::cerr << "Error in input! type --help!\n"; return -1;}

    // Import environment inside simulation
    Simulation.Import(&Environment);

    // Fitness
    if(fitness) Simulation.FitnessType = static_cast<FitnessTypes>(fitnessType);
    else Simulation.FitnessType = FT_COM_DIST;
    // Penalty function to fitness
    if(penalty) Simulation.PenaltyType = static_cast<PenaltyTypes>(penaltyType);
    else Simulation.PenaltyType = PT_NONE;
    if(penaltyExp) Simulation.FitnessPenaltyExp = fitnessPenaltyExp;
    Simulation.UpdatePenalty();
    // Behavior
    if(behavior) Simulation.BehaviorType = static_cast<BehaviorTypes>(behaviorType);
    else Simulation.BehaviorType = BT_NONE;
    if(interval) Simulation.BehaviorRecordInterval = behaviorRecInterval;
    if(recordStart) Simulation.BehaviorNextRecTime = recordStartTime;

    //
    // Simulation main loop
    //
    if(!quiet) std::cout << "Starting simulation with fitness function: " + Simulation.getFitnessType() + "..";
    boost::timer timer;
    if(Simulation.pEnv->pObj->GetNumVox() > 2){ //Only proceed to simulation if enough voxels...
        Simulation.Simulated = true;
        while(Simulation.TimeStep(&pRetMessage))
        {
            if(!quiet && print && Simulation.CurStepCount%1000 == 0) { Simulation.UpdateFitness(); cout << Simulation.getStats().toStdString() << endl; }
            Simulation.UpdateBehaviorStats();
            Simulation.UpdatePenalty();
        }
        Simulation.UpdateFitness();
    }
    else // else simulation is not valid and output 0.0 fitness.
    {
        if(!quiet)std::cerr << "Warning, zero fitness, no more than two voxels...";
        pRetMessage = "Simulation Ended. Stop condition met.\n";
    }
    if(!quiet)std::cout << ".Done\n";
    double simulationTime = timer.elapsed();
    if(!quiet)std::cout << "Simulation took: " << simulationTime << " Seconds to complete.\n";

    //
    // Simulation output
    //
    std::vector<std::string> tokens;
    boost::split(tokens, inputPath, boost::is_any_of("/"));
    std::string file= tokens.at(tokens.size()-1);
    tokens.clear(); boost::split(tokens, file, boost::is_any_of("."));
    int k = 0;
    while(tokens.at(k) != "vxa")
    {
        if(k>0) inputFileName += ".";
        inputFileName += tokens.at(k++);
    }

    if(outputFile && pRetMessage == "Simulation Ended. Stop condition met.\n"){
        if(!quiet)std::cout << "Writing output file..";
        if(boost::filesystem::exists((outputPath == "." ? "":outputPath) + inputFileName + ".vfo"))
            boost::filesystem::remove((outputPath == "." ? "":outputPath) + inputFileName + ".vfo");

        Simulation.SaveResultFile(((outputPath == "." ? "":outputPath) + inputFileName + ".vfo").c_str());
        if(!quiet)std::cout << ".Done\n";
    }
    return 1;
}
