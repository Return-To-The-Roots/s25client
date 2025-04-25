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
helpers::EnumArray<unsigned, BuildingType> GetStartupSet(const AIPlayerJH& aijh, unsigned woodAvailable)
{
    const unsigned milCount = aijh.player.GetBuildingRegister().GetMilitaryBuildings().size();

    auto values = helpers::EnumArray<unsigned, BuildingType>();
    auto milToSawmill = AI_CONFIG.startupMilToSawmill;
    auto milToWoodcutter = AI_CONFIG.startupMilToWoodcutter;
    values[BuildingType::Forester] = CalcForesters(aijh, woodAvailable);
    values[BuildingType::Sawmill] = (unsigned)(milToSawmill.constant + milCount * milToSawmill.linear);
    values[BuildingType::Woodcutter] = (unsigned)(milToWoodcutter.constant + milCount * milToWoodcutter.linear);
    values[BuildingType::Quarry] = 1 + milCount / 3;
    values[BuildingType::Fishery] = 1 + milCount / 5;
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

unsigned doGetNumBuildings(const BuildingCount bldCount, BuildingType type)
{
    return bldCount.buildings[type] + bldCount.buildingSites[type];
}
unsigned CalcWoodcutters(const AIPlayerJH& aijh, const BuildingCount bldCount, unsigned woodAvailable)
{
    BuildParams params = AI_CONFIG.sawmillToWoodcutter;
    auto sawmills = GetNumBuildings(bldCount, BuildingType::Sawmill);
    unsigned baseWoodCutters = (unsigned)(params.constant + params.linear * sawmills);

    unsigned woodcutters = GetNumBuildings(bldCount, BuildingType::Woodcutter);

    unsigned forestersActive = bldCount.buildings[BuildingType::Forester];
    unsigned extraWood = woodAvailable - 6 * forestersActive;

    unsigned additional_woodcutters = 0;
    if(baseWoodCutters > 0 && extraWood / baseWoodCutters > 20)
    {
        additional_woodcutters = std::min((unsigned)(woodcutters * 0.4), extraWood / 20);
    }

    unsigned max_available_woodcutter = maxWoodcutter(aijh);
    return std::min(max_available_woodcutter + 2, additional_woodcutters + baseWoodCutters);
}

unsigned CalcForesters(const AIPlayerJH& aijh, unsigned woodAvailable)
{

    BuildingCount bldCount = aijh.player.GetBuildingRegister().GetBuildingNums();
    const Inventory& inventory = aijh.player.GetInventory();
    unsigned max_available_forester = inventory[Job::Forester] + inventory[GoodType::Shovel];
    unsigned additional_forester = doGetNumBuildings(bldCount, BuildingType::Charburner);
    unsigned sawmills = doGetNumBuildings(bldCount, BuildingType::Sawmill);
    signed count = 0u;

    count = CalcCount(sawmills, AI_CONFIG.sawmillToForester);
    count -= (unsigned)(woodAvailable / AI_CONFIG.foresterWoodLevel);

    count += additional_forester;
    count = std::max(1, count);
    count = std::min((signed)max_available_forester + 1, count);
    return count;
}

unsigned CalcCount(unsigned x, BuildParams params)
{
    Logarithmic paramsLog2 = params.logTwo;
    return (unsigned)(params.constant + params.linear * x + std::log(paramsLog2.constant + paramsLog2.linear * x));
}

unsigned CalcPigFarms(const BuildingCount buildingNums)
{
    if(AI_CONFIG.pigfarmMultiplier == 0)
    {
        return 0;
    }
    unsigned farms = GetNumBuildings(buildingNums, BuildingType::Farm);
    unsigned wanted = (farms < 8) ? farms / 4 : (farms - 2) / 4;
    unsigned slaughterhouses = GetNumBuildings(buildingNums, BuildingType::Slaughterhouse);
    if(wanted > slaughterhouses + 1)
        wanted = slaughterhouses + 1;
    return wanted;
}

unsigned CalcFarms(const AIPlayerJH& aijh, unsigned numMilitaryBlds)
{
    return (unsigned)std::min<double>(maxFarmer(aijh) * 1.2, numMilitaryBlds * AI_CONFIG.farmToMil.linear);
}

unsigned CalcBreweries(const AIPlayerJH& aijh, const BuildingCount buildingNums)
{
    unsigned armories = GetNumBuildings(buildingNums, BuildingType::Armory);
    unsigned farms = GetNumBuildings(buildingNums, BuildingType::Farm);
    if(armories == 0 || farms < 3)
    {
        return 0;
    }
    BuildParams params = AI_CONFIG.breweryToArmory;
    if(aijh.ggs.isEnabled(AddonId::INEXHAUSTIBLE_MINES))
        return unsigned(params.constant + armories * params.linear);
    return 1 + armories / 6;
}

unsigned CalcArmories(const AIPlayerJH& aijh, const BuildingCount buildingNums)
{
    unsigned ironsmelters = GetNumBuildings(buildingNums, BuildingType::Ironsmelter);
    unsigned metalworks = GetNumBuildings(buildingNums, BuildingType::Metalworks);
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

unsigned CalcMetalworks(const AIPlayerJH& aijh, const BuildingCount buildingNums)
{
    unsigned ironsmelters = GetNumBuildings(buildingNums, BuildingType::Ironsmelter);
    if(ironsmelters == 0)
    {
        return 0;
    }
    if(!aijh.ggs.isEnabled(AddonId::INEXHAUSTIBLE_MINES))
    {
        return 1;
    }
    return std::min(CalcCount(ironsmelters, AI_CONFIG.metalworksToIronsmelter), (unsigned)AI_CONFIG.maxMetalworks + 1);
}
unsigned CalcWells(const Inventory& inventory, helpers::EnumArray<unsigned, BuildingType> buildingsWanted)
{
    unsigned waterOnStore = inventory[GoodType::Water];
    unsigned flourOnStore = inventory[GoodType::Flour];
    unsigned users = buildingsWanted[BuildingType::Bakery] + buildingsWanted[BuildingType::PigFarm]
                     + buildingsWanted[BuildingType::DonkeyBreeder] + buildingsWanted[BuildingType::Brewery];
    users -= (unsigned)(waterOnStore / 50.0);
    users += (unsigned)(flourOnStore / 50.0);
    return (unsigned)(AI_CONFIG.wellToUsers.constant + AI_CONFIG.wellToUsers.linear * users);
}

unsigned GetNumBuildings(BuildingCount buildingNums, BuildingType type)
{
    return buildingNums.buildings[type] + buildingNums.buildingSites[type];
}
} // namespace AIJH