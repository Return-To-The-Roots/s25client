#ifndef CONFIG_H
#define CONFIG_H
#include "WeightParams.h"
#include "helpers/EnumArray.h"
#include "gameTypes/BuildingType.h"
#include "gameTypes/GoodTypes.h"

#include <yaml-cpp/yaml.h>

struct AIConfig
{
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