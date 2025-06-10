#ifndef CONFIG_H
#define CONFIG_H
#include "WeightParams.h"
#include "helpers/EnumArray.h"
#include "gameTypes/BuildingType.h"
#include "gameTypes/GoodTypes.h"

#include <yaml-cpp/yaml.h>

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

    helpers::EnumArray<WantedParams, BuildingType> wantedParams;
    helpers::EnumArray<LocationParams, BuildingType> locationParams;
};

extern AIConfig AI_CONFIG;

extern BuildParams parseBuildParams(const YAML::Node& node, const BuildParams& defaults);
extern WantedParams parseWantedParams(const YAML::Node& node, WantedParams params);
extern WantedParams parseWeights(const YAML::Node& rootNode);

extern void initDefaults();
extern void applyWeightsCfg(std::string weightCfgPath);
extern void applyBldPlannerCfg(YAML::Node plannerNode);
extern void applyPosFinderCfg(YAML::Node posFinder);
extern void initAIConfig(std::string configPath);

#endif