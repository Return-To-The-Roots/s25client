//
// Created by pavel on 31.01.25.
//

#include "AIConfig.h"
#include "StatsConfig.h"
#include "WeightParser.h"
#include "gameData/MaxPlayers.h"
#include "helpers/EnumRange.h"

#include <array>
#include <cctype>
#include <iostream>
#include <memory>
#include <utility>
#include <algorithm>
#include <optional>
#include <yaml-cpp/yaml.h>

// Define the global instance
AIConfig AI_CONFIG;

AIConfig::AIConfig()
{
    distributionParams[GoodType::Grain][BuildingType::Brewery].enabled = true;
    distributionParams[GoodType::Grain][BuildingType::Brewery].overstockingPenalty[GoodType::Beer] =
      BuildParams{0.0, -0.04, 0.0, {}, 0, 99999, true};
    distributionParams[GoodType::Coal][BuildingType::Mint].enabled = true;
    distributionParams[GoodType::Coal][BuildingType::Mint].overstockingPenalty[GoodType::Coins] =
      BuildParams{0.0, -0.04, 0.0, {}, 0, 99999, true};
}

CombatConfig::CombatConfig()
{
    attackIntervals[AI::Level::Easy] = 2500;
    attackIntervals[AI::Level::Medium] = 750;
    attackIntervals[AI::Level::Hard] = 100;
}

