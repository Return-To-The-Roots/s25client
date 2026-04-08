#pragma once

#include "ai/AIResource.h"
#include "helpers/EnumArray.h"
#include "gameTypes/BuildingCount.h"
#include "gameTypes/BuildingType.h"

struct Inventory;

namespace AIJH {
class AIWorldView;

class BuildCalculator
{
public:
    BuildCalculator(const AIWorldView& aijh, BuildingCount buildingNums, const Inventory& inventory,
                    unsigned woodAvailable);

    helpers::EnumArray<unsigned, BuildingType> GetStartupSet();

    unsigned Calc(BuildingType type);
    unsigned CalcPigFarms();

private:
    const AIWorldView& aijh;
    BuildingCount buildingNums;
    const Inventory& inventory;
    unsigned woodAvailable;
    const unsigned numMilitaryBlds;

    unsigned doCalc(BuildingType type);
    unsigned GetNumBuildings(BuildingType type);
    unsigned getAvailableResource(AIResource resType);
};


unsigned GetNumBuildings(BuildingCount buildingNums, BuildingType type);

} // namespace AIJH
