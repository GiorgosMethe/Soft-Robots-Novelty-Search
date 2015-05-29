#include "NEAT_GeneticIndividualBehavior.h"

namespace NEAT
{
GeneticIndividualBehavior::GeneticIndividualBehavior()
{
    validBehavior = false;
    preProcessType = BPT_NONE;
    evaluationType = SET_ABSDIF;
    signatureVector.clear();
}

void GeneticIndividualBehavior::makeBehaviorFromStr(std::vector<string> strValueVector, int type)
{
    cv::Mat tmpSignature;
    for (unsigned int i = 0; i < strValueVector.size(); ++i) {
        std::string valueStr = strValueVector.at(i);
        std::vector<std::string> tokens;
        boost::split(tokens, valueStr, boost::is_any_of(","));
        if(tokens.size() == 3)
        {
            cv::Vec3d tmp;
            tmp[0] = std::atof(tokens.at(0).c_str());
            tmp[1] = std::atof(tokens.at(1).c_str());
            tmp[2] = std::atof(tokens.at(2).c_str());
            tmpSignature.push_back(tmp);
        }
        else if(tokens.size() == 2)
        {
            cv::Vec2d tmp;
            tmp[0] = std::atof(tokens.at(0).c_str());
            tmp[1] = std::atof(tokens.at(1).c_str());
            tmpSignature.push_back(tmp);
        }
        else if(tokens.size() == 1 && strValueVector.size() != 1)
        {
            double tmp = std::atof(valueStr.c_str());
            tmpSignature.push_back(tmp);
        }
        else if(tokens.size() == 1 && strValueVector.size() == 1)
        {
            for (unsigned i=0; i<valueStr.length(); ++i)
            {
                std::stringstream ss; ss << valueStr.at(i);
                double a;
                ss >> a;
                tmpSignature.push_back(a);
            }
        }
        else
        {
            std::cerr << "ERROR : UNKNOWN TYPE OF SIGNATURE TYPE" << std::endl;
            exit(77);
        }
        tokens.clear();
    }
    behaviorAtype tmp;
    tmp.mat = tmpSignature;
    tmp.type = type;
    signatureVector.push_back(tmp);
    signature = &signatureVector[0];
    return;
}


void GeneticIndividualBehavior::writeSignature(string filename, string postfix, bool all, std::vector<std::string> &outNames)
{
    if(all && validBehavior)
    {
        for(int i = 0; i < signatureVector.size(); i++)
        {
            std::stringstream ss;
            ss << signatureVector[i].type;
            string filename1 = filename + "--Behavior_" + ss.str();
            ofstream signatureFile((filename1 + postfix).c_str(), ios::out);
            if(signatureFile.is_open()) writeMatOfstream(signatureVector[i].mat, signatureFile);
            signatureFile.close();
            outNames.push_back(filename1);
        }
    }
    else if(validBehavior)
    {
        ofstream signatureFile((filename + postfix).c_str(), ios::out);
        if(signatureFile.is_open() && validBehavior) writeMatOfstream(signature->mat, signatureFile);
        signatureFile.close();
        outNames.push_back(filename);
    }
    return;
}

void GeneticIndividualBehavior::writeMatOfstream(cv::Mat mat, std::ofstream &stream)
{
    int a = mat.rows;
    if((int)NEAT::Globals::getSingleton()->getParameterValue("DFTsimilarityLength") > 0 &&
            (int)NEAT::Globals::getSingleton()->getParameterValue("DFTsimilarityLength") < mat.rows)
        a = (int)NEAT::Globals::getSingleton()->getParameterValue("DFTsimilarityLength");

    for (int var = 0; var < a; ++var){
        if(mat.type() == CV_64FC3)
            stream << mat.at<cv::Vec3d>((int) var)[0] << "," << mat.at<cv::Vec3d>((int) var)[1] << "," << mat.at<cv::Vec3d>((int) var)[2] << "\n";
        else if(mat.type() == CV_64FC2)
            stream << mat.at<cv::Vec2d>((int) var)[0] << "," << mat.at<cv::Vec2d>((int) var)[1] << "\n";
        else if(mat.type() == CV_64F || mat.type() == CV_32F)
            stream << mat.at<double>((int) var) << "\n";
        else{}
    }
    return;
}

void GeneticIndividualBehavior::preProcessing()
{
    if(preProcessType == BPT_DFT && validBehavior)
    {
        DFT(signature->mat);
    }
    else if(preProcessType == BPT_NOR && validBehavior)
    {
        Normalize(signature->mat);
    }
}

void GeneticIndividualBehavior::DFT(cv::Mat &source)
{
    if(source.type() != CV_64F && source.type() != CV_32F)
    {
        std::cerr << "ERROR : DFT CANNOT HANDLE DATA PASSED" << std::endl;
        exit(79);
    }
    cv::Mat target;
    int optimalDftSize = cv::getOptimalDFTSize(source.rows);
    target = cvCreateMat(optimalDftSize, 1, CV_32F);
    cv::dft(source, target);
    source = target.clone();
    return;
}

void GeneticIndividualBehavior::Normalize(cv::Mat &source)
{
    if(source.type() == CV_64FC2)
    {
        // Normalize data points, each new point will be the average of each 7 neighbors
        // -3, -2, -1, 0, +1, +2, +3
        cv::Mat tmpNormalized;
        for (int i = 3; i < source.rows-3; i++)
        {
            cv::Vec2d center(0.0,0.0);
            for(int j = -3; j < 3; j++)
            {
                center[0] += source.at<cv::Vec2d>((int) i+j)[0];
                center[1] += source.at<cv::Vec2d>((int) i+j)[1];
            }
            tmpNormalized.push_back(cv::Vec2d(center[0] / 7, center[1] / 7));
        }
        source = tmpNormalized.clone();
        // subtract the first points coordinates from all points.
        // point(x) -= point(0)
        double fX = source.at<cv::Vec2d>((int) 0)[0];
        double fY = source.at<cv::Vec2d>((int) 0)[1];
        cv::Vec2d center(0.0,0.0);
        for (int i = 0; i < source.rows; ++i)
        {
            source.at<cv::Vec2d>((int) i)[0] -= fX;
            source.at<cv::Vec2d>((int) i)[1] -= fY;
            center[0] += source.at<cv::Vec2d>((int) i)[0];
            center[1] += source.at<cv::Vec2d>((int) i)[1];
        }
        // Center of mass of the points
        center[0] /= (double)source.rows;
        center[1] /= (double)source.rows;
        // Check angle the center point of the cloud has in respect to the axis start x,y for this case
        double angle = atan2(center[0], center[1]);
        // rotate data points
        for (int i = 0; i < source.rows; ++i)
        {
            double x = source.at<cv::Vec2d>((int) i)[0];
            double y = source.at<cv::Vec2d>((int) i)[1];
            double xP = x * cos(angle) - y * sin(angle);
            double yP = x * sin(angle) + y * cos(angle);
            source.at<cv::Vec2d>((int) i)[0] = xP;
            source.at<cv::Vec2d>((int) i)[1] = yP;
        }
    }
    else if(source.type() == CV_64FC3)
    {
        // Normalize data points, each new point will be the average of each 7 neighbors
        // -3, -2, -1, 0, +1, +2, +3
        cv::Mat tmpNormalized;
        for (int i = 3; i < source.rows-3; i++)
        {
            cv::Vec3d center(0.0, 0.0, 0.0);
            for(int j = -3; j < 3; j++)
            {
                center[0] += source.at<cv::Vec3d>((int) i+j)[0];
                center[1] += source.at<cv::Vec3d>((int) i+j)[1];
                center[2] += source.at<cv::Vec3d>((int) i+j)[2];
            }
            tmpNormalized.push_back(cv::Vec3d(center[0] / 7, center[1] / 7, center[2] / 7));
        }
        source = tmpNormalized.clone();

//        // subtract the first points coordinates from all points.
//        // point(x) -= point(0)
        double fX = source.at<cv::Vec3d>((int) 0)[0];
        double fY = source.at<cv::Vec3d>((int) 0)[1];
        double fZ = source.at<cv::Vec3d>((int) 0)[2];
        cv::Vec3d center(0.0, 0.0, 0.0);
        for (int i = 0; i < source.rows; ++i)
        {
            source.at<cv::Vec3d>((int) i)[0] -= fX;
            source.at<cv::Vec3d>((int) i)[1] -= fY;
            source.at<cv::Vec3d>((int) i)[2] -= fZ;
            center[0] += source.at<cv::Vec3d>((int) i)[0];
            center[1] += source.at<cv::Vec3d>((int) i)[1];
            center[2] += source.at<cv::Vec3d>((int) i)[2];
        }
        // Center of mass of the points.
        center[0] /= (double)source.rows;
        center[1] /= (double)source.rows;
        center[2] /= (double)source.rows;

        // Check angle the center point of the cloud has in respect to the axis start x,y for this case
        double angle = atan2(center[0], center[1]);
        // rotate data points
        for (int i = 0; i < source.rows; ++i)
        {
            double x = source.at<cv::Vec3d>((int) i)[0];
            double y = source.at<cv::Vec3d>((int) i)[1];
            double xP = x * cos(angle) - y * sin(angle);
            double yP = x * sin(angle) + y * cos(angle);
            source.at<cv::Vec3d>((int) i)[0] = xP;
            source.at<cv::Vec3d>((int) i)[1] = yP;
        }
    }
    else
    {
        std::cerr << "ERROR : TYPE OF DATA CAN NOT BE HANDLED BY NORMALIZATION FUNCTION" << std::endl;
        exit(79);
    }
    return;
}

double GeneticIndividualBehavior::getDiff(GeneticIndividualBehavior other)
{
    if(!validBehavior || !other.validBehavior)
    {
        return 0.0; // comparing two individuals from which one has no behavior, return 0.0
    }
    else if(signature->mat.type() != other.signature->mat.type())
    {
        std::cerr << "ERROR, INDIVIDUALS HAVE DIFFERENT BEHAVIOR SIGNATURE TYPES" << std::endl;
        exit(79);
    }
    else if(evaluationType == SET_NONE)
    {
        return 0.0;
    }
    else if(evaluationType == SET_ABSDIF)
    {
        if(signature->mat.type() == CV_64FC3)
        {
            double diff = 0.0;
            for(int i = 0; i < signature->mat.rows; i++)
            {
                cv::Vec3d diffV3d = signature->mat.at<cv::Vec3d>((int) i) - other.signature->mat.at<cv::Vec3d>((int) i);
                diff += sqrt(pow(diffV3d[0],2)+pow(diffV3d[1],2)+pow(diffV3d[2],2));
            }
            return diff;
        }
        else if(signature->mat.type() == CV_64FC2)
        {
            double diff = 0.0;
            for(int i = 0; i < signature->mat.rows; i++)
            {
                cv::Vec2d diffV2d = signature->mat.at<cv::Vec2d>((int) i) - other.signature->mat.at<cv::Vec2d>((int) i);
                diff += sqrt(pow(diffV2d[0],2)+pow(diffV2d[1],2));
            }
            return diff;
        }
        else if(signature->mat.type() == CV_64F || signature->mat.type() == CV_32F)
        {
            int a = signature->mat.rows;
            if((int)Globals::getSingleton()->getParameterValue("DFTsimilarityLength") > 0 &&
                    (int)NEAT::Globals::getSingleton()->getParameterValue("DFTsimilarityLength") < signature->mat.rows)
                a = (int)NEAT::Globals::getSingleton()->getParameterValue("DFTsimilarityLength");

            double diff = 0.0;
            for(int i = 0; i < a; i++)
            {
                diff += abs(signature->mat.at<double>((int) i) - other.signature->mat.at<double>((int) i));
            }
            return diff;
        }
        else
        {
            cout << signature->mat.type() << endl;
            std::cerr << "ERROR : DATA TYPE IS INVALID FOR THIS OPERATION SET_ABSDIF" << std::endl;
            exit(79);
        }
    }
    else
    {
        std::cerr << "ERROR, UNKNOWN TYPE OF SIMILARITY TYPE" << std::endl;
        exit(79);
    }
}
}
