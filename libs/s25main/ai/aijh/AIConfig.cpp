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

CombatConfig::CombatConfig()
{
    attackIntervals[AI::Level::Easy] = 2500;
    attackIntervals[AI::Level::Medium] = 750;
    attackIntervals[AI::Level::Hard] = 100;
}

namespace {
void applyCombatCfg(const YAML::Node& combatNode)
{
    if(!combatNode)
        return;

    if(const YAML::Node fulfillmentNode = combatNode["fulfillment"])
    {
        try
        {
            if(const YAML::Node value = fulfillmentNode["low"])
                AI_CONFIG.combat.fulfillmentLow = value.as<double>();
            if(const YAML::Node value = fulfillmentNode["medium"])
                AI_CONFIG.combat.fulfillmentMedium = value.as<double>();
            if(const YAML::Node value = fulfillmentNode["high"])
                AI_CONFIG.combat.fulfillmentHigh = value.as<double>();
        } catch(const YAML::TypedBadConversion<double>& e)
        {
            std::cerr << "Warning: Invalid combat fulfillment value, using defaults. Error: " << e.what()
                      << std::endl;
        }
    }

    if(const YAML::Node attackIntervalsNode = combatNode["attackIntervals"])
    {
        try
        {
            if(const YAML::Node value = attackIntervalsNode["easy"])
                AI_CONFIG.combat.attackIntervals[AI::Level::Easy] = value.as<unsigned>();
            if(const YAML::Node value = attackIntervalsNode["medium"])
                AI_CONFIG.combat.attackIntervals[AI::Level::Medium] = value.as<unsigned>();
            if(const YAML::Node value = attackIntervalsNode["hard"])
                AI_CONFIG.combat.attackIntervals[AI::Level::Hard] = value.as<unsigned>();
        } catch(const YAML::TypedBadConversion<unsigned>& e)
        {
            std::cerr << "Warning: Invalid combat attack interval, using defaults. Error: " << e.what() << std::endl;
        }
    }
}
} // namespace

extern void applyWeightsCfg(std::string weightCfgPath)
{
    try
    {
        std::locale oldLocale = std::locale::global(std::locale("C"));
        YAML::Node rootNode = YAML::LoadFile(weightCfgPath);
        applyPosFinderCfg(rootNode["posFinder"]);
        applyBldPlannerCfg(rootNode["buildPlanner"]);
        applyCombatCfg(rootNode["combat"]);
        std::locale::global(oldLocale);
    } catch(const YAML::Exception& e)
    {
        std::cerr << "Error parsing weights YAML file: " << e.what() << std::endl;
        exit(1);
    }
}

extern void applyPosFinderCfg(YAML::Node posFinder)
{
    std::string bldName;
    BuildingType bldType;
    for(const auto& weightsNode : posFinder)
    {
        try
        {
            bldName = weightsNode.first.as<std::string>();
            bldType = BUILDING_NAME_MAP.at(bldName);
            AI_CONFIG.locationParams[bldType] =
              Weights::parseLocationParams(weightsNode.second, AI_CONFIG.locationParams[bldType]);
        } catch(const YAML::TypedBadConversion<double>& e)
        {
            std::cerr << "Warning: Invalid value for 'linear', using default. Error: " << e.what() << std::endl;
            continue;
        }
    }
}
extern void applyBldPlannerCfg(YAML::Node plannerNode)
{
    std::string bldName;
    BuildingType bldType;
    for(const auto& weightsNode : plannerNode)
    {
        try
        {
            bldName = weightsNode.first.as<std::string>();
            bldType = BUILDING_NAME_MAP.at(bldName);
            AI_CONFIG.wantedParams[bldType] =
              Weights::parseWantedParams(weightsNode.second, AI_CONFIG.wantedParams[bldType]);
        } catch(const YAML::TypedBadConversion<double>& e)
        {
            std::cerr << "Warning: Invalid value for 'linear', using default. Error: " << e.what() << std::endl;
            continue;
        }
    }
}

namespace {
struct ConfigInitializer
{
    ConfigInitializer() {}
};
ConfigInitializer _initializer; // Runs before main()
} // namespace
