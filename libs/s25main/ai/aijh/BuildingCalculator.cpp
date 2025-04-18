#include "BuildingCalculator.h"

#include "AIConfig.h"
#include "AIPlayerJH.h"
#include "BuildingPlanner.h"
#include "PlannerHelper.h"

#include "gameTypes/GoodTypes.h"
#include "gameTypes/JobTypes.h"

#include <cmath>

class AIPlayer;
struct Inventory;
class GamePlayer;

namespace AIJH {
helpers::EnumArray<unsigned, BuildingType> GetStartupSet(unsigned numMilitaryBlds, unsigned woodAvailable)
{
    auto values = helpers::EnumArray<unsigned, BuildingType>();
    auto milToSawmill = AI_CONFIG.startupMilToSawmill;
    auto milToWoodcutter = AI_CONFIG.startupMilToWoodcutter;
    values[BuildingType::Forester] = 1 + numMilitaryBlds / 7;
    values[BuildingType::Sawmill] = (unsigned)(milToSawmill.constant + numMilitaryBlds * milToSawmill.linear);
    values[BuildingType::Woodcutter] = (unsigned)(milToWoodcutter.constant + numMilitaryBlds * milToWoodcutter.linear);
    values[BuildingType::Quarry] = 1 + numMilitaryBlds / 3;
    values[BuildingType::GraniteMine] = -1;
    values[BuildingType::CoalMine] = -1;
    values[BuildingType::IronMine] = -1;
    values[BuildingType::GoldMine] = -1;
    values[BuildingType::Catapult] = -1;
    values[BuildingType::Fishery] = -1;
    values[BuildingType::Hunter] = -1;
    values[BuildingType::Farm] = -1;
    values[BuildingType::Charburner] = -1;
    values[BuildingType::Ironsmelter] = -1;
    values[BuildingType::Metalworks] = -1;

    if(woodAvailable < 15000)
    {
        values[BuildingType::Forester]++;
    }
    return values;
}

unsigned doGetNumBuildings(const BuildingCount bldCount, BuildingType type)
{
    return bldCount.buildings[type] + bldCount.buildingSites[type];
}

unsigned CalcForesters(const AIPlayerJH& aijh, unsigned woodAvailable)
{
    const unsigned numMilitaryBlds = aijh.player.GetBuildingRegister().GetMilitaryBuildings().size();

    BuildingCount bldCount = aijh.player.GetBuildingRegister().GetBuildingNums();
    const Inventory& inventory = aijh.player.GetInventory();
    unsigned max_available_forester = inventory[Job::Forester] + inventory[GoodType::Shovel];
    unsigned additional_forester = doGetNumBuildings(bldCount, BuildingType::Charburner);

    unsigned count = 0;
    if(numMilitaryBlds > 0)
        count = (unsigned)(std::log(2 + numMilitaryBlds) + 1);

    if(woodAvailable > AI_CONFIG.foresterWoodLevel)
    {
        return std::min(count, unsigned(2));
    }
    // If we are low on wood, we need more foresters
    if(aijh.player.GetBuildingRegister().GetBuildingSites().size()
       > (inventory[GoodType::Boards] + inventory[GoodType::Wood]) * 2)
        additional_forester++;

    count += additional_forester;
    count = std::min(max_available_forester, count);
    return count;
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