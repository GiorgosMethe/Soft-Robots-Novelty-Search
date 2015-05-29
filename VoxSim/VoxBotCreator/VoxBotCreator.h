#ifndef VOXBOTCREATOR_H
#define VOXBOTCREATOR_H

#include "../Voxelyze/VX_Sim.h"
#include <string>
#include <vector>
#include <iostream>

class VoxBotCreator
{

public:

    VoxBotCreator();

    // load a simulation
    bool LoadVXASimulation(std::string filename);

    // write a simulation with the made VoxBot
    inline void writeVXASimulation(std::string filename)
    {
        this->Simulation.SaveVXAFile(filename);
    }

    // insert a VoxBot
    inline bool InsertVoxBot(std::vector<int> input)
    {
        for(unsigned int i = 0; i < input.size(); i++)
            this->Simulation.pEnv->pObj->Structure.SetData(i, input.at(i));
        if(this->Simulation.pEnv->pObj->GetNumVox() == 0) return false;
        return true;
    }

    // returns the number of materials are available.
    inline int GetNumMaterials()
    {
        return this->Simulation.pEnv->pObj->GetNumMaterials();
    }

    // clear the structure, every voxel is set to 0.
    void clearStructure()
    {
        this->Simulation.pEnv->pObj->Structure.ResetStructure();
    }

    // resize structure
    void resizeStructure(int x, int y, int z)
    {
        this->Simulation.pEnv->pObj->Structure.Resize(x,y,z);
    }

    // Disconnect structure
    inline void RemoveDisconnected()
    {
        this->Simulation.pEnv->RemoveDisconnected();
    }

    // get the dimensions of the structure
    inline int getXDim()
    {
        return this->Simulation.pEnv->pObj->GetVXDim();
    }

    inline int getYDim()
    {
        return this->Simulation.pEnv->pObj->GetVYDim();
    }

    inline int getZDim()
    {
        return this->Simulation.pEnv->pObj->GetVZDim();
    }

    //set Object properties CTE, density, poissons, matTempPhase
    inline void setMatProperties(int matIndex, double cte, double density, double poissons, double matTempPhase)
    {
        Simulation.pEnv->pObj->GetBaseMat(matIndex)->SetCTE(cte);
        Simulation.pEnv->pObj->GetBaseMat(matIndex)->SetDensity(density);
        Simulation.pEnv->pObj->GetBaseMat(matIndex)->SetPoissonsRatio(poissons);
        Simulation.pEnv->pObj->GetBaseMat(matIndex)->SetMatTempPhase(matTempPhase);
    }

private:

    bool IsLoaded;
    CVX_Sim Simulation;
    CVX_Environment Environment;
    CVX_Object Object;
};

#endif // VOXBOTCREATOR_H
