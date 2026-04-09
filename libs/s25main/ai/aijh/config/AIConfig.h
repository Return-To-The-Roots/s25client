#ifndef CONFIG_H
#define CONFIG_H
#include "WeightParams.h"
#include "helpers/EnumArray.h"
#include "gameTypes/AIInfo.h"
#include "gameTypes/BuildingType.h"
#include "gameTypes/GoodTypes.h"
#include "gameData/ToolConsts.h"

#include <vector>
#include <yaml-cpp/yaml.h>

enum class TargetSelectionAlgorithm
{
    Random,
    Prudent,
    Biting,
    Attrition
};

struct CombatConfig
{
    double fulfillmentLow = 4.0;
    double fulfillmentMedium = 8.0;
    double fulfillmentHigh = 12.0;
    double forceAdvantageRatio = 1.10;
    double minNearTroopsDensity = 1.0;
    helpers::EnumArray<unsigned, AI::Level> attackIntervals;
    TargetSelectionAlgorithm targetSelection = TargetSelectionAlgorithm::Random;

    CombatConfig();
};

struct DistributionParams
{
    bool enabled = false;
    helpers::EnumArray<BuildParams, GoodType> overstockingPenalty{};
};

struct BQPenaltyConfig
{
    double buildLocation = 0.05;
    double roadRoute = 1.0;
};

struct AIConfig
{
    helpers::EnumArray<WantedParams, BuildingType> wantedParams;
    helpers::EnumArray<LocationParams, BuildingType> locationParams;
    helpers::EnumArray<helpers::EnumArray<DistributionParams, BuildingType>, GoodType> distributionParams;
    CombatConfig combat;
    BQPenaltyConfig bqPenalty;
    bool reserveMilitaryBorderSlots = true;
    unsigned reserveMilitaryBorderlandThreshold = 150;
    std::vector<BuildingType> disableBuilding;
    helpers::EnumArray<signed, Tool> toolPriority = TOOL_PRIORITY;

    AIConfig();
};

extern AIConfig AI_CONFIG;

extern BuildParams parseBuildParams(const YAML::Node& node, const BuildParams& defaults);
extern WantedParams parseWantedParams(const YAML::Node& node, WantedParams params);
extern WantedParams parseWeights(const YAML::Node& rootNode);

extern void initDefaults();
extern void applyWeightsCfg(std::string weightCfgPath);
extern void applyWeightsCfg(std::string weightCfgPath, AIConfig& targetConfig);
extern void ApplyPlayerWeightsCfg(unsigned char playerId, std::string weightCfgPath);
extern const AIConfig& GetAIConfigForPlayer(unsigned char playerId);
extern void initAIConfig(std::string configPath);

#endif
