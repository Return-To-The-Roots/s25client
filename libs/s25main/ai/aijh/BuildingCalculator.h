#include "AIPlayerJH.h"

namespace AIJH {
    helpers::EnumArray<unsigned, BuildingType> GetStartupSet(const AIPlayerJH& aijh, unsigned numMilitaryBlds);
    unsigned doGetNumBuildings(const BuildingCount bldCount, BuildingType type);
    unsigned CalcForesters(const AIPlayerJH& aijh);

} // namespace AIJH