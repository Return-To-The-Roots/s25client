// Copyright (C) 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

///////////////////////////////////////////////////////////////////////////////

#include "WineLoader.h"
#include "Loader.h"

// Wine Addon animation and images


constexpr unsigned NOT_AVAILABLE = 0;
constexpr unsigned EMPTY = 0;

namespace wineaddon {
std::map<std::pair<Nation, BuildingType>, BuildingImages> buildings{
  {{Nation::Africans, BuildingType::Vineyard}, {1, 2, 3, 4, 5, 6, NOT_AVAILABLE, 7, 8}},
  {{Nation::Africans, BuildingType::Winery}, {9, 10, 11, 12, 13, 14, NOT_AVAILABLE, 15, 16}},
  {{Nation::Africans, BuildingType::Temple}, {17, 18, 19, 20, 21, 22, 23, 24, 25}},
  {{Nation::Japanese, BuildingType::Vineyard}, {26, 27, 28, 29, 30, 31, NOT_AVAILABLE, 32, 33}},
  {{Nation::Japanese, BuildingType::Winery}, {34, 35, 36, 37, 38, 39, NOT_AVAILABLE, 40, 41}},
  {{Nation::Japanese, BuildingType::Temple}, {42, 43, 44, 45, 46, 47, 48, 49, 50}},
  {{Nation::Romans, BuildingType::Vineyard}, {51, 52, 53, 54, 55, 56, NOT_AVAILABLE, 57, 58}},
  {{Nation::Romans, BuildingType::Winery}, {59, 60, 61, 62, 63, 64, NOT_AVAILABLE, 65, 66}},
  {{Nation::Romans, BuildingType::Temple}, {67, 68, 69, 70, 71, 72, NOT_AVAILABLE, 73, 74}},
  {{Nation::Vikings, BuildingType::Vineyard}, {75, 76, 77, 78, 79, 80, NOT_AVAILABLE, 81, 82}},
  {{Nation::Vikings, BuildingType::Winery}, {83, 84, 85, 86, 87, 88, 89, 90, 91}},
  {{Nation::Vikings, BuildingType::Temple}, {92, 93, 94, 95, 96, 97, 98, 99, 100}},
  {{Nation::Babylonians, BuildingType::Vineyard}, {101, 102, 103, 104, 105, 106, NOT_AVAILABLE, 107, 108}},
  {{Nation::Babylonians, BuildingType::Winery}, {109, 110, 111, 112, 113, 114, NOT_AVAILABLE, 115, 116}},
  {{Nation::Babylonians, BuildingType::Temple}, {117, 118, 119, 120, EMPTY, 122, NOT_AVAILABLE, EMPTY, 124}}};

