/*******************************************************************************
Copyright (c) 2010, Jonathan Hiller (Cornell University)
If used in publication cite "J. Hiller and H. Lipson "Dynamic Simulation of Soft Heterogeneous Objects" In press. (2011)"

This file is part of Voxelyze.
Voxelyze is free software: you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
Voxelyze is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
See <http://www.opensource.org/licenses/lgpl-3.0.html> for license details.
*******************************************************************************/

#include "VX_SimGA.h"

CVX_SimGA::CVX_SimGA()
{
    Fitness = 0.0f;
    TrackVoxel = 0;
    ActuatedVoxelNum = 0;
    ConnectionsNum = 0;
    GroundPenalty = 0;
    TotalVoxelNum = 0;
    FitnessPenaltyExp = 1.5;
    BehaviorRecordInterval = 0.01;
    BehaviorNextRecTime = 0.0;
    FitnessPenalty = 0.0;
    FitnessFileName = "";
    WriteFitnessFile = false;
    FitnessType = FT_NONE;	//no reporting is default
    PenaltyType = PT_NONE; //no reporting fitness penalty is default
    BehaviorType = BT_NONE; //no reporting behavior is default
    Simulated = false;
}

void CVX_SimGA::UpdateFitness()
{
    switch (FitnessType) {
    case FT_NONE:
        Fitness = 0.0f;
        break;
    case FT_COM_DIST: // body lengths travelled
        Fitness = ((SS.CurCM-IniCM).Length()  / (pEnv->pObj->GetVXDim() * pEnv->pObj->GetLatticeDim()));
        break;
    case FT_COM_DIST_XY: // body lengths only in respect to the ground plane
        Fitness = ((SS.CurCM.NormalizeOnAxis(Vec3D<int>(1,1,0))-IniCM.NormalizeOnAxis(Vec3D<int>(1,1,0))) / (pEnv->pObj->GetVXDim() * pEnv->pObj->GetLatticeDim()));
        break;
    case FT_COM_TOTAL_TRAJ_LENGTH:
        Fitness = (SS.TotalObjTrajectoryLength / 1000.0) / (pEnv->pObj->GetVXDim() * pEnv->pObj->GetLatticeDim());
        break;
    default:
        break;
    }
}

void CVX_SimGA::UpdateBehaviorStats()
{
    if(CurTime >= BehaviorNextRecTime && BehaviorType != BT_NONE)
    {
        switch (BehaviorType) {
        case BT_PACE:
            paceHistory.push_back((SS.CurCM-SS.LastCM).Length() / dt);
            break;
        case BT_TRAJECTORY:
        case BT_TRAJECTORY_XY:
            trajectory.push_back(SS.CurCM);
            break;
        case BT_KINETIC_ENERGY:
            kineticEnergyHistory.push_back(SS.TotalObjKineticE);
            break;
        case BT_PRESSURE:
            maxPressureHistory.push_back(SS.MaxPressure);
            break;
        case BT_VOXELS_GROUND:
        {
            int NumTouching = 0;
            int LocNumVox = pEnv->pObj->GetNumVox();
            for (int i=0; i<LocNumVox; i++){
                if (VoxArray[i].GetCurGroundPenetration() > 0){ NumTouching++; }
            }
            groundVoxelHistory.push_back(NumTouching);
            break;
        }
        case BT_ALL:
        {
            paceHistory.push_back((SS.CurCM-SS.LastCM).Length() / dt);
            trajectory.push_back(SS.CurCM);
            int NumTouching = 0;
            int LocNumVox = pEnv->pObj->GetNumVox();
            for (int i=0; i<LocNumVox; i++){
                if (VoxArray[i].GetCurGroundPenetration() > 0){ NumTouching++; }
            }
            groundVoxelHistory.push_back(NumTouching);
            break;
        }
        default:
            break;
        }
        BehaviorNextRecTime += BehaviorRecordInterval;
    }
}


