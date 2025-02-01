#ifndef CONFIG_H
#define CONFIG_H
#include <string>

struct AIConfig {
    unsigned startupMilBuildings = 15;
    float farmToIronMineRatio = 2.5;
    float woodcutterToForesterRatio = 3.5;
    float woodcutterToStorehouseRatio = 2;
    float breweryToArmoryRatio = 5;
    double millToFarmRatio = 0.66;
    std::string statsPath = "/home/pavel/Documents/settlers_ai/stats/009";
};

extern AIConfig AI_CONFIG;

#endif