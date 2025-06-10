//
// Created by pavel on 31.01.25.
//

#include "AIConfig.h"

#include "StatsConfig.h"
#include "WeightParser.h"

#include <iostream>
#include <yaml-cpp/yaml.h>

// Define the global instance
AIConfig AI_CONFIG;

extern void applyWeightsCfg(std::string weightCfgPath)
{
    std::locale oldLocale = std::locale::global(std::locale("C"));
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
                AI_CONFIG.wantedParams[bldType] = Weights::parseWantedParams(weightsNode.second, AI_CONFIG.wantedParams[bldType]);
            } catch (const YAML::TypedBadConversion<double>& e) {
                std::cerr << "Warning: Invalid value for 'linear', using default. Error: " << e.what() << std::endl;
                continue;
            }
        }
        auto posFinderNode = rootNode["posFinder"];
        for(const auto& weightsNode : posFinderNode)
        {
            try
            {
                bldName = weightsNode.first.as<std::string>();
                bldType = BUILDING_NAME_MAP.at(bldName);
                AI_CONFIG.wantedParams[bldType] = Weights::parseWantedParams(weightsNode.second, AI_CONFIG.wantedParams[bldType]);
            } catch (const YAML::TypedBadConversion<double>& e) {
                std::cerr << "Warning: Invalid value for 'linear', using default. Error: " << e.what() << std::endl;
                continue;
            }
        }
    } catch(const YAML::Exception& e)
    {
        std::cerr << "Error parsing weights YAML file: " << e.what() << std::endl;
        exit(1);
    }
    std::locale::global(oldLocale);
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

        AI_CONFIG.milToFarm = Weights::parseBuildParams(configNode["farm_to_mil"], AI_CONFIG.milToFarm);
        AI_CONFIG.wellToUsers = Weights::parseBuildParams(configNode["well_to_users"], AI_CONFIG.wellToUsers);
        AI_CONFIG.freeFarmToMill = Weights::parseBuildParams(configNode["farm_to_mill"], AI_CONFIG.freeFarmToMill);
        AI_CONFIG.milToSawmill = Weights::parseBuildParams(configNode["military_to_sawmill"], AI_CONFIG.milToSawmill);
        AI_CONFIG.sawmillToForester = Weights::parseBuildParams(configNode["sawmill_to_forester"], AI_CONFIG.sawmillToForester);
        AI_CONFIG.sawmillToWoodcutter =
          Weights::parseBuildParams(configNode["sawmill_to_woodcutter"], AI_CONFIG.sawmillToWoodcutter);
        AI_CONFIG.ironsmelterToMetalworks =
          Weights::parseBuildParams(configNode["ironsmelter_to_metalworks"], AI_CONFIG.ironsmelterToMetalworks);
        AI_CONFIG.ironMineToIronsmelter =
          Weights::parseBuildParams(configNode["iron_mine_to_ironsmelter"], AI_CONFIG.ironMineToIronsmelter);
        AI_CONFIG.startupMilToSawmill =
          Weights::parseBuildParams(configNode["startup_mil_to_sawmill"], AI_CONFIG.startupMilToSawmill);
        AI_CONFIG.startupMilToWoodcutter =
          Weights::parseBuildParams(configNode["startup_mil_to_sawmill"], AI_CONFIG.startupMilToWoodcutter);

    } catch(const YAML::Exception& e)
    {
        std::cerr << "Error parsing YAML file: " << e.what() << std::endl;
        exit(1);
    }
}

namespace {
struct ConfigInitializer
{
    ConfigInitializer() { }
};
ConfigInitializer _initializer; // Runs before main()
} // namespace