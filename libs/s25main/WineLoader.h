// Copyright (C) 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

///////////////////////////////////////////////////////////////////////////////
#pragma once

#include "helpers/MultiArray.h"
#include "ogl/glSmartBitmap.h"
#include "gameTypes/BuildingType.h"
#include "gameTypes/GoodTypes.h"
#include "gameTypes/JobTypes.h"
#include "gameTypes/Nation.h"
#include "gameTypes/TempleProductionMode.h"

class glArchivItem_Bitmap;
class glTexturePacker;

namespace wineaddon {

bool isWineAddonBuildingType(BuildingType bld);
bool isWineAddonGoodType(GoodType good);
bool isWineAddonJobType(Job job);

enum class BobTypes
{
    WINEGROWER_DIGGING_ANIMATION,
    WINEGROWER_PLANTING_ANIMATION,
    WINEGROWER_PICKING_GRAPES_ANIMATION,
    WINEGROWER_GRAPEFIELDS_ONE,
    WINEGROWER_GRAPEFIELDS_TWO,
    WINEGROWER_GRAPEFIELDS_ONE_SHADOW,
    WINEGROWER_GRAPEFIELDS_TWO_SHADOW,
    WINEGROWER_WALKING_WITH_SHOVEL,
    WINEGROWER_WALKING_WITH_EMPTY_BASKET,
    WINEGROWER_WALKING_WITH_FULL_BASKET,
    VINTNER_WORK_WINDOW,
    VINTNER_WALKING,
    VINTNER_CARRYING_WINE_IN_OUT,
    TEMPLE_WORK_WINDOW_START_ANIMATION,
    TEMPLE_WORK_WINDOW_MAIN_ANIMATION,
    TEMPLE_WORK_WINDOW_END_ANIMATION,
    TEMPLESERVANT_WALKING,
    TEMPLESERVANT_CARRYING_GOLD_IN_OUT,
    TEMPLESERVANT_CARRYING_IRON_IN_OUT,
    TEMPLESERVANT_CARRYING_COAL_IN_OUT,
    TEMPLESERVANT_CARRYING_STONE_IN_OUT,
    THIN_CARRIER_CARRYING_GRAPES,
    FAT_CARRIER_CARRYING_GRAPES,
    THIN_CARRIER_CARRYING_WINE,
    FAT_CARRIER_CARRYING_WINE,
    GRAPES_WARE_ICON,
    WINE_WARE_ICON,
    WINEGROWER_JOB_ICON,
    VINTNER_JOB_ICON,
    TEMPLESERVANT_JOB_ICON,
    TEMPLE_OUTPUT_WARE_ICON_RANDOM,
    GRAPES_WARE_ON_GROUND_OF_FLAG,
    WINE_WARE_ON_GROUND_OF_FLAG,
    DONKEY_BOAT_CARRYING_GRAPES_WARE,
    DONKEY_BOAT_CARRYING_WINE_WARE
};

constexpr auto maxEnumValue(BobTypes)
{
    return BobTypes::DONKEY_BOAT_CARRYING_WINE_WARE;
}

extern helpers::EnumArray<unsigned, BobTypes> bobIndex;
extern helpers::MultiArray<glSmartBitmap, 2, 5> grapefield_cache;

ITexture* GetWareTex(GoodType good);
ITexture* GetWareStackTex(GoodType good);
ITexture* GetWareDonkeyTex(GoodType good);
ITexture* GetJobTex(Job job);
ITexture* GetTempleProductionModeTex(ProductionMode mode);

void fillCache(glTexturePacker& stp);

} // namespace wineaddon
