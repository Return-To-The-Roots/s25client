// Copyright (C) 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

///////////////////////////////////////////////////////////////////////////////

#include "LeatherLoader.h"
#include "GlobalGameSettings.h"
#include "Loader.h"
#include "addons/const_addons.h"
#include "ogl/glArchivItem_Bitmap.h"
#include "ogl/glTexturePacker.h"
#include "world/GameWorldBase.h"

namespace leatheraddon {

helpers::EnumArray<unsigned, BobTypes> bobIndex = {0,   0,   21,  69,  117, 125, 173, 221, 244, 292, 340, 388,
                                                   436, 484, 532, 580, 628, 629, 630, 630, 631, 631, 632, 633,
                                                   634, 635, 636, 637, 638, 639, 640, 641, 642, 643};

ITexture* GetWareTex(const GoodType good)
{
    switch(good)
    {
        case GoodType::Skins: return LOADER.GetImageN("leather_bobs", bobIndex[BobTypes::SKINS_WARE_ICON]);
        case GoodType::Leather: return LOADER.GetImageN("leather_bobs", bobIndex[BobTypes::LEATHER_WARE_ICON]);
        case GoodType::Armor: return LOADER.GetImageN("leather_bobs", bobIndex[BobTypes::ARMOR_WARE_ICON]);
        default: return nullptr;
    }
}

ITexture* GetWareStackTex(const GoodType good)
{
    switch(good)
    {
        case GoodType::Skins: return LOADER.GetImageN("leather_bobs", bobIndex[BobTypes::SKINS_WARE_ON_GROUND_OF_FLAG]);
        case GoodType::Leather:
            return LOADER.GetImageN("leather_bobs", bobIndex[BobTypes::LEATHER_WARE_ON_GROUND_OF_FLAG]);
        case GoodType::Armor: return LOADER.GetImageN("leather_bobs", bobIndex[BobTypes::ARMOR_WARE_ON_GROUND_OF_FLAG]);
        default: return nullptr;
    }
}

ITexture* GetWareDonkeyTex(const GoodType good)
{
    switch(good)
    {
        case GoodType::Skins:
            return LOADER.GetImageN("leather_bobs", bobIndex[BobTypes::DONKEY_BOAT_CARRYING_SKINS_WARE]);
        case GoodType::Leather:
            return LOADER.GetImageN("leather_bobs", bobIndex[BobTypes::DONKEY_BOAT_CARRYING_LEATHER_WARE]);
        case GoodType::Armor:
            return LOADER.GetImageN("leather_bobs", bobIndex[BobTypes::DONKEY_BOAT_CARRYING_ARMOR_WARE]);
        default: return nullptr;
    }
}

ITexture* GetJobTex(Job job)
{
    switch(job)
    {
        case Job::Skinner: return LOADER.GetImageN("leather_bobs", bobIndex[BobTypes::SKINNER_JOB_ICON]);
        case Job::Tanner: return LOADER.GetImageN("leather_bobs", bobIndex[BobTypes::TANNER_JOB_ICON]);
        case Job::LeatherWorker: return LOADER.GetImageN("leather_bobs", bobIndex[BobTypes::LEATHERWORKER_JOB_ICON]);
        default: return nullptr;
    }
}

bool isAddonActive(const GameWorldBase& gwb)
{
    return gwb.GetGGS().isEnabled(AddonId::LEATHER);
}

BobTypes wareToCarrierBobIndex(const GoodType good, const bool fat)
{
    switch(good)
    {
        default: return BobTypes::INVALID;
        case GoodType::Skins: return fat ? BobTypes::FAT_CARRIER_CARRYING_SKINS : BobTypes::THIN_CARRIER_CARRYING_SKINS;
        case GoodType::Leather:
            return fat ? BobTypes::FAT_CARRIER_CARRYING_LEATHER : BobTypes::THIN_CARRIER_CARRYING_LEATHER;
        case GoodType::Armor: return fat ? BobTypes::FAT_CARRIER_CARRYING_ARMOR : BobTypes::THIN_CARRIER_CARRYING_ARMOR;
    }
}

} // namespace leatheraddon