    std::map<BobTypes, BobEntry> bobs = {
  {BobTypes::WINEGROWER_DIGGING_ANIMATION, {{1, 15}, "Winegrower digging animation (setup grape field, loop)"}},
  {BobTypes::WINEGROWER_PLANTING_ANIMATION,
   {{17, 20}, "Winegrower planting animation (setup field after digging, loop)"}},
  {BobTypes::WINEGROWER_PICKING_GRAPES_ANIMATION,
   {{22, 25}, "Winegrower picking grapes animation (harvest field, loop for duration)"}},
  {BobTypes::WINEGROWER_GRAPEFIELDS_ONE, {{27, 31}, "Grape field A"}},
  {BobTypes::WINEGROWER_GRAPEFIELDS_TWO, {{32, 36}, "Grape field B"}},
  {BobTypes::WINEGROWER_GRAPEFIELDS_ONE_SHADOW, {{37, 41}, "Grape field A shadows"}},
  {BobTypes::WINEGROWER_GRAPEFIELDS_TWO_SHADOW, {{42, 46}, "Grape field B shadows"}},
  {BobTypes::WINEGROWER_WALKING_WITH_SHOVEL,
   {{48, 95}, "Winegrower walking with shovel (walk to setup field and back to house)"}},
  {BobTypes::WINEGROWER_WALKING_WITH_EMPTY_BASKET,
   {{97, 144}, "Winegrower walking with empty basket (walking to harvest grapes)"}},
  {BobTypes::WINEGROWER_WALKING_WITH_FULL_BASKET,
   {{146, 193}, "Winery work window - Vintner stomping grapes in barrel (loop for duration)"}},
  {BobTypes::VINTNER_WORK_WINDOW, {{195,202}, "Winery work window - Vintner stomping grapes in barrel (loop for duration)"}},
  {BobTypes::VINTNER_WALKING, {{204, 251}, "Vintner walking"}},
  {BobTypes::VINTNER_CARRYING_WINE_IN_OUT, {{253, 268}, "Vintner carrying wine out (and back in if flag is full)"}},
  {BobTypes::TEMPLE_WORK_WINDOW_START_ANIMATION,
   {{270, 280}, "Temple work window - servant pours wine into crucible (play once at start)"}},
  {BobTypes::TEMPLE_WORK_WINDOW_MAIN_ANIMATION,
   {{282, 287}, "Temple work window - wine and food is burned (loop for duration)"}},
  {BobTypes::TEMPLE_WORK_WINDOW_END_ANIMATION,
   {{289, 293}, "Temple work window - last loop of animation (play once at end)"}},
  {BobTypes::TEMPLESERVANT_WALKING, {{295, 342}, "Temple servant walking"}},
  {BobTypes::TEMPLESERVANT_CARRYING_GOLD_IN_OUT,
   {{344, 359}, "Temple servant carrying gold out (and back in if flag is full)"}},
  {BobTypes::TEMPLESERVANT_CARRYING_IRON_IN_OUT,
   {{361, 376}, "Temple servant carrying iron ore out (and back in if flag is full)"}},
  {BobTypes::TEMPLESERVANT_CARRYING_COAL_IN_OUT,
   {{378, 393}, "Temple servant carrying coal out (and back in if flag is full)"}},
  {BobTypes::TEMPLESERVANT_CARRYING_STONE_IN_OUT,
   {{395, 410}, "Temple servant carrying stone out (and back in if flag is full)"}},
  {BobTypes::THIN_CARRIER_CARRYING_GRAPES, {{412, 459}, "Thin carrier carrying grapes"}},
  {BobTypes::FAT_CARRIER_CARRYING_GRAPES, {{461, 508}, "Fat carrier carrying grapes"}},
  {BobTypes::THIN_CARRIER_CARRYING_WINE,
   {{510, 515}, "Thin carrier head carrying wine (stitch to legs; same as beer)"}},
  {BobTypes::FAT_CARRIER_CARRYING_WINE, {{517, 523}, "Fat carrier head carrying wine (stitch to legs; same as beer)"}},
  {BobTypes::GRAPES_WARE_ICON, {{525, 525}, "Grapes Ware Icon"}},
  {BobTypes::WINE_WARE_ICON, {{526, 526}, "Wine Ware Icon"}},
  {BobTypes::WINEGROWER_JOB_ICON, {{527, 527}, "Winegrower Job Icon"}},
  {BobTypes::VINTNER_JOB_ICON, {{528, 528}, "Vintner Job Icon"}},
  {BobTypes::TEMPLESERVANT_JOB_ICON, {{529, 529}, "Temple Servant Job Icon"}},
  {BobTypes::TEMPLE_OUTPUT_WARE_ICON_RANDOM,
   {{530, 530}, "Temple output ware icon/default ware setting (random mineral)"}},
  {BobTypes::GRAPES_WARE_ON_GROUND_OF_FLAG, {{532, 532}, "Grapes Ware (on ground at flag)"}},
  {BobTypes::WINE_WARE_ON_GROUND_OF_FLAG, {{533, 533}, "Wine Ware (on ground at flag)"}},
  {BobTypes::DONKEY_BOAT_CARRYING_GRAPES_WARE, {{535, 535}, "Donkey/boat carrying grapes ware"}},
  {BobTypes::DONKEY_BOAT_CARRYING_WINE_WARE, {{536, 536}, "Donkey/boat carrying wine ware"}}};

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

unsigned getStartIndexOfBob(const BobTypes bobType)
{
    return bobs[bobType].bobEntryRange.first;
}

unsigned GetWareTex(const GoodType good)
{
    return getStartIndexOfBob(good == GoodType::Wine ? WINE_WARE_ICON : GRAPES_WARE_ICON);
}

unsigned GetWareStackTex(const GoodType good)
{
    return getStartIndexOfBob(good == GoodType::Wine ? WINE_WARE_ON_GROUND_OF_FLAG : GRAPES_WARE_ON_GROUND_OF_FLAG);
}

unsigned GetWareDonkeyTex(const GoodType good)
{
    return getStartIndexOfBob(good == GoodType::Wine ? DONKEY_BOAT_CARRYING_WINE_WARE :
                                                       DONKEY_BOAT_CARRYING_GRAPES_WARE);
}

unsigned GetJobTex(Job job)
{
    if(job == Job::Winegrower)
        return getStartIndexOfBob(BobTypes::WINEGROWER_JOB_ICON);
    else if(job == Job::Vintner)
        return getStartIndexOfBob(BobTypes::VINTNER_JOB_ICON);
    else if(job == Job::TempleServant)
        return getStartIndexOfBob(BobTypes::TEMPLESERVANT_JOB_ICON);
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


helpers::MultiArray<glSmartBitmap, 2, 5> grapefield_cache;

void fillCache(std::unique_ptr<glTexturePacker>& stp)
{
    for(unsigned type = 0; type < 2; ++type)
    {
        const auto field = getStartIndexOfBob(BobTypes(WINEGROWER_GRAPEFIELDS_ONE + type));
        const auto shadow = getStartIndexOfBob(BobTypes(WINEGROWER_GRAPEFIELDS_ONE_SHADOW + type));
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