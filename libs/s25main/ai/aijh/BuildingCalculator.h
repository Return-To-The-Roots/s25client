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

    unsigned CalcIronsmelter();
    unsigned CalcForesters();
    unsigned CalcWoodcutters();
    unsigned CalcPigFarms();
    unsigned CalcQuarry();
    unsigned CalcMills();
    unsigned CalcSawmills();
    unsigned CalcArmories();
    unsigned CalcWells();
    unsigned CalcFarms();
    unsigned CalcBreweries();
    unsigned CalcIronMines();

private:
    const AIPlayerJH& aijh;
    BuildingCount buildingNums;
    const Inventory& inventory;
    unsigned woodAvailable;
    const unsigned numMilitaryBlds;

    unsigned doCalc(BuildingType type);
    unsigned calcGrainUsers();
    unsigned calcWaterUsers();
    unsigned GetNumBuildings(BuildingType type);
    unsigned calcCount(unsigned x, BuildParams params);
};


unsigned GetNumBuildings(BuildingCount buildingNums, BuildingType type);

} // namespace AIJH