#ifndef __GENETICINDIVIDUALBEHAVIOR_H__
#define __GENETICINDIVIDUALBEHAVIOR_H__

#include "opencv2/opencv.hpp"
#include "opencv2/core/core.hpp"
#include <vector>
#include <assert.h>
#include "NEAT_Globals.h"

enum BehaviorPreprocessingType
{
    BPT_NONE,
    BPT_DFT,
    BPT_NOR
};

enum BehaviorSimilarityEvaluationType
{
    SET_NONE,
    SET_ABSDIF
};

struct behaviorAtype
{
    cv::Mat mat;
    int type;
};

namespace NEAT
{
class GeneticIndividualBehavior
{
public:
    GeneticIndividualBehavior();

    GeneticIndividualBehavior & operator= (const GeneticIndividualBehavior & other)
    {
        preProcessType = other.preProcessType;
        validBehavior = other.validBehavior;
        evaluationType = other.evaluationType;
        signatureVector = other.signatureVector;
        if(signatureVector.size() > 0) signature = &signatureVector[0];
        return *this;
    }

    void makeBehaviorFromStr(std::vector<string> strValueVector, int type);

    BehaviorPreprocessingType preProcessType;

    std::vector<behaviorAtype> signatureVector;

    behaviorAtype *signature;

    BehaviorSimilarityEvaluationType evaluationType;

    bool validBehavior;

    void DFT(cv::Mat &Behavior);

    void Normalize(cv::Mat &Behavior);

    void writeSignature(string filename, string postfix, bool all, std::vector<std::string> &outNames);

    void writeMatOfstream(cv::Mat mat, std::ofstream &stream);

    void preProcessing();

    double getDiff(GeneticIndividualBehavior other);
};
}


#endif
