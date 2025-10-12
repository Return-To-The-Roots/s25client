#include "BuildingCalculator.h"

#include "AIConfig.h"
#include "AIPlayerJH.h"
#include "BuildingPlanner.h"
#include "PlannerHelper.h"
#include "WeightParams.h"
#include "addons/const_addons.h"
#include "gameTypes/GoodTypes.h"

#include <cmath>

class AIPlayer;
struct Inventory;
class GamePlayer;

namespace AIJH {
BuildCalculator::BuildCalculator(const AIPlayerJH& aijh, BuildingCount buildingNums, const Inventory& inventory,
                                 unsigned woodAvailable)
    : aijh(aijh), buildingNums(buildingNums), inventory(inventory), woodAvailable(woodAvailable),
      numMilitaryBlds(aijh.player.GetBuildingRegister().GetMilitaryBuildings().size())
{}

helpers::EnumArray<unsigned, BuildingType> BuildCalculator::GetStartupSet()
{
    auto values = helpers::EnumArray<unsigned, BuildingType>();
    values[BuildingType::Forester] = doCalc(BuildingType::Forester);
    values[BuildingType::Sawmill] = doCalc(BuildingType::Sawmill);
    values[BuildingType::Woodcutter] = doCalc(BuildingType::Woodcutter);
    values[BuildingType::Quarry] = 1 + numMilitaryBlds / 3;
    values[BuildingType::Fishery] = 1 + numMilitaryBlds / 5;
    values[BuildingType::GraniteMine] = -1;
    values[BuildingType::CoalMine] = -1;
    values[BuildingType::IronMine] = -1;
    values[BuildingType::GoldMine] = -1;
    values[BuildingType::Catapult] = -1;
    values[BuildingType::Hunter] = -1;
    values[BuildingType::Farm] = -1;
    values[BuildingType::Charburner] = -1;
    values[BuildingType::Ironsmelter] = -1;
    values[BuildingType::Metalworks] = -1;

    return values;
}
unsigned BuildCalculator::Calc(BuildingType type)
{
    switch(type)
    {
        case BuildingType::PigFarm: return doCalc(type);
        case BuildingType::Slaughterhouse: return doCalc(type);
        case BuildingType::Woodcutter: return doCalc(type);
        case BuildingType::Forester: return doCalc(type);
        case BuildingType::Farm: return doCalc(type);
        case BuildingType::Quarry: return doCalc(type);
        case BuildingType::Well: return doCalc(type);
        case BuildingType::Sawmill: return doCalc(type);
        case BuildingType::Mill: return doCalc(type);
        case BuildingType::Bakery: return doCalc(type);
        case BuildingType::Ironsmelter: return doCalc(type);
        case BuildingType::Armory: return doCalc(type);
        case BuildingType::Metalworks: return doCalc(type);
        case BuildingType::Brewery: return doCalc(type);
        case BuildingType::IronMine: return doCalc(type);
        case BuildingType::CoalMine: return doCalc(type);
        case BuildingType::DonkeyBreeder: return doCalc(type);
        default: return 0u;
    }
}

unsigned BuildCalculator::doCalc(BuildingType type)
{
    WantedParams wantedParams = AI_CONFIG.wantedParams[type];
    unsigned workersAvailable = maxWorkers(aijh, type);
    unsigned maxBld = workersAvailable + (unsigned)CALC::calcCount(workersAvailable, wantedParams.workersAdvance);
    unsigned currentBld = GetNumBuildings(type);
    if(currentBld >= maxBld)
    {
        return maxBld;
    }

    double count = 0;
    double productivity = aijh.GetProductivity(type);
    if(buildingNums.buildings[type] > 0 && productivity < wantedParams.minProductivity)
        return currentBld;
    if(buildingNums.buildings[type] > 2 && wantedParams.productivity.enabled)
    {
        double diff = 100 - productivity;
        double malus = std::pow(diff * wantedParams.productivity.linear,  wantedParams.productivity.exponential);
        count -= malus;
    }

    for(const auto bldType : helpers::enumRange<BuildingType>())
    {
        BuildParams params = wantedParams.bldWeights[bldType];
        if(!params.enabled)
            continue;

        unsigned bldCount = GetNumBuildings(bldType);
        if(bldCount >= params.min)
        {
            double value = CALC::calcCount(bldCount, params);
            count += std::min<double>(value, params.max);
        }
    }
    for(const auto goodType : helpers::enumRange<GoodType>())
    {
        BuildParams params = wantedParams.goodWeights[goodType];
        if(!params.enabled)
            continue;
        unsigned goodCount = inventory.goods[goodType];
        if(goodCount >= params.min)
        {
            double value = CALC::calcCount(goodCount, params);
            count += std::min<double>(value, params.max);
        }
    }
    for(const auto statType : helpers::enumRange<StatisticType>())
    {
        BuildParams params = wantedParams.statsWeights[statType];
        if(!params.enabled)
            continue;
        unsigned statValue = aijh.player.GetStatisticCurrentValue(statType);
        if(statValue >= params.min)
        {
            double value = CALC::calcCount(statValue, params);
            count += std::min<double>(value, params.max);
        }
    }
    for(const auto resType : helpers::enumRange<AIResource>())
    {
        BuildParams params = wantedParams.resourceWeights[resType];
        if(!params.enabled)
            continue;
        unsigned resValue = getAvailableResource(resType);
        if(resValue >= params.min)
        {
            double value = CALC::calcCount(resValue, params);
            count += std::min<double>(value, params.max);
        }
    }
    count = std::max<double>(0.0, count);
    unsigned result = std::max<unsigned>(0, static_cast<unsigned>(count));
    return std::min<unsigned>(std::min<unsigned>(result, maxBld), wantedParams.max);
}

unsigned BuildCalculator::CalcPigFarms()
{
    unsigned farms = GetNumBuildings(BuildingType::Farm);
    unsigned wanted = (farms < 8) ? farms / 4 : (farms - 2) / 4;
    unsigned slaughterhouses = GetNumBuildings(BuildingType::Slaughterhouse);
    if(wanted > slaughterhouses + 1)
        wanted = slaughterhouses + 1;
    return wanted;
}

unsigned BuildCalculator::getAvailableResource(AIResource resType)
{
    switch(resType)
    {
        case AIResource::Wood: return woodAvailable;
        default: return 0;
    }
}

unsigned BuildCalculator::GetNumBuildings(BuildingType type)
{
    return buildingNums.buildings[type] + buildingNums.buildingSites[type];
}

} // namespace AIJH