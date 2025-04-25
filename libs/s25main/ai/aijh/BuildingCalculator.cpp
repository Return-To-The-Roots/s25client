#include "BuildingCalculator.h"

#include "AIConfig.h"
#include "AIPlayerJH.h"
#include "BuildingPlanner.h"
#include "GlobalGameSettings.h"
#include "PlannerHelper.h"
#include "addons/const_addons.h"
#include "gameTypes/GoodTypes.h"
#include "gameTypes/JobTypes.h"

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
    values[BuildingType::Forester] = CalcForesters();
    values[BuildingType::Sawmill] = calcCount(numMilitaryBlds, AI_CONFIG.startupMilToSawmill);
    values[BuildingType::Woodcutter] = calcCount(numMilitaryBlds, AI_CONFIG.startupMilToWoodcutter);
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

unsigned BuildCalculator::CalcForesters()
{
    const Inventory& inventory = aijh.player.GetInventory();
    unsigned max_available_forester = inventory[Job::Forester] + inventory[GoodType::Shovel];
    unsigned additional_forester = GetNumBuildings(BuildingType::Charburner);
    unsigned sawmills = GetNumBuildings(BuildingType::Sawmill);
    signed count = 0u;

    count = calcCount(sawmills, AI_CONFIG.sawmillToForester);
    count -= (unsigned)(woodAvailable / AI_CONFIG.foresterWoodLevel);

    count += additional_forester;
    count = std::max(1, count);
    count = std::min((signed)max_available_forester + 1, count);
    return count;
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
    unsigned count = (unsigned)std::min<double>(maxFarmer(aijh) * 1.2, numMilitaryBlds * AI_CONFIG.milToFarm.linear);
    unsigned grainUsers = calcGrainUsers();
    if(grainUsers > 5)
    {
        return unsigned(std::min<double>(grainUsers * 1.75, count));
    }
    return count;
}

unsigned BuildCalculator::CalcBreweries()
{
    unsigned armories = GetNumBuildings(BuildingType::Armory);
    unsigned farms = GetNumBuildings(BuildingType::Farm);
    if(armories == 0 || farms < 3)
    {
        return 0;
    }
    BuildParams params = AI_CONFIG.breweryToArmory;
    if(aijh.ggs.isEnabled(AddonId::INEXHAUSTIBLE_MINES))
        return unsigned(params.constant + armories * params.linear);
    return 1 + armories / 6;
}
unsigned BuildCalculator::CalcIronMines()
{
    unsigned count = 0;

    if(GetNumBuildings(BuildingType::Farm) > 7) // quite the empire just scale mines with farms
    {
        if(aijh.ggs.isEnabled(AddonId::INEXHAUSTIBLE_MINES))
        {
            count = unsigned(GetNumBuildings(BuildingType::Farm) / AI_CONFIG.farmToIronMineRatio);
        } else
            count = std::min(GetNumBuildings(BuildingType::Farm) / 2, GetNumBuildings(BuildingType::Ironsmelter) + 1);
    } else
    {
        unsigned numFoodProducers = GetNumBuildings(BuildingType::Bakery)
                                    + GetNumBuildings(BuildingType::Slaughterhouse)
                                    + GetNumBuildings(BuildingType::Hunter) + GetNumBuildings(BuildingType::Fishery);
        count = (inventory.people[Job::Miner] + inventory.goods[GoodType::PickAxe]
                   > GetNumBuildings(BuildingType::CoalMine) + GetNumBuildings(BuildingType::GoldMine) + 1
                 && numFoodProducers > 4) ?
                  2 :
                  1;
    }
    if(GetNumBuildings(BuildingType::IronMine) > 5 && aijh.GetProductivity(BuildingType::IronMine) < 70.0)
    {
        count = GetNumBuildings(BuildingType::IronMine);
    }
    return count;
}

unsigned BuildCalculator::CalcArmories()
{
    unsigned ironsmelters = GetNumBuildings(BuildingType::Ironsmelter);
    unsigned metalworks = GetNumBuildings(BuildingType::Metalworks);
    if(ironsmelters < 2)
    {
        return 0;
    }
    unsigned armoriesWanted = std::max(0, static_cast<int>(ironsmelters - metalworks));

    if(aijh.ggs.isEnabled(AddonId::HALF_COST_MIL_EQUIP))
    {
        armoriesWanted = armoriesWanted * 2;
    }
    return armoriesWanted;
}

