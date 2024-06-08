// Copyright (C) 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

///////////////////////////////////////////////////////////////////////////////

#include "WineLoader.h"
#include "Loader.h"

namespace wineaddon {

bool isWineAddonBuildingType(BuildingType bld)
{
    return bld == BuildingType::Vineyard || bld == BuildingType::Winery || bld == BuildingType::Temple;
}

bool isWineAddonGoodType(GoodType good)
{
    return good == GoodType::Wine || good == GoodType::Grapes;
}

bool isWineAddonJobType(Job job)
{
    return job == Job::Winegrower || job == Job::Vintner || job == Job::TempleServant;
}

BuildingImages GetBuildingImages(Nation nation, BuildingType buildType)
{
    const unsigned buildingStartIndex =
      1 + rttr::enum_cast(nation) * 27 + (rttr::enum_cast(buildType) - rttr::enum_cast(BuildingType::Vineyard)) * 9;
    return {buildingStartIndex,     buildingStartIndex + 1, buildingStartIndex + 2,
            buildingStartIndex + 3, buildingStartIndex + 4, buildingStartIndex + 5,
            buildingStartIndex + 6, buildingStartIndex + 7, buildingStartIndex + 8};
}

helpers::EnumArray<unsigned, BobTypes> bobIndex = {1,   17,  22,  27,  32,  37,  42,  48,  97,  146, 195, 204,
                                                   253, 270, 282, 289, 295, 344, 361, 378, 395, 412, 461, 510,
                                                   517, 525, 526, 527, 528, 529, 530, 532, 533, 535, 536};

unsigned GetWareTex(const GoodType good)
{
    return bobIndex[good == GoodType::Wine ? BobTypes::WINE_WARE_ICON : BobTypes::GRAPES_WARE_ICON];
}

unsigned GetWareStackTex(const GoodType good)
{
    return bobIndex[good == GoodType::Wine ? BobTypes::WINE_WARE_ON_GROUND_OF_FLAG :
                                             BobTypes::GRAPES_WARE_ON_GROUND_OF_FLAG];
}

unsigned GetWareDonkeyTex(const GoodType good)
{
    return bobIndex[good == GoodType::Wine ? BobTypes::DONKEY_BOAT_CARRYING_WINE_WARE :
                                             BobTypes::DONKEY_BOAT_CARRYING_GRAPES_WARE];
}

unsigned GetJobTex(Job job)
{
    if(job == Job::Winegrower)
        return bobIndex[BobTypes::WINEGROWER_JOB_ICON];
    else if(job == Job::Vintner)
        return bobIndex[BobTypes::VINTNER_JOB_ICON];
    else if(job == Job::TempleServant)
        return bobIndex[BobTypes::TEMPLESERVANT_JOB_ICON];
    return 0;
}

glArchivItem_Bitmap* GetWineImage(unsigned nr)
{
    return LOADER.GetImageN("wine", nr);
}

glArchivItem_Bitmap* GetWineBobImage(unsigned nr)
{
    return LOADER.GetImageN("wine_bobs", nr);
}

ITexture* GetTempleProductionModeTex(ProductionMode productionMode)
{
    switch(productionMode)
    {
        case ProductionMode::Default: return GetWineBobImage(bobIndex[BobTypes::TEMPLE_OUTPUT_WARE_ICON_RANDOM]);
        case ProductionMode::IronOre: return LOADER.GetWareTex(GoodType::IronOre);
        case ProductionMode::Coal: return LOADER.GetWareTex(GoodType::Coal);
        case ProductionMode::Stone: return LOADER.GetWareTex(GoodType::Stones);
    }
    return nullptr;
}

helpers::MultiArray<glSmartBitmap, 2, 5> grapefield_cache;

void fillCache(std::unique_ptr<glTexturePacker>& stp)
{
    for(unsigned type = 0; type < 2; ++type)
    {
        const auto field = bobIndex[BobTypes(rttr::enum_cast(BobTypes::WINEGROWER_GRAPEFIELDS_ONE) + type)];
        const auto shadow = bobIndex[BobTypes(rttr::enum_cast(BobTypes::WINEGROWER_GRAPEFIELDS_ONE_SHADOW) + type)];
        for(unsigned size = 0; size < 5; ++size)
        {
            glSmartBitmap& bmp = grapefield_cache[type][size];
            bmp.reset();
            bmp.add(GetWineBobImage(field + size));
            bmp.addShadow(GetWineBobImage(shadow + size));
            stp->add(bmp);
        }
    }
}

} // namespace wineaddon