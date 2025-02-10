#include "BuildingCalculator.h"

#include "AIPlayerJH.h"
#include "BuildingPlanner.h"

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
    values[BuildingType::Forester] = 1 + numMilitaryBlds / 7;
    values[BuildingType::Sawmill] = 3 + unsigned (numMilitaryBlds / 3);
    values[BuildingType::Woodcutter] = 3 + numMilitaryBlds / 3;
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

unsigned CalcForesters(const AIPlayerJH& aijh)
{
    const unsigned numMilitaryBlds = aijh.player.GetBuildingRegister().GetMilitaryBuildings().size();

    BuildingCount bldCount = aijh.player.GetBuildingRegister().GetBuildingNums();
    const Inventory& inventory = aijh.player.GetInventory();
    unsigned max_available_forester = inventory[Job::Forester] + inventory[GoodType::Shovel];
    unsigned additional_forester = doGetNumBuildings(bldCount, BuildingType::Charburner);

    unsigned count = 0;
    if(numMilitaryBlds > 0)
        count = (unsigned)(std::log(2 + numMilitaryBlds) + 1);
    // If we are low on wood, we need more foresters
    if(aijh.player.GetBuildingRegister().GetBuildingSites().size()
       > (inventory[GoodType::Boards] + inventory[GoodType::Wood]) * 2)
        additional_forester++;

    count += additional_forester;
    count = std::min(max_available_forester, count);
    return count;
}

} // namespace AIJH