void CVX_SimGA::UpdatePenalty()
{
    int num_act = 0;
    switch (PenaltyType) {
    case PT_NONE:
        FitnessPenalty = 0.0;
        break;
    case PT_VOX_NUM:
        FitnessPenalty = pow(((double)pEnv->pObj->GetNumVox() / ((double)pEnv->pObj->GetVXDim()*(double)pEnv->pObj->GetVYDim()*(double)pEnv->pObj->GetVZDim())), FitnessPenaltyExp);
        break;
    case PT_ACT_VOX_NUM:
        for (int x = 0; x < pEnv->pObj->GetVXDim(); ++x)
        {
            for (int y = 0; y < pEnv->pObj->GetVYDim(); ++y)
            {
                for (int z = 0; z < pEnv->pObj->GetVZDim(); ++z)
                {
                    int tmpIndex = pEnv->pObj->GetMat(pEnv->pObj->GetIndex(x,y,z));
                    if(pEnv->pObj->GetBaseMat(tmpIndex)->GetCTE() != 0.0) num_act++;
                }
            }
        }
        FitnessPenalty = pow(((double)num_act / (double)pEnv->pObj->GetNumVox()), FitnessPenaltyExp);
        break;
    case PT_CONN_NUM:
        FitnessPenalty = pow(((double)pEnv->pObj->Structure.GetConnectionsNum() / (6 * (double)pEnv->pObj->GetNumVox())),FitnessPenaltyExp);
        break;
    case PT_TOUCH_GROUND:
    {
        int NumTouching = 0;
        int LocNumVox = pEnv->pObj->GetNumVox();
        for (int i=0; i<LocNumVox; i++){
            if (VoxArray[i].GetCurGroundPenetration() > 0){ NumTouching++; }
        }
        int NewSum = GroundPenalty + NumTouching;
        if(NewSum > GroundPenalty)
        {
            FitnessPenalty = 0.0;
        }
        else
        {
            FitnessPenalty = std::min(FitnessPenalty + (dt / GetStopConditionValue()) , 1.0);
        }
        GroundPenalty = NewSum;
        break;
    }
    default:
        break;
    }
}

QString CVX_SimGA::getStats()
{
    QString tmp;
    tmp = "Time =" + QString:: number(CurTime, 'f', 1) + " sec" +
            " | StepNum =" + QString::number(CurStepCount) +
            " | Temp = " + QString::number(pEnv->GetCurTemp(), 'f', 1)+ " C" +
            " | Step = " + QString::number(dt * 1000000, 'f', 1)+ " nSec";

    tmp += " | Fit = "+ QString::number(Fitness, 'f', 1);
    return tmp;
}

void CVX_SimGA::SaveResultFile(std::string filename)
{
    CXML_Rip XML;
    WriteResultFile(&XML);
    XML.SaveFile(filename);
}