namespace {
std::array<std::unique_ptr<AIConfig>, MAX_PLAYERS> gPlayerConfigs;

void applyCombatCfg(const YAML::Node& combatNode, AIConfig& config)
{
    if(!combatNode)
        return;

    auto parseTargetSelection = [](const std::string& raw)
      -> std::optional<TargetSelectionAlgorithm> {
        std::string normalized;
        normalized.resize(raw.size());
        std::transform(raw.begin(), raw.end(), normalized.begin(), [](unsigned char c) {
            return static_cast<char>(std::tolower(c));
        });

        if(normalized == "random")
            return TargetSelectionAlgorithm::Random;
        if(normalized == "prudent")
            return TargetSelectionAlgorithm::Prudent;
        if(normalized == "biting")
            return TargetSelectionAlgorithm::Biting;
        if(normalized == "attrition")
            return TargetSelectionAlgorithm::Attrition;
        return std::nullopt;
    };

    if(const YAML::Node fulfillmentNode = combatNode["fulfillment"])
    {
        try
        {
            if(const YAML::Node value = fulfillmentNode["low"])
                config.combat.fulfillmentLow = value.as<double>();
            if(const YAML::Node value = fulfillmentNode["medium"])
                config.combat.fulfillmentMedium = value.as<double>();
            if(const YAML::Node value = fulfillmentNode["high"])
                config.combat.fulfillmentHigh = value.as<double>();
        } catch(const YAML::TypedBadConversion<double>& e)
        {
            std::cerr << "Warning: Invalid combat fulfillment value, using defaults. Error: " << e.what()
                      << std::endl;
        }
    }

    if(const YAML::Node value = combatNode["forceAdvantageRatio"])
    {
        try
        {
            config.combat.forceAdvantageRatio = value.as<double>();
        } catch(const YAML::TypedBadConversion<double>& e)
        {
            std::cerr << "Warning: Invalid combat forceAdvantageRatio value, using default. Error: " << e.what()
                      << std::endl;
        }
    }

    if(const YAML::Node value = combatNode["minNearTroopsDensity"])
    {
        try
        {
            config.combat.minNearTroopsDensity = value.as<double>();
        } catch(const YAML::TypedBadConversion<double>& e)
        {
            std::cerr << "Warning: Invalid combat minNearTroopsDensity value, using default. Error: "
                      << e.what() << std::endl;
        }
    }

    if(const YAML::Node attackIntervalsNode = combatNode["attackIntervals"])
    {
        try
        {
            if(const YAML::Node value = attackIntervalsNode["easy"])
                config.combat.attackIntervals[AI::Level::Easy] = value.as<unsigned>();
            if(const YAML::Node value = attackIntervalsNode["medium"])
                config.combat.attackIntervals[AI::Level::Medium] = value.as<unsigned>();
            if(const YAML::Node value = attackIntervalsNode["hard"])
                config.combat.attackIntervals[AI::Level::Hard] = value.as<unsigned>();
        } catch(const YAML::TypedBadConversion<unsigned>& e)
        {
            std::cerr << "Warning: Invalid combat attack interval, using defaults. Error: " << e.what() << std::endl;
        }
    }

    if(const YAML::Node targetSelectionNode = combatNode["targetSelection"])
    {
        try
        {
            const std::string selectionValue = targetSelectionNode.as<std::string>();
            if(const auto parsed = parseTargetSelection(selectionValue))
                config.combat.targetSelection = *parsed;
            else
                std::cerr << "Warning: Unknown target selection algorithm '" << selectionValue
                          << "', defaulting to Random." << std::endl;
        } catch(const YAML::TypedBadConversion<std::string>& e)
        {
            std::cerr << "Warning: Invalid target selection value, using default. Error: " << e.what() << std::endl;
        }
    }
}

void applyPosFinderCfg(const YAML::Node& posFinder, AIConfig& config)
{
    std::string bldName;
    BuildingType bldType;
    for(const auto& weightsNode : posFinder)
    {
        try
        {
            bldName = weightsNode.first.as<std::string>();
            bldType = BUILDING_NAME_MAP.at(bldName);
            config.locationParams[bldType] =
              Weights::parseLocationParams(weightsNode.second, config.locationParams[bldType]);
        } catch(const YAML::TypedBadConversion<double>& e)
        {
            std::cerr << "Warning: Invalid value for 'linear', using default. Error: " << e.what() << std::endl;
            continue;
        }
    }
}

void applyBldPlannerCfg(const YAML::Node& plannerNode, AIConfig& config)
{
    std::string bldName;
    BuildingType bldType;
    for(const auto& weightsNode : plannerNode)
    {
        try
        {
            bldName = weightsNode.first.as<std::string>();
            bldType = BUILDING_NAME_MAP.at(bldName);
            config.wantedParams[bldType] =
              Weights::parseWantedParams(weightsNode.second, config.wantedParams[bldType]);
        } catch(const YAML::TypedBadConversion<double>& e)
        {
            std::cerr << "Warning: Invalid value for 'linear', using default. Error: " << e.what() << std::endl;
            continue;
        }
    }
}

void applyDisableBuildingCfg(const YAML::Node& disableNode, AIConfig& config)
{
    config.disableBuilding.clear();

    if(!disableNode)
        return;

    if(!disableNode.IsSequence())
    {
        std::cerr << "Warning: disableBuilding must be a sequence of building names." << std::endl;
        return;
    }

    for(const auto& node : disableNode)
    {
        try
        {
            const std::string bldName = node.as<std::string>();
            const auto iter = BUILDING_NAME_MAP.find(bldName);
            if(iter == BUILDING_NAME_MAP.end())
            {
                std::cerr << "Warning: Unknown building '" << bldName << "' in disableBuilding list." << std::endl;
                continue;
            }
            config.disableBuilding.push_back(iter->second);
        } catch(const YAML::TypedBadConversion<std::string>& e)
        {
            std::cerr << "Warning: Invalid disableBuilding entry, skipping. Error: " << e.what() << std::endl;
        }
    }
}

std::optional<Tool> parseToolName(const std::string& toolName)
{
    const auto iter = GOOD_NAMES_MAP.find(toolName);
    if(iter == GOOD_NAMES_MAP.end())
        return std::nullopt;

    const GoodType good = iter->second;
    for(const auto tool : helpers::enumRange<Tool>())
    {
        if(TOOL_TO_GOOD[tool] == good)
            return tool;
    }
    return std::nullopt;
}

void applyToolPriorityCfg(const YAML::Node& toolPriorityNode, AIConfig& config)
{
    if(!toolPriorityNode)
        return;

    if(!toolPriorityNode.IsMap())
    {
        std::cerr << "Warning: toolPriority must be a map of tool names to values." << std::endl;
        return;
    }

    for(const auto& node : toolPriorityNode)
    {
        try
        {
            const std::string toolName = node.first.as<std::string>();
            const auto tool = parseToolName(toolName);
            if(!tool)
            {
                std::cerr << "Warning: Unknown tool '" << toolName << "' in toolPriority map." << std::endl;
                continue;
            }
            config.toolPriority[*tool] = node.second.as<signed>();
        } catch(const YAML::TypedBadConversion<signed>& e)
        {
            std::cerr << "Warning: Invalid toolPriority value, skipping. Error: " << e.what() << std::endl;
        } catch(const YAML::TypedBadConversion<std::string>& e)
        {
            std::cerr << "Warning: Invalid toolPriority key, skipping. Error: " << e.what() << std::endl;
        }
    }
}

void applyDistributionAdjusterCfg(const YAML::Node& distributionNode, AIConfig& config)
{
    if(!distributionNode)
        return;

    if(!distributionNode.IsMap())
    {
        std::cerr << "Warning: distributionAdjuster must be a map of distributed goods." << std::endl;
        return;
    }

    for(const auto& goodNode : distributionNode)
    {
        try
        {
            const std::string distributedGoodName = goodNode.first.as<std::string>();
            const auto distributedGoodIt = GOOD_NAMES_MAP.find(distributedGoodName);
            if(distributedGoodIt == GOOD_NAMES_MAP.end())
            {
                std::cerr << "Warning: Unknown distributed good '" << distributedGoodName
                          << "' in distributionAdjuster map."
                          << std::endl;
                continue;
            }

            const YAML::Node buildingMapNode = goodNode.second;
            if(!buildingMapNode.IsMap())
            {
                std::cerr << "Warning: distributionAdjuster entry for '" << distributedGoodName
                          << "' must be a map of building names." << std::endl;
                continue;
            }

            for(const auto& buildingNode : buildingMapNode)
            {
                const std::string bldName = buildingNode.first.as<std::string>();
                const auto bldIter = BUILDING_NAME_MAP.find(bldName);
                if(bldIter == BUILDING_NAME_MAP.end())
                {
                    std::cerr << "Warning: Unknown building '" << bldName
                              << "' in distributionAdjuster map for good '" << distributedGoodName << "'."
                              << std::endl;
                    continue;
                }

                DistributionParams& params = config.distributionParams[distributedGoodIt->second][bldIter->second];
                const YAML::Node paramsNode = buildingNode.second;
                if(!paramsNode.IsMap())
                {
                    std::cerr << "Warning: distributionAdjuster entry for '" << distributedGoodName << " -> " << bldName
                              << "' must be a map." << std::endl;
                    continue;
                }

                if(const YAML::Node penaltiesNode = paramsNode["overstockingPenalty"])
                {
                    if(!penaltiesNode.IsMap())
                    {
                        std::cerr << "Warning: overstockingPenalty for '" << distributedGoodName << " -> " << bldName
                                  << "' must be a map of good names to BuildParams." << std::endl;
                        continue;
                    }

                    for(const auto& penaltyNode : penaltiesNode)
                    {
                        try
                        {
                            const std::string goodName = penaltyNode.first.as<std::string>();
                            const auto goodIter = GOOD_NAMES_MAP.find(goodName);
                            if(goodIter == GOOD_NAMES_MAP.end())
                            {
                                std::cerr << "Warning: Unknown good '" << goodName
                                          << "' in distributionAdjuster overstockingPenalty map." << std::endl;
                                continue;
                            }

                            params.overstockingPenalty[goodIter->second] =
                              Weights::parseBuildParams(penaltyNode.second, params.overstockingPenalty[goodIter->second]);
                            params.enabled = true;
                        } catch(const YAML::TypedBadConversion<std::string>& e)
                        {
                            std::cerr << "Warning: Invalid distributionAdjuster penalty key, skipping. Error: "
                                      << e.what() << std::endl;
                        } catch(const YAML::Exception& e)
                        {
                            std::cerr << "Warning: Invalid distributionAdjuster penalty value, skipping. Error: "
                                      << e.what() << std::endl;
                        }
                    }
                }
            }
        } catch(const YAML::TypedBadConversion<std::string>& e)
        {
            std::cerr << "Warning: Invalid distributionAdjuster key, skipping. Error: "
                      << e.what() << std::endl;
        }
    }
}
} // namespace

