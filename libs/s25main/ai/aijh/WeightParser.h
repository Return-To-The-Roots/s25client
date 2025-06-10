//
// Created by pavel on 09.06.25.
//

#ifndef WEIGHTPARSER_H
#define WEIGHTPARSER_H
#include "AIConfig.h"

namespace Weights {
inline extern BuildParams parseBuildParams(const YAML::Node& node, const BuildParams& defaults)
{
    BuildParams params = defaults;
    params.enabled = true;
    if(node["constant"])
        params.constant = node["constant"].as<double>();
    if(node["linear"])
    {
        params.linear = node["linear"].as<double>();
    }
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
    if(node["max"])
    {
        params.max = node["max"].as<unsigned>();
    }
    return params;
}
LocationParams parseLocationParams(const YAML::Node& node, WantedParams params)
{
    params.enabled = true;
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

            params.bldWeights[bldType] = Weights::parseBuildParams(weightEntry.second, params.bldWeights[bldType]);
        }
}
WantedParams parseWantedParams(const YAML::Node& node, WantedParams params)
{
    params.enabled = true;
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

            params.bldWeights[bldType] = Weights::parseBuildParams(weightEntry.second, params.bldWeights[bldType]);
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

            params.goodWeights[goodType] = Weights::parseBuildParams(weightEntry.second, params.goodWeights[goodType]);
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

            params.statsWeights[statType] =
              Weights::parseBuildParams(weightEntry.second, params.statsWeights[statType]);
        }
    if(node["resources"])
        for(const auto& weightEntry : node["resources"])
        {
            std::string resourceStr = weightEntry.first.as<std::string>();
            AIResource resourceType;
            try
            {
                resourceType = AI_RESOURCE_NAME_MAP.at(resourceStr);
            } catch(...)
            {
                continue;
            }

            params.resourceWeights[resourceType] =
              Weights::parseBuildParams(weightEntry.second, params.resourceWeights[resourceType]);
        }
    if(node["workersAdvance"])
        params.workersAdvance = Weights::parseBuildParams(node["workersAdvance"], {});

    if(node["max"])
        params.max = node["max"].as<unsigned>();

    if(node["minProductivity"])
        params.minProductivity = node["minProductivity"].as<unsigned>();

    return params;
}
#endif // WEIGHTPARSER_H
