
#include "WeightParser.h"
#include "WeightParams.h"
#include "AIConfig.h"

ProximityParams Weights::parseProximityParams(const YAML::Node& node, const ProximityParams& defaults)
{
    ProximityParams params = defaults;
    params.enabled = true;
    if(node["minimal"])
    {
        params.minimal = Weights::parseBuildParams(node["minimal"], params.minimal);
    }
    return params;
}

RatingParams Weights::parseRatingParams(const YAML::Node& node, const RatingParams& defaults)
{
    RatingParams params = defaults;
    params.enabled = true;
    if(node["radius"])
        params.radius = static_cast<unsigned>(node["radius"].as<double>());
    if(node["multiplier"])
        params.multiplier = static_cast<int>(node["multiplier"].as<double>());
    return params;
}

LocationParams Weights::parseLocationParams(const YAML::Node& node, const LocationParams& defaults)
{
    LocationParams params = defaults;
    if(node["buildOnBorder"])
        params.buildOnBorder = node["buildOnBorder"].as<bool>();
    if(node["proximity"])
        for(const auto& weightEntry : node["proximity"])
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

            params.proximity[bldType] = parseProximityParams(weightEntry.second, params.proximity[bldType]);
        }
    if(node["rating"])
        for(const auto& weightEntry : node["rating"])
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

            params.rating[bldType] = parseRatingParams(weightEntry.second, params.rating[bldType]);
        }
    return params;
}

BuildParams Weights::parseBuildParams(const YAML::Node& node, const BuildParams& defaults)
{
    BuildParams params = defaults;
    params.enabled = true;
    if(node["constant"])
        params.constant = node["constant"].as<double>();
    if(node["linear"])
    {
        params.linear = node["linear"].as<double>();
    }
    if(node["exponential"])
    {
        params.exponential = node["exponential"].as<double>();
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
        params.min = static_cast<unsigned>(node["min"].as<double>());
    }
    if(node["max"])
    {
        params.max = static_cast<unsigned>(node["max"].as<double>());
    }
    return params;
}

WantedParams Weights::parseWantedParams(const YAML::Node& node, WantedParams params)
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
        params.workersAdvance = parseBuildParams(node["workersAdvance"], {});

    if(node["max"])
        params.max = static_cast<unsigned>(node["max"].as<double>());

    if(node["minProductivity"])
        params.minProductivity = static_cast<unsigned>(node["minProductivity"].as<double>());

    if(node["productivity"])
        params.productivity = parseBuildParams(node["productivity"], {});
    return params;
}
