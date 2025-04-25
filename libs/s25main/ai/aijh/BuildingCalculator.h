#include "AIPlayerJH.h"
#include "AIConfig.h"

namespace AIJH {
    helpers::EnumArray<unsigned, BuildingType> GetStartupSet(const AIPlayerJH& aijh, unsigned woodAvailable);
    unsigned doGetNumBuildings(BuildingCount bldCount, BuildingType type);
    unsigned CalcForesters(const AIPlayerJH& aijh, unsigned woodAvailable);
    unsigned CalcWoodcutters(const AIPlayerJH& aijh, BuildingCount bldCount, unsigned woodAvailable);
    unsigned CalcPigFarms(BuildingCount buildingNums);
    unsigned CalcFarms(const AIPlayerJH& aijh, unsigned foodusers);
    unsigned CalcArmories(const AIPlayerJH& aijh, BuildingCount buildingNums);
    unsigned CalcBreweries(const AIPlayerJH& aijh, BuildingCount buildingNums);
    unsigned CalcMetalworks(const AIPlayerJH& aijh, BuildingCount buildingNums);
    unsigned CalcWells(const Inventory& inventory, helpers::EnumArray<unsigned, BuildingType> buildingsWanted);

    unsigned GetNumBuildings(BuildingCount buildingNums, BuildingType type);
    unsigned CalcCount(unsigned x, BuildParams params);

} // namespace AIJH