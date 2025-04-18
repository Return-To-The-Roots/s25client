#include "AIPlayerJH.h"

namespace AIJH {
    helpers::EnumArray<unsigned, BuildingType> GetStartupSet(unsigned numMilitaryBlds, unsigned woodAvailable);
    unsigned doGetNumBuildings(BuildingCount bldCount, BuildingType type);
    unsigned CalcForesters(const AIPlayerJH& aijh, unsigned woodAvailable);
    unsigned CalcPigFarms(BuildingCount buildingNums);
    unsigned CalcFarms(const AIPlayerJH& aijh, unsigned foodusers);
    unsigned CalcWells(const Inventory& inventory, helpers::EnumArray<unsigned, BuildingType> buildingsWanted);

    unsigned GetNumBuildings(BuildingCount buildingNums, BuildingType type);

} // namespace AIJH