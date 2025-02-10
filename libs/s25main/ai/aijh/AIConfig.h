#ifndef CONFIG_H
#define CONFIG_H

struct AIConfig
{
    double startupMilBuildings = 15.0;
    double farmToIronMineRatio = 2.5;
    double woodcutterToForesterRatio = 3.5;
    double woodcutterToStorehouseRatio = 2.0;
    double woodcutterToWoodRatio = 2500;
    double breweryToArmoryRatio = 5;
    double ironsmelterToIronMineRatio = 0.9;
    double pigfarmMultiplier = 1.0;
    double millToFarmRatio = 0.66;
    double maxMetalworks = 2.0;
};

extern AIConfig AI_CONFIG;

#endif