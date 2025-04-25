//
// Created by pavel on 31.01.25.
//

#include "AIConfig.h"

#include <iostream>
#include <yaml-cpp/yaml.h>

// Define the global instance
AIConfig AI_CONFIG;

extern BuildParams parseBuildParams(const YAML::Node& node, const BuildParams& defaults)
{
    BuildParams params = defaults; // Start with default values

    if(node["constant"])
        params.constant = node["constant"].as<double>();
    if(node["linear"])
        params.linear = node["linear"].as<double>();

    if(node["logTwo"])
    {
        YAML::Node nodeLog2 = node["logTwo"];
        if(nodeLog2["constant"])
            params.logTwo.constant = nodeLog2["constant"].as<double>();
        if(nodeLog2["linear"])
            params.logTwo.linear = nodeLog2["linear"].as<double>();
    }
    return params;
}

extern void initAIConfig(std::string configPath)
{
    YAML::Node configNode = YAML::LoadFile(configPath);
    try
    {
        YAML::Node configNode = YAML::LoadFile(configPath);

        AI_CONFIG.foresterWoodLevel = configNode["forester_wood_level"].as<double>();
        AI_CONFIG.startupMilBuildings = configNode["startup_mil_buildings"].as<double>();
        AI_CONFIG.farmToIronMineRatio = configNode["farm_to_ironMine_ratio"].as<double>();
        AI_CONFIG.woodcutterToStorehouseRatio = configNode["woodcutter_to_storehouse_ratio"].as<double>();
        AI_CONFIG.breweryToArmoryRatio = configNode["brewery_to_armory_ratio"].as<double>();
        AI_CONFIG.maxMetalworks = configNode["max_metalworks"].as<double>();
        AI_CONFIG.pigfarmMultiplier = configNode["pigfarm_multiplier"].as<double>();

        AI_CONFIG.milToFarm = parseBuildParams(configNode["farm_to_mil"], AI_CONFIG.milToFarm);
        AI_CONFIG.wellToUsers = parseBuildParams(configNode["well_to_users"], AI_CONFIG.wellToUsers);
        AI_CONFIG.freeFarmToMill = parseBuildParams(configNode["farm_to_mill"], AI_CONFIG.freeFarmToMill);
        AI_CONFIG.milToSawmill = parseBuildParams(configNode["military_to_sawmill"], AI_CONFIG.milToSawmill);
        AI_CONFIG.sawmillToForester = parseBuildParams(configNode["sawmill_to_forester"], AI_CONFIG.sawmillToForester);
        AI_CONFIG.sawmillToWoodcutter = parseBuildParams(configNode["sawmill_to_woodcutter"], AI_CONFIG.sawmillToWoodcutter);
        AI_CONFIG.ironsmelterToMetalworks = parseBuildParams(configNode["ironsmelter_to_metalworks"], AI_CONFIG.ironsmelterToMetalworks);
        AI_CONFIG.ironMineToIronsmelter = parseBuildParams(configNode["iron_mine_to_ironsmelter"], AI_CONFIG.ironMineToIronsmelter);
        AI_CONFIG.startupMilToSawmill = parseBuildParams(configNode["startup_mil_to_sawmill"], AI_CONFIG.startupMilToSawmill);
        AI_CONFIG.startupMilToWoodcutter = parseBuildParams(configNode["startup_mil_to_sawmill"], AI_CONFIG.startupMilToWoodcutter);

    } catch(const YAML::Exception& e)
    {
        std::cerr << "Error parsing YAML file: " << e.what() << std::endl;
        exit(1);
    }
}