#ifndef CONFIG_H
#define CONFIG_H
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
};

struct AIConfig
{
    double foresterWoodLevel = 100.0;
    double startupMilBuildings = 15.0;
    double farmToIronMineRatio = 2.5;
    double woodcutterToStorehouseRatio = 2.0;
    double woodcutterToWoodRatio = 2500;
    double breweryToArmoryRatio = 5;
    double ironsmelterToIronMineRatio = 0.9;
    double pigfarmMultiplier = 0.0;
    double maxMetalworks = 2.0;
    double foresterFreeRadius = 4.0;
    BuildParams sawmillToWoodcutter = {1, 2};
    BuildParams farmToMil = {0, 0.7};
    BuildParams wellToUsers = {0, 1.2};
    double millToFarmRatio = 0.75;
    BuildParams milToSawmill = {4.0, 0.1, {0.0, 2}};
    BuildParams milToForester = {0.1, 0.025, {2.0, 0.25}};
    BuildParams sawmillToForester = {0, 0.0, {2.0, 1}};
    BuildParams metalworksToIronsmelter = {0.0, 0.00, {1.0, 1.0}};
    BuildParams breweryToArmory = {1.0, 0.2};

    BuildParams startupMilToSawmill = {3.0, 0.4};
    BuildParams startupMilToWoodcutter = {3.0, 0.4};
};

extern AIConfig AI_CONFIG;

extern BuildParams parseBuildParams(const YAML::Node& node, const BuildParams& defaults);

extern void initAIConfig(std::string configPath);

#endif