void CVX_SimGA::WriteResultFileBehavior(BehaviorTypes tmp, CXML_Rip* pXML)
{
    pXML->DownLevel("Behavior");
    pXML->Element("BehaviorType", (int)tmp);
    double time = 0.0;
    if(tmp == BT_PACE)
    {
        for (unsigned int i = 0; i < paceHistory.size(); ++i)
        {
            pXML->DownLevel("Record");
            pXML->Element("Time", time);
            pXML->Element("Value", paceHistory.at(i));
            pXML->UpLevel();
            time += BehaviorRecordInterval;
        }
    }
    else if(tmp == BT_TRAJECTORY)
    {
        for (unsigned int i = 0; i < trajectory.size(); ++i)
        {
            pXML->DownLevel("Record");
            pXML->Element("Time", time);
            std::ostringstream value;
            value << trajectory.at(i).getX() << "," << trajectory.at(i).getY() << "," << trajectory.at(i).getZ();
            pXML->Element("Value", value.str());
            pXML->UpLevel();
            time += BehaviorRecordInterval;
        }
    }
    else if(tmp == BT_TRAJECTORY_XY)
    {
        for (unsigned int i = 0; i < trajectory.size(); ++i)
        {
            pXML->DownLevel("Record");
            pXML->Element("Time", time);
            std::ostringstream value;
            value << trajectory.at(i).getX() << "," << trajectory.at(i).getY();
            pXML->Element("Value", value.str());
            pXML->UpLevel();
            time += BehaviorRecordInterval;
        }
    }else if(tmp == BT_VOXELS_GROUND)
    {
        for (unsigned int i = 0; i < groundVoxelHistory.size(); ++i)
        {
            pXML->DownLevel("Record");
            pXML->Element("Time", time);
            pXML->Element("Value", groundVoxelHistory.at(i));
            pXML->UpLevel();
            time += BehaviorRecordInterval;
        }
    }
    else if(tmp == BT_SHAPE)
    {
        pXML->DownLevel("Record");
        pXML->Element("Time", time);
        std::ostringstream value;
        for (int z = 0; z < pEnv->pObj->GetVZDim(); ++z){
            for (int y = 0; y < pEnv->pObj->GetVYDim(); ++y){
                for (int x = 0; x < pEnv->pObj->GetVXDim(); ++x){
                    int tmpIndex = pEnv->pObj->GetMat(pEnv->pObj->GetIndex(x,y,z));
                    value << (tmpIndex != 0);
                }
            }
        }
        pXML->Element("Value", value.str());
        pXML->UpLevel();
    }
    else if(tmp == BT_KINETIC_ENERGY)
    {
        for (unsigned int i = 0; i < kineticEnergyHistory.size(); ++i)
        {
            pXML->DownLevel("Record");
            pXML->Element("Time", time);
            pXML->Element("Value", kineticEnergyHistory.at(i));
            pXML->UpLevel();
            time += BehaviorRecordInterval;
        }
    }
    else if(tmp == BT_PRESSURE)
    {
        for (unsigned int i = 0; i < maxPressureHistory.size(); ++i)
        {
            pXML->DownLevel("Record");
            pXML->Element("Time", time);
            pXML->Element("Value", maxPressureHistory.at(i));
            pXML->UpLevel();
            time += BehaviorRecordInterval;
        }
    }
    pXML->UpLevel();
}

void CVX_SimGA::WriteResultFile(CXML_Rip* pXML)
{
    pXML->DownLevel("Voxelyze_Sim_Result");
    pXML->SetElAttribute("Version", "1.0");
    pXML->Element("Valid", (int)Simulated);

    if(Simulated)
    {
        pXML->DownLevel("Fitness");
        pXML->Element("FitnessType", (int)FitnessType);
        pXML->Element("Value", Fitness);
        pXML->UpLevel();

        pXML->DownLevel("Penalty");
        pXML->Element("PenaltyType", (int)PenaltyType);
        pXML->Element("Value", FitnessPenalty);
        pXML->UpLevel();

        // EXPERIMENTAL CODE...
        if(BehaviorType == BT_ALL){
            for ( int i = BT_NONE; i != BT_ALL; i++){
                BehaviorTypes bT = static_cast<BehaviorTypes>(i);
                if(bT != BT_ALL && bT != BT_NONE) WriteResultFileBehavior(bT, pXML);
            }
        }
        else if (BehaviorType != BT_NONE){
            WriteResultFileBehavior(BehaviorType, pXML);
        }
    }
    pXML->UpLevel();
}

void CVX_SimGA::WriteAdditionalSimXML(CXML_Rip* pXML)
{
    pXML->DownLevel("GA");
    pXML->Element("Fitness", Fitness);
    pXML->Element("FitnessType", (int)FitnessType);
    pXML->Element("TrackVoxel", TrackVoxel);
    pXML->Element("FitnessFileName", FitnessFileName);
    pXML->Element("WriteFitnessFile", WriteFitnessFile);
    pXML->UpLevel();
}

bool CVX_SimGA::ReadAdditionalSimXML(CXML_Rip* pXML)
{
    if (pXML->FindElement("GA")){
        int TmpInt;
        if (!pXML->FindLoadElement("Fitness", &Fitness)) Fitness = 0;
        if (pXML->FindLoadElement("FitnessType", &TmpInt)) FitnessType=(FitnessTypes)TmpInt; else Fitness = 0;
        if (!pXML->FindLoadElement("TrackVoxel", &TrackVoxel)) TrackVoxel = 0;
        if (!pXML->FindLoadElement("FitnessFileName", &FitnessFileName)) FitnessFileName = "";
        if (!pXML->FindLoadElement("WriteFitnessFile", &WriteFitnessFile)) WriteFitnessFile = true;
        pXML->UpLevel();
    }

    return true;
}
