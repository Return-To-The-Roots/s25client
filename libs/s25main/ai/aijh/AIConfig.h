#ifndef CONFIG_H
#define CONFIG_H
#include <string>

struct AIConfig
{
    std::string runId = "000";
    unsigned startupMilBuildings = 15;
    float farmToIronMineRatio = 2.5;
    float woodcutterToForesterRatio = 3.5;
    float woodcutterToStorehouseRatio = 2;
    float woodcutterToWoodRatio = 2500;
    float breweryToArmoryRatio = 5;
    double ironsmelterToIronMineRatio = 0.9;
    double millToFarmRatio = 0.66;
    std::string statsPath = "/home/pavel/Documents/settlers_ai/stats/";
    std::string savesPath = "/home/pavel/Documents/settlers_ai/saves/";
};

extern AIConfig AI_CONFIG;

#endif