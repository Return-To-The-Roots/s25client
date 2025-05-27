#include "AIConfig.h"
#include "AIPlayerJH.h"

namespace AIJH {
class BuildCalculator
{
public:
    BuildCalculator(const AIPlayerJH& aijh, BuildingCount buildingNums, const Inventory& inventory,
                    unsigned woodAvailable);

    helpers::EnumArray<unsigned, BuildingType> GetStartupSet();

    unsigned Calc(BuildingType type);

    unsigned CalcWoodcutters();
    unsigned CalcPigFarms();
    unsigned CalcFarms();

private:
    const AIPlayerJH& aijh;
    BuildingCount buildingNums;
    const Inventory& inventory;
    unsigned woodAvailable;
    const unsigned numMilitaryBlds;

    unsigned doCalc(BuildingType type);
    unsigned calcGrainUsers();
    unsigned GetNumBuildings(BuildingType type);
    double calcCount(unsigned x, BuildParams params);
    unsigned getAvailableResource(AIResource resType);
};


unsigned GetNumBuildings(BuildingCount buildingNums, BuildingType type);

} // namespace AIJH