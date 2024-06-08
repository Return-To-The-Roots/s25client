// Copyright (C) 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

///////////////////////////////////////////////////////////////////////////////

#include "WineLoader.h"
#include "Loader.h"
#include "gameTypes/PactTypes.h"

// Wine Addon animation and images
namespace wineaddon {

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
  {BobTypes::VINTNER_WORK_WINDOW,
   {{195, 202}, "Winery work window - Vintner stomping grapes in barrel (loop for duration)"}},
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

BuildingImages GetBuildingImages(Nation nation, BuildingType buildType)
{
    const unsigned buildingStartIndex =
      1 + rttr::enum_cast(nation) * 27 + (rttr::enum_cast(buildType) - rttr::enum_cast(BuildingType::Vineyard)) * 9;
    return {buildingStartIndex,     buildingStartIndex + 1, buildingStartIndex + 2,
            buildingStartIndex + 3, buildingStartIndex + 4, buildingStartIndex + 5,
            buildingStartIndex + 6, buildingStartIndex + 7, buildingStartIndex + 8};
}

unsigned getStartIndexOfBob(const BobTypes bobType)
{
    return bobs[bobType].bobEntryRange.first;
}

unsigned GetWareTex(const GoodType good)
{
    return getStartIndexOfBob(good == GoodType::Wine ? BobTypes::WINE_WARE_ICON : BobTypes::GRAPES_WARE_ICON);
}

unsigned GetWareStackTex(const GoodType good)
{
    return getStartIndexOfBob(good == GoodType::Wine ? BobTypes::WINE_WARE_ON_GROUND_OF_FLAG :
                                                       BobTypes::GRAPES_WARE_ON_GROUND_OF_FLAG);
}

unsigned GetWareDonkeyTex(const GoodType good)
{
    return getStartIndexOfBob(good == GoodType::Wine ? BobTypes::DONKEY_BOAT_CARRYING_WINE_WARE :
                                                       BobTypes::DONKEY_BOAT_CARRYING_GRAPES_WARE);
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

ITexture* GetTempleProductionModeTex(ProductionMode productionMode)
{
    switch(productionMode)
    {
        case ProductionMode::Default:
            return GetWineBobImage(getStartIndexOfBob(BobTypes::TEMPLE_OUTPUT_WARE_ICON_RANDOM));
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
        const auto field = getStartIndexOfBob(BobTypes(rttr::enum_cast(BobTypes::WINEGROWER_GRAPEFIELDS_ONE) + type));
        const auto shadow =
          getStartIndexOfBob(BobTypes(rttr::enum_cast(BobTypes::WINEGROWER_GRAPEFIELDS_ONE_SHADOW) + type));
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