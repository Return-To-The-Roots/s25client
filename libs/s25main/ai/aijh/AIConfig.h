#ifndef CONFIG_H
#define CONFIG_H
#include "helpers/EnumArray.h"
#include "gameTypes/BuildingType.h"
#include "gameTypes/GoodTypes.h"
#include "gameTypes/StatisticTypes.h"

#include <yaml-cpp/yaml.h>

struct Logarithmic
{
    double constant = 0.0;
    double linear = 0.0;
};
struct BuildParams
{
    double constant = 0.0;
    double linear = 0.0;
    Logarithmic logTwo = {0.0, 0.0};
    Logarithmic logE = {0.0, 0.0};
    unsigned min = 0;
    unsigned max = 99999;
};

struct WantedParams
{
    bool enables = false;
    helpers::EnumArray<BuildParams, BuildingType> bldWeights = helpers::EnumArray<BuildParams, BuildingType>{};
    helpers::EnumArray<BuildParams, GoodType> goodWeights = helpers::EnumArray<BuildParams, GoodType>{};
    BuildParams workersAdvance = {1};
    unsigned max = 10000;
    unsigned minProductivity = 0;
    helpers::EnumArray<BuildParams, StatisticType> statsWeights = helpers::EnumArray<BuildParams, StatisticType>{};
};

struct AIConfig
{
    double foresterWoodLevel = 200.0;
    double startupMilBuildings = 15.0;
    double farmToIronMineRatio = 3;
    double woodcutterToStorehouseRatio = 2.0;
    double woodcutterToWoodRatio = 2500;
    double breweryToArmoryRatio = 5;
    double pigfarmMultiplier = 0.0;
    double maxMetalworks = 4.0;
    double foresterFreeRadius = 4.0;
    BuildParams sawmillToWoodcutter = {1, 2};
    BuildParams milToFarm = {0, 0.95};
    BuildParams wellToUsers = {0, 1.2};
    BuildParams freeFarmToMill = {0.0, 0.85};
    BuildParams milToSawmill = {4.0, 0.1, {0.0, 2}};
    BuildParams milToForester = {0.1, 0.025, {2.0, 0.25}};
    BuildParams sawmillToForester = {1, 0.25 };
    BuildParams ironsmelterToMetalworks = {1.0, 0.00, {1.0, 1.0}};
    BuildParams ironMineToIronsmelter = {0.0, 1.0};
    BuildParams breweryToArmory = {1.0, 0.2};

    BuildParams startupMilToSawmill = {3.0, 0.4};
    BuildParams startupMilToWoodcutter = {3.0, 0.4};

    helpers::EnumArray<WantedParams, BuildingType> wantedParams;
};

extern AIConfig AI_CONFIG;

extern BuildParams parseBuildParams(const YAML::Node& node, const BuildParams& defaults);
extern WantedParams parseWantedParams(const YAML::Node& node, WantedParams params);
extern WantedParams parseWeights(const YAML::Node& rootNode);

extern void initDefaults();
extern void applyWeightsCfg(std::string weightCfgPath);
extern void initAIConfig(std::string configPath);

#endif