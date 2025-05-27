#include "BuildingCalculator.h"

#include "AIConfig.h"
#include "AIPlayerJH.h"
#include "BuildingPlanner.h"
#include "GlobalGameSettings.h"
#include "PlannerHelper.h"
#include "addons/const_addons.h"
#include "gameTypes/GoodTypes.h"
#include "gameTypes/JobTypes.h"

#include <boost/math/special_functions/sign.hpp>

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
    values[BuildingType::Woodcutter] = (unsigned)calcCount(numMilitaryBlds, AI_CONFIG.startupMilToWoodcutter);
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
    unsigned maxBld = workersAvailable + (unsigned)calcCount(workersAvailable, wantedParams.workersAdvance);
    unsigned currentBld = GetNumBuildings(type);
    if(currentBld >= maxBld)
    {
        return maxBld;
    }
    if(buildingNums.buildings[type] > 0 && aijh.GetProductivity(type) < wantedParams.minProductivity)
    {
        return currentBld;
    }
    double count = 0;
    for(const auto type : helpers::enumRange<BuildingType>())
    {
        BuildParams params = wantedParams.bldWeights[type];
        if(!params.enabled)
            continue;

        unsigned bldCount = GetNumBuildings(type);
        if(bldCount >= params.min)
        {
            double value = calcCount(bldCount, params);
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
            double value = calcCount(goodCount, params);
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
            double value = calcCount(statValue, params);
            count += std::min<double>(value, params.max);
        }
    }
    for(const auto resType : helpers::enumRange<AIResource>())
    {
        BuildParams params = wantedParams.resourceWeights[resType];
        if(!params.enabled)
            continue;
        unsigned statValue = getAvailableResource(resType);
        if(statValue >= params.min)
        {
            double value = calcCount(statValue, params);
            count += std::min<double>(value, params.max);
        }
    }
    unsigned result = std::max<unsigned>(0, static_cast<unsigned>(count));
    return std::min<unsigned>(std::min<unsigned>(result, maxBld), wantedParams.max);
}

unsigned BuildCalculator::CalcWoodcutters()
{
    BuildParams params = AI_CONFIG.sawmillToWoodcutter;
    auto sawmills = GetNumBuildings(BuildingType::Sawmill);
    unsigned baseWoodCutters = (unsigned)(params.constant + params.linear * sawmills);

    unsigned woodcutters = GetNumBuildings(BuildingType::Woodcutter);

    unsigned forestersActive = buildingNums.buildings[BuildingType::Forester];
    unsigned extraWood = woodAvailable - 6 * forestersActive;

    unsigned additional_woodcutters = 0;
    if(baseWoodCutters > 0 && extraWood / baseWoodCutters > 20)
    {
        additional_woodcutters = std::min((unsigned)(woodcutters * 0.4), extraWood / 20);
    }

    unsigned max_available_woodcutter = maxWoodcutter(aijh);
    unsigned count = std::min(max_available_woodcutter + 2, additional_woodcutters + baseWoodCutters);
    return std::min(count, sawmills * 3);
}

unsigned BuildCalculator::CalcPigFarms()
{
    if(AI_CONFIG.pigfarmMultiplier == 0)
    {
        return 0;
    }
    unsigned farms = GetNumBuildings(BuildingType::Farm);
    unsigned wanted = (farms < 8) ? farms / 4 : (farms - 2) / 4;
    unsigned slaughterhouses = GetNumBuildings(BuildingType::Slaughterhouse);
    if(wanted > slaughterhouses + 1)
        wanted = slaughterhouses + 1;
    return wanted;
}

unsigned BuildCalculator::CalcFarms()
{
    unsigned count = (unsigned)std::min<double>(maxFarmer(aijh) * 1.1, numMilitaryBlds * AI_CONFIG.milToFarm.linear);
    unsigned grainUsers = calcGrainUsers();
    if(grainUsers > 5)
    {
        return unsigned(std::min<double>(grainUsers * 1.75, count));
    }
    return count;
}

unsigned BuildCalculator::getAvailableResource(AIResource resType)
{
    switch(resType)
    {
        case AIResource::Wood: return woodAvailable;
        default: return 0;
    }
}

unsigned BuildCalculator::calcGrainUsers()
{
    return GetNumBuildings(BuildingType::Mill) + GetNumBuildings(BuildingType::Charburner)
           + GetNumBuildings(BuildingType::Brewery) + GetNumBuildings(BuildingType::PigFarm)
           + GetNumBuildings(BuildingType::DonkeyBreeder);
}

unsigned BuildCalculator::GetNumBuildings(BuildingType type)
{
    return buildingNums.buildings[type] + buildingNums.buildingSites[type];
}

double BuildCalculator::calcCount(unsigned x, BuildParams params)
{
    double log2_linear = abs(params.logTwo.linear);
    double log2Val = std::max(0.0, std::log(params.logTwo.constant + log2_linear * x));
    return params.constant + params.linear * x + boost::math::sign(params.logTwo.linear) * log2Val;
}

} // namespace AIJH