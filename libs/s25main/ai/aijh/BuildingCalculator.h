#include "AIPlayerJH.h"

namespace AIJH {
    helpers::EnumArray<unsigned, BuildingType> GetStartupSet(unsigned numMilitaryBlds, unsigned woodAvailable);
    unsigned doGetNumBuildings(BuildingCount bldCount, BuildingType type);
    unsigned CalcForesters(const AIPlayerJH& aijh);

} // namespace AIJH