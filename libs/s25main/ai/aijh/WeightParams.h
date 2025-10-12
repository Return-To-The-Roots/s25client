//
// Created by pavel on 10.06.25.
//

#ifndef WEIGHTPARAMS_H
#define WEIGHTPARAMS_H
#include "ai/AIResource.h"
#include "helpers/EnumArray.h"
#include "gameTypes/BuildingType.h"
#include "gameTypes/GoodTypes.h"
#include "gameTypes/StatisticTypes.h"
#include <cmath>

struct Logarithmic
{
    double constant = 0.0;
    double linear = 0.0;
};
struct BuildParams
{
    double constant = 0.0;
    double linear = 0.0;
    double exponential = 0.0;
    Logarithmic logTwo = {0.0, 0.0};
    Logarithmic logE = {0.0, 0.0};
    unsigned min = 0;
    unsigned max = 99999;
    bool enabled = false;
};

struct RatingParams
{
    bool enabled = false;
};

struct ProximityParams
{
    BuildParams minimal = {};
    mutable bool enabled = false;
};

struct LocationParams
{
    bool enabled = false;
    helpers::EnumArray<ProximityParams, BuildingType> proximity = helpers::EnumArray<ProximityParams, BuildingType>{};
    RatingParams ratingParams = {false};
};

struct WantedParams
{
    bool enabled = false;
    helpers::EnumArray<BuildParams, BuildingType> bldWeights = helpers::EnumArray<BuildParams, BuildingType>{};
    helpers::EnumArray<BuildParams, GoodType> goodWeights = helpers::EnumArray<BuildParams, GoodType>{};
    helpers::EnumArray<BuildParams, StatisticType> statsWeights = helpers::EnumArray<BuildParams, StatisticType>{};
    helpers::EnumArray<BuildParams, AIResource> resourceWeights = helpers::EnumArray<BuildParams, AIResource>{};
    BuildParams workersAdvance = {1};
    unsigned max = 10000;
    unsigned minProductivity = 0;
    BuildParams productivity = {};
};

namespace CALC {
    double calcCount(unsigned x, BuildParams params);
}
#endif //WEIGHTPARAMS_H
