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
    return params;
}

WantedParams parseWantedParams(const YAML::Node& node, WantedParams params)
{
    if(node["bldWeights"])
        for(const auto& weightEntry : node["bldWeights"])
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

    if(node["workersAdvance"])
        params.workersAdvance = parseBuildParams(node["workersAdvance"], {});

    if(node["max"])
        params.max = node["max"].as<unsigned>();

    return params;
}

extern void initDefaults()
{
    helpers::EnumArray<BuildParams, BuildingType> donkeyBreederBldParams =
      helpers::EnumArray<BuildParams, BuildingType>{};
    helpers::EnumArray<BuildParams, GoodType> donkeyBreederGoodParams = helpers::EnumArray<BuildParams, GoodType>{};
    helpers::EnumArray<BuildParams, StatisticType> donkeyBreederStatsParams =
      helpers::EnumArray<BuildParams, StatisticType>{};
    donkeyBreederStatsParams[StatisticType::Country] = {1, 0, {}, {}, 2000};
    AI_CONFIG.wantedParams[BuildingType::DonkeyBreeder] = {donkeyBreederBldParams,  donkeyBreederGoodParams, {}, 1, 2,
                                                           donkeyBreederStatsParams};

    helpers::EnumArray<BuildParams, BuildingType> wellBldParams = helpers::EnumArray<BuildParams, BuildingType>{};
    wellBldParams[BuildingType::Bakery] = {0, 1};
    wellBldParams[BuildingType::PigFarm] = {0, 1};
    wellBldParams[BuildingType::DonkeyBreeder] = {0, 1};
    wellBldParams[BuildingType::Brewery] = {0, 1};
    helpers::EnumArray<BuildParams, GoodType> wellGoodParams = helpers::EnumArray<BuildParams, GoodType>{};
    wellGoodParams[GoodType::Water] = {0, -0.02, {}, {}, 50};
    wellGoodParams[GoodType::Flour] = {0, 0.02, {}, {}, 50};
    AI_CONFIG.wantedParams[BuildingType::Well] = {wellBldParams, wellGoodParams};

    helpers::EnumArray<BuildParams, BuildingType> sawmillBldParams = helpers::EnumArray<BuildParams, BuildingType>{};
    helpers::EnumArray<BuildParams, GoodType> sawmillGoodParams = helpers::EnumArray<BuildParams, GoodType>{};
    helpers::EnumArray<BuildParams, StatisticType> sawmillStatParams = helpers::EnumArray<BuildParams, StatisticType>{};
    sawmillStatParams[StatisticType::Country] = {2, 0.001, {-3, 0.05}};
    AI_CONFIG.wantedParams[BuildingType::Sawmill] = {sawmillBldParams, sawmillGoodParams, {2}, 9999, 70,
                                                     sawmillStatParams};
    helpers::EnumArray<BuildParams, BuildingType> millBldParams = helpers::EnumArray<BuildParams, BuildingType>{};
    millBldParams[BuildingType::Farm] = {0, 0.9};
    millBldParams[BuildingType::DonkeyBreeder] = {0, -1};
    millBldParams[BuildingType::Brewery] = {0, -1};
    helpers::EnumArray<BuildParams, GoodType> millGoodParams = helpers::EnumArray<BuildParams, GoodType>{};
    millGoodParams[GoodType::Flour] = {-1, -0.02, {}, {}, 50};
    AI_CONFIG.wantedParams[BuildingType::Mill] = {millBldParams, millGoodParams, {2}};

    helpers::EnumArray<BuildParams, BuildingType> bakeryBldParams = helpers::EnumArray<BuildParams, BuildingType>{};
    bakeryBldParams[BuildingType::Mill] = {0, 1};
    helpers::EnumArray<BuildParams, GoodType> bakeryGoodParams = helpers::EnumArray<BuildParams, GoodType>{};
    AI_CONFIG.wantedParams[BuildingType::Bakery] = {bakeryBldParams, bakeryGoodParams, {2}};

    WantedParams ironmineWantedParams = {};
    ironmineWantedParams.workersAdvance = {2};
    ironmineWantedParams.bldWeights[BuildingType::Farm] = {0, 0.34};
    ironmineWantedParams.goodWeights[GoodType::IronOre] = {-1, -0.02, {}, {}, 50};
    ironmineWantedParams.goodWeights[GoodType::Bread] = {0, 0.02, {}, {}, 0};
    ironmineWantedParams.goodWeights[GoodType::Fish] = {0, 0.02, {}, {}, 0};
    ironmineWantedParams.goodWeights[GoodType::Meat] = {0, 0.02, {}, {}, 0};
    ironmineWantedParams.minProductivity = 70;
    AI_CONFIG.wantedParams[BuildingType::IronMine] = ironmineWantedParams;

    WantedParams coalmineWantedParams = {};
    coalmineWantedParams.workersAdvance = {2};
    coalmineWantedParams.bldWeights[BuildingType::Farm] = {0, 0.66};
    coalmineWantedParams.goodWeights[GoodType::Coal] = {-1, -0.02, {}, {}, 50};
    coalmineWantedParams.goodWeights[GoodType::Bread] = {0, 0.02, {}, {}, 0};
    coalmineWantedParams.goodWeights[GoodType::Fish] = {0, 0.02, {}, {}, 0};
    coalmineWantedParams.goodWeights[GoodType::Meat] = {0, 0.02, {}, {}, 0};
    coalmineWantedParams.minProductivity = 70;
    AI_CONFIG.wantedParams[BuildingType::CoalMine] = coalmineWantedParams;

    WantedParams ironsmelterWantedParams = {};
    ironsmelterWantedParams.workersAdvance = {2};
    ironmineWantedParams.bldWeights[BuildingType::IronMine] = {0, 1, {}, {}, 0};
    coalmineWantedParams.goodWeights[GoodType::Iron] = {0, -0.02, {}, {}, 50};
    ironmineWantedParams.minProductivity = 70;
    AI_CONFIG.wantedParams[BuildingType::Ironsmelter] = ironsmelterWantedParams;

    helpers::EnumArray<BuildParams, BuildingType> metalworksBldParams = helpers::EnumArray<BuildParams, BuildingType>{};
    metalworksBldParams[BuildingType::Ironsmelter] = {0, 1};
    helpers::EnumArray<BuildParams, GoodType> metalworksGoodParams = helpers::EnumArray<BuildParams, GoodType>{};
    AI_CONFIG.wantedParams[BuildingType::Metalworks] = {metalworksBldParams, metalworksGoodParams, {2}, 3};

    WantedParams armoryWantedParams = {};
    armoryWantedParams.workersAdvance = {2};
    ironmineWantedParams.bldWeights[BuildingType::Ironsmelter] = {0, 1, {}, {}, 2};
    ironmineWantedParams.bldWeights[BuildingType::Metalworks] = {0, -1};
    ironmineWantedParams.minProductivity = 70;
    AI_CONFIG.wantedParams[BuildingType::Armory] = armoryWantedParams;

    WantedParams breweryWantedParams = {};
    breweryWantedParams.bldWeights[BuildingType::Armory] = {1, 0.20, {}, {}, 1};
    breweryWantedParams.minProductivity = 70;
    AI_CONFIG.wantedParams[BuildingType::Brewery] = breweryWantedParams;

    helpers::EnumArray<BuildParams, BuildingType> breweryBldParams = helpers::EnumArray<BuildParams, BuildingType>{};
    breweryBldParams[BuildingType::Armory] = {0, 0.25};
    breweryBldParams[BuildingType::Farm] = {1, 0.0, {}, {}, 3};
    auto breweryGoodParams = helpers::EnumArray<BuildParams, GoodType>{};
    breweryGoodParams[GoodType::Beer] = {-1, -0.02, {}, {}, 50};
    AI_CONFIG.wantedParams[BuildingType::Brewery] = {breweryBldParams, breweryGoodParams, {2}, 3};
}

extern void applyWeightsCfg(std::string weightCfgPath)
{
    try
    {
        YAML::Node rootNode = YAML::LoadFile(weightCfgPath);
        std::string bldName;
        BuildingType bldType;
        for(const auto& weightsNode : rootNode)
        {
            try
            {
                bldName = weightsNode.first.as<std::string>();
                bldType = BUILDING_NAME_MAP.at(bldName);
                parseWantedParams(weightsNode.second, AI_CONFIG.wantedParams[bldType]);
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