#include "VoxBotCreator.h"

VoxBotCreator::VoxBotCreator()
{
    IsLoaded = false;
}

bool VoxBotCreator::LoadVXASimulation(std::string filename)
{
    // connect environment to simulation and object to environment
    Environment.pObj = &Object;
    this->Simulation.pEnv = &Environment;
    Environment.AddObject(&Object);

    // load simulation
    std::string pRetMessage;
    bool IsLoaded = Simulation.LoadVXAFile(filename, &pRetMessage);
    if(!IsLoaded) return false;

    // Import environment into simulation
    this->Simulation.Import(&Environment);
    return IsLoaded;
}
