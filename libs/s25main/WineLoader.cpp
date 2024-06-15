// Copyright (C) 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

///////////////////////////////////////////////////////////////////////////////

#include "WineLoader.h"
#include "GlobalGameSettings.h"
#include "Loader.h"
#include "addons/const_addons.h"
#include "ogl/glArchivItem_Bitmap.h"
#include "ogl/glTexturePacker.h"
#include "world/GameWorldBase.h"

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

helpers::EnumArray<unsigned, BobTypes> bobIndex = {1,   17,  22,  27,  32,  37,  42,  48,  97,  146, 195, 204,
                                                   253, 270, 282, 289, 295, 344, 361, 378, 395, 412, 461, 510,
                                                   517, 525, 526, 527, 528, 529, 530, 532, 533, 535, 536};

ITexture* GetWareTex(const GoodType good)
{
    return LOADER.GetImageN("wine_bobs",
                            bobIndex[good == GoodType::Wine ? BobTypes::WINE_WARE_ICON : BobTypes::GRAPES_WARE_ICON]);
}

ITexture* GetWareStackTex(const GoodType good)
{
    return LOADER.GetImageN("wine_bobs", bobIndex[good == GoodType::Wine ? BobTypes::WINE_WARE_ON_GROUND_OF_FLAG :
                                                                           BobTypes::GRAPES_WARE_ON_GROUND_OF_FLAG]);
}

ITexture* GetWareDonkeyTex(const GoodType good)
{
    return LOADER.GetImageN("wine_bobs", bobIndex[good == GoodType::Wine ? BobTypes::DONKEY_BOAT_CARRYING_WINE_WARE :
                                                                           BobTypes::DONKEY_BOAT_CARRYING_GRAPES_WARE]);
}

ITexture* GetJobTex(Job job)
{
    switch(job)
    {
        case Job::Winegrower: return LOADER.GetImageN("wine_bobs", bobIndex[BobTypes::WINEGROWER_JOB_ICON]);
        case Job::Vintner: return LOADER.GetImageN("wine_bobs", bobIndex[BobTypes::VINTNER_JOB_ICON]);
        case Job::TempleServant: return LOADER.GetImageN("wine_bobs", bobIndex[BobTypes::TEMPLESERVANT_JOB_ICON]);
        default: return nullptr;
    }
}

ITexture* GetTempleProductionModeTex(ProductionMode productionMode)
{
    switch(productionMode)
    {
        case ProductionMode::Default:
            return LOADER.GetImageN("wine_bobs", bobIndex[BobTypes::TEMPLE_OUTPUT_WARE_ICON_RANDOM]);
        case ProductionMode::IronOre: return LOADER.GetWareTex(GoodType::IronOre);
        case ProductionMode::Coal: return LOADER.GetWareTex(GoodType::Coal);
        case ProductionMode::Stone: return LOADER.GetWareTex(GoodType::Stones);
    }
    return nullptr;
}

bool isAddonActive(const GameWorldBase& gwb)
{
    return gwb.GetGGS().isEnabled(AddonId::WINE);
}

helpers::MultiArray<glSmartBitmap, 2, 5> grapefield_cache;

void fillCache(glTexturePacker& stp)
{
    for(unsigned type = 0; type < 2; ++type)
    {
        const auto field = bobIndex[BobTypes(rttr::enum_cast(BobTypes::WINEGROWER_GRAPEFIELDS_ONE) + type)];
        const auto shadow = bobIndex[BobTypes(rttr::enum_cast(BobTypes::WINEGROWER_GRAPEFIELDS_ONE_SHADOW) + type)];
        for(unsigned size = 0; size < 5; ++size)
        {
            glSmartBitmap& bmp = grapefield_cache[type][size];
            bmp.reset();
            bmp.add(LOADER.GetImageN("wine_bobs", field + size));
            bmp.addShadow(LOADER.GetImageN("wine_bobs", shadow + size));
            stp.add(bmp);
        }
    }
}

} // namespace wineaddon