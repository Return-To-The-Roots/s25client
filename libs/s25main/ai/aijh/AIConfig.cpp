//
// Created by pavel on 31.01.25.
//

#include "AIConfig.h"

#include "helpers/EnumRange.h"

#include "gameTypes/GoodTypes.h"

#include <iostream>
#include <yaml-cpp/yaml.h>

// Define the global instance
AIConfig AI_CONFIG;

extern BuildParams parseBuildParams(const YAML::Node& node, const BuildParams& defaults)
{
    BuildParams params = defaults;

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
    if(node["min"])
    {
        params.min = node["min"].as<unsigned>();
    }
    return params;
}

WantedParams parseWantedParams(const YAML::Node& node, WantedParams params)
{
    if(node["buildings"])
        for(const auto& weightEntry : node["buildings"])
        {
            std::string buildingStr = weightEntry.first.as<std::string>();
            BuildingType bldType;
            try
            {
                bldType = BUILDING_NAME_MAP.at(buildingStr);
            } catch(...)
            {
                continue;
            }

            params.bldWeights[bldType] = parseBuildParams(weightEntry.second, params.bldWeights[bldType]);
        }
    if(node["goods"])
        for(const auto& weightEntry : node["goods"])
        {
            std::string statStr = weightEntry.first.as<std::string>();
            GoodType goodType;
            try
            {
                goodType = GOOD_NAMES_MAP.at(statStr);
            } catch(...)
            {
                continue;
            }

            params.goodWeights[goodType] = parseBuildParams(weightEntry.second, params.goodWeights[goodType]);
        }
    if(node["stats"])
        for(const auto& weightEntry : node["stats"])
        {
            std::string statStr = weightEntry.first.as<std::string>();
            StatisticType statType;
            try
            {
                statType = STATS_NAME_MAP.at(statStr);
            } catch(...)
            {
                continue;
            }

            params.statsWeights[statType] = parseBuildParams(weightEntry.second, params.statsWeights[statType]);
        }
    if(node["workersAdvance"])
        params.workersAdvance = parseBuildParams(node["workersAdvance"], {});

    if(node["max"])
        params.max = node["max"].as<unsigned>();

    if(node["minProductivity"])
        params.minProductivity = node["minProductivity"].as<unsigned>();

    return params;
}

extern void initDefaults()
{
}

extern void applyWeightsCfg(std::string weightCfgPath)
{
    try
    {
        YAML::Node rootNode = YAML::LoadFile(weightCfgPath);
        auto plannerNode = rootNode["buildPlanner"];
        std::string bldName;
        BuildingType bldType;
        for(const auto& weightsNode : plannerNode)
        {
            try
            {
                bldName = weightsNode.first.as<std::string>();
                bldType = BUILDING_NAME_MAP.at(bldName);
                AI_CONFIG.wantedParams[bldType] = parseWantedParams(weightsNode.second, AI_CONFIG.wantedParams[bldType]);
            } catch(...)
            {
                continue;
            }
        }
    } catch(const YAML::Exception& e)
    {
        std::cerr << "Error parsing weights YAML file: " << e.what() << std::endl;
        exit(1);
    }
}
extern void initAIConfig(std::string configPath)
{
    try
    {
        YAML::Node configNode = YAML::LoadFile(configPath);

        AI_CONFIG.foresterWoodLevel = configNode["forester_wood_level"].as<double>();
        AI_CONFIG.startupMilBuildings = configNode["startup_mil_buildings"].as<double>();
        AI_CONFIG.farmToIronMineRatio = configNode["farm_to_ironMine_ratio"].as<double>();
        AI_CONFIG.woodcutterToStorehouseRatio = configNode["woodcutter_to_storehouse_ratio"].as<double>();
        AI_CONFIG.breweryToArmoryRatio = configNode["brewery_to_armory_ratio"].as<double>();
        AI_CONFIG.pigfarmMultiplier = configNode["pigfarm_multiplier"].as<double>();

        AI_CONFIG.milToFarm = parseBuildParams(configNode["farm_to_mil"], AI_CONFIG.milToFarm);
        AI_CONFIG.wellToUsers = parseBuildParams(configNode["well_to_users"], AI_CONFIG.wellToUsers);
        AI_CONFIG.freeFarmToMill = parseBuildParams(configNode["farm_to_mill"], AI_CONFIG.freeFarmToMill);
        AI_CONFIG.milToSawmill = parseBuildParams(configNode["military_to_sawmill"], AI_CONFIG.milToSawmill);
        AI_CONFIG.sawmillToForester = parseBuildParams(configNode["sawmill_to_forester"], AI_CONFIG.sawmillToForester);
        AI_CONFIG.sawmillToWoodcutter =
          parseBuildParams(configNode["sawmill_to_woodcutter"], AI_CONFIG.sawmillToWoodcutter);
        AI_CONFIG.ironsmelterToMetalworks =
          parseBuildParams(configNode["ironsmelter_to_metalworks"], AI_CONFIG.ironsmelterToMetalworks);
        AI_CONFIG.ironMineToIronsmelter =
          parseBuildParams(configNode["iron_mine_to_ironsmelter"], AI_CONFIG.ironMineToIronsmelter);
        AI_CONFIG.startupMilToSawmill =
          parseBuildParams(configNode["startup_mil_to_sawmill"], AI_CONFIG.startupMilToSawmill);
        AI_CONFIG.startupMilToWoodcutter =
          parseBuildParams(configNode["startup_mil_to_sawmill"], AI_CONFIG.startupMilToWoodcutter);

    } catch(const YAML::Exception& e)
    {
        std::cerr << "Error parsing YAML file: " << e.what() << std::endl;
        exit(1);
    }
}

namespace {
struct ConfigInitializer
{
    ConfigInitializer() { initDefaults(); }
};
ConfigInitializer _initializer; // Runs before main()
} // namespace