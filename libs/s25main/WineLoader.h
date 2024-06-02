// Copyright (C) 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

///////////////////////////////////////////////////////////////////////////////
#pragma once

#include "helpers/MultiArray.h"
#include "ogl/glTexturePacker.h"
#include "ogl/glSmartBitmap.h"
#include "ogl/glArchivItem_Bitmap.h"
#include "gameTypes/BuildingType.h"
#include "gameTypes/GoodTypes.h"
#include "gameTypes/JobTypes.h"
#include "gameTypes/Nation.h"
#include <map>

// Wine Addon animation and images

namespace wineaddon {


struct BuildingImages
{
    unsigned building;
    unsigned building_shadow;
    unsigned buildingskeleton;
    unsigned buildingskeleton_shadow;
    unsigned door;
    unsigned building_winter;
    unsigned building_shadow_winter;
    unsigned door_winter;
    unsigned icon;
};

bool isWineAddonBuildingType(BuildingType bld);
bool isWineAddonGoodType(GoodType good);
bool isWineAddonJobType(Job job);

extern std::map<std::pair<Nation, BuildingType>, BuildingImages> buildings;
using BobEntryRange = std::pair<unsigned, unsigned>;
struct BobEntry
{
    BobEntryRange bobEntryRange;
    std::string description;
};

enum BobTypes
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
    DONKEY_BOAT_CARRYING_WINE_WARE,

};

extern std::map<BobTypes, BobEntry> bobs;

unsigned getStartIndexOfBob(const BobTypes bobType);
unsigned GetWareTex(const GoodType good);
unsigned GetWareStackTex(const GoodType good);
unsigned GetWareDonkeyTex(const GoodType good);
unsigned GetJobTex(const Job job);
glArchivItem_Bitmap* GetWineImage(unsigned nr);
glArchivItem_Bitmap* GetWineBobImage(unsigned nr);

/// Grapefield: Type, Size
extern helpers::MultiArray<glSmartBitmap, 2, 5> grapefield_cache;

void fillCache(std::unique_ptr<glTexturePacker>& stp);
} // namespace wineaddon