extern void applyWeightsCfg(std::string weightCfgPath)
{
    applyWeightsCfg(std::move(weightCfgPath), AI_CONFIG);
}

extern void applyWeightsCfg(std::string weightCfgPath, AIConfig& targetConfig)
{
    try
    {
        std::locale oldLocale = std::locale::global(std::locale("C"));
        YAML::Node rootNode = YAML::LoadFile(weightCfgPath);
        applyPosFinderCfg(rootNode["posFinder"], targetConfig);
        applyBldPlannerCfg(rootNode["buildPlanner"], targetConfig);
        applyCombatCfg(rootNode["combat"], targetConfig);
        applyDisableBuildingCfg(rootNode["disableBuilding"], targetConfig);
        applyToolPriorityCfg(rootNode["toolPriority"], targetConfig);
        applyDistributionAdjusterCfg(rootNode["distributionAdjuster"], targetConfig);
        std::locale::global(oldLocale);
    } catch(const YAML::Exception& e)
    {
        std::cerr << "Error parsing weights YAML file: " << e.what() << std::endl;
        exit(1);
    }
}

extern void ApplyPlayerWeightsCfg(const unsigned char playerId, std::string weightCfgPath)
{
    if(playerId >= MAX_PLAYERS)
    {
        std::cerr << "Invalid player id for weights override: " << static_cast<unsigned>(playerId) << std::endl;
        return;
    }

    auto cfg = std::make_unique<AIConfig>();
    applyWeightsCfg(std::move(weightCfgPath), *cfg);
    gPlayerConfigs[playerId] = std::move(cfg);
}

extern const AIConfig& GetAIConfigForPlayer(const unsigned char playerId)
{
    if(playerId < MAX_PLAYERS)
    {
        if(gPlayerConfigs[playerId])
            return *gPlayerConfigs[playerId];
    }
    return AI_CONFIG;
}

namespace {
struct ConfigInitializer
{
    ConfigInitializer() {}
};
ConfigInitializer _initializer; // Runs before main()
} // namespace
