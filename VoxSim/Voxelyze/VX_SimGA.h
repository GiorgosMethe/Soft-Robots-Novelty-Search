/*******************************************************************************
Copyright (c) 2010, Jonathan Hiller (Cornell University)
If used in publication cite "J. Hiller and H. Lipson "Dynamic Simulation of Soft Heterogeneous Objects" In press. (2011)"

This file is part of Voxelyze.
Voxelyze is free software: you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
Voxelyze is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
See <http://www.opensource.org/licenses/lgpl-3.0.html> for license details.
*******************************************************************************/

#ifndef VX_SIMGA_H
#define VX_SIMGA_H

//wrapper class for VX_Sim with convenience functions and nomenclature for using Voxelyze within a genetic algorithm.

#include "VX_Sim.h"
#include <QString>

enum FitnessTypes{
    FT_NONE,
    FT_COM_DIST,
    FT_COM_DIST_XY,
    FT_COM_TOTAL_TRAJ_LENGTH
};

enum PenaltyTypes{
    PT_NONE,
    PT_VOX_NUM,
    PT_ACT_VOX_NUM,
    PT_CONN_NUM,
    PT_TOUCH_GROUND
};

enum BehaviorTypes{
    BT_NONE,
    BT_TRAJECTORY,
    BT_TRAJECTORY_XY,
    BT_PACE,
    BT_SHAPE,
    BT_VOXELS_GROUND,
    BT_KINETIC_ENERGY,
    BT_PRESSURE,
    BT_ALL // Only for experimental purposes
};

class CVX_SimGA : public CVX_Sim
{
public:
    CVX_SimGA();
    ~CVX_SimGA(){}

    void SaveResultFile(std::string filename);
    void WriteResultFile(CXML_Rip* pXML);

    void WriteAdditionalSimXML(CXML_Rip* pXML);
    bool ReadAdditionalSimXML(CXML_Rip* pXML);

    void WriteResultFileBehavior(BehaviorTypes tmp, CXML_Rip* pXML);

    QString getStats();

    std::string getFitnessType()
    {
        switch (this->FitnessType)
        {
        case FT_NONE:return "FT_NONE";break;
        case FT_COM_DIST:return "FT_COM_DIST";break;
        case FT_COM_DIST_XY:return "FT_COM_DIST_XY";break;
        case FT_COM_TOTAL_TRAJ_LENGTH:return "FT_COM_TOTAL_TRAJ_LENGTH";break;
        default: return "FT_UNKNOWN"; break;
        }
    }

    void UpdateFitness();
    void UpdatePenalty();
    void UpdateBehaviorStats();

    float Fitness;	//!<Keeps track of whatever fitness we choose to track
    FitnessTypes FitnessType; //!<Holds the fitness reporting type. For now =0 tracks the center of mass, =1 tracks a particular Voxel number
    PenaltyTypes PenaltyType;
    BehaviorTypes BehaviorType;
    int	TrackVoxel;		//!<Holds the particular voxel that will be tracked (if used).
    std::string FitnessFileName;	//!<Holds the filename of the fitness output file that might be used
    bool WriteFitnessFile;
    bool Simulated;
    int ActuatedVoxelNum;
    int ConnectionsNum;
    int TotalVoxelNum;
    long int GroundPenalty;
    double FitnessPenaltyExp;
    double FitnessPenalty;
    double BehaviorNextRecTime;
    double BehaviorRecordInterval;

    std::vector< Vec3D<> > trajectory;
    std::vector<double> paceHistory;
    std::vector<int> groundVoxelHistory;
    std::vector<double> kineticEnergyHistory;
    std::vector<double> maxPressureHistory;
};

#endif //VX_SIMGA_H