unsigned BuildCalculator::CalcMetalworks()
{
    unsigned ironsmelters = GetNumBuildings(BuildingType::Ironsmelter);
    if(ironsmelters == 0)
    {
        return 0;
    }
    if(!aijh.ggs.isEnabled(AddonId::INEXHAUSTIBLE_MINES))
    {
        return 1;
    }
    return std::min(calcCount(ironsmelters, AI_CONFIG.ironsmelterToMetalworks), (unsigned)AI_CONFIG.maxMetalworks);
}

unsigned BuildCalculator::CalcWells()
{
    unsigned waterOnStore = inventory[GoodType::Water];
    unsigned flourOnStore = inventory[GoodType::Flour];
    unsigned users = calcWaterUsers();
    users -= unsigned(waterOnStore / 50.0);
    users += unsigned(flourOnStore / 50.0);
    return unsigned(AI_CONFIG.wellToUsers.constant + AI_CONFIG.wellToUsers.linear * users);
}

unsigned BuildCalculator::CalcMills()
{
    unsigned millsNum = GetNumBuildings(BuildingType::Mill);
    unsigned foodusers = calcGrainUsers();
    unsigned farms = buildingNums.buildings[BuildingType::Farm];
    unsigned nonMillUsers = foodusers - millsNum;
    if(farms >= nonMillUsers)
        return calcCount(farms - nonMillUsers, AI_CONFIG.freeFarmToMill);
    return millsNum;
}

unsigned BuildCalculator::CalcSawmills()
{
    return calcCount(numMilitaryBlds, AI_CONFIG.milToSawmill);
}

unsigned BuildCalculator::CalcIronsmelter()
{
    unsigned ironMines = GetNumBuildings(BuildingType::IronMine);
    unsigned count = calcCount(ironMines, AI_CONFIG.ironMineToIronsmelter);
    return std::min<unsigned>(count, maxIronFounder(aijh) + 2);
}

unsigned BuildCalculator::calcGrainUsers()
{
    return GetNumBuildings(BuildingType::Mill) + GetNumBuildings(BuildingType::Charburner)
           + GetNumBuildings(BuildingType::Brewery) + GetNumBuildings(BuildingType::PigFarm)
           + GetNumBuildings(BuildingType::DonkeyBreeder);
}

unsigned BuildCalculator::calcWaterUsers()
{
    return GetNumBuildings(BuildingType::Bakery) + GetNumBuildings(BuildingType::PigFarm)
           + GetNumBuildings(BuildingType::DonkeyBreeder) + GetNumBuildings(BuildingType::Brewery);
}

unsigned BuildCalculator::CalcQuarry()
{
    unsigned count = 0;
    if(inventory.goods[GoodType::PickAxe] + inventory.people[Job::Miner] < 7 && inventory.people[Job::Stonemason] > 0
       && inventory.people[Job::Miner] < 3)
    {
        count = std::max(std::min(inventory.people[Job::Stonemason], numMilitaryBlds), 2u);
    } else
    {
        //>6miners = build up to 6 depending on resources, else max out at miners/2
        if(inventory.people[Job::Miner] > 6)
            count = std::min(inventory.goods[GoodType::PickAxe] + inventory.people[Job::Stonemason], 6u);
        else
            count = inventory.people[Job::Miner] / 2;

        if(count > numMilitaryBlds)
            count = numMilitaryBlds;
    }
    unsigned sawmills = GetNumBuildings(BuildingType::Sawmill);
    count = std::max(count, (unsigned)(sawmills / 1.2 + 1));
    count = std::min(inventory.goods[GoodType::PickAxe] + inventory.people[Job::Stonemason] + 2, count);
    return count;
}

unsigned BuildCalculator::GetNumBuildings(BuildingType type)
{
    return buildingNums.buildings[type] + buildingNums.buildingSites[type];
}

unsigned BuildCalculator::calcCount(unsigned x, BuildParams params)
{
    double log2Val = std::max(0.0, std::log(params.logTwo.constant + params.logTwo.linear * x));
    return (unsigned)(params.constant + params.linear * x + log2Val);
}

} // namespace AIJH