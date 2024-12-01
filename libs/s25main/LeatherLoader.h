// Copyright (C) 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

///////////////////////////////////////////////////////////////////////////////
#pragma once

#include "helpers/EnumArray.h"
#include "ogl/glSmartBitmap.h"
#include "gameTypes/BuildingType.h"
#include "gameTypes/GoodTypes.h"
#include "gameTypes/JobTypes.h"

class glArchivItem_Bitmap;
class glTexturePacker;
class GameWorldBase;

namespace leatheraddon {

bool isLeatherAddonBuildingType(BuildingType bld);
bool isLeatherAddonGoodType(GoodType good);
bool isLeatherAddonJobType(Job job);

enum class BobTypes
{
    SKINNER_SKINNING_ANIMAL_CARCASS_ANIMATION,
    SKINNER_WALKING,
    SKINNER_CARRYING_SKINS,
    TANNERY_WORK_WINDOW_ANIMATION,
    TANNER_WALKING,
    TANNER_CARRYING_LEATHER_IN_OUT,
    LEATHERWORKS_WORK_WINDOW_ANIMATION,
    LEATHERWORKER_WALKING,
    LEATHERWORKER_CARRYING_ARMOR_IN_OUT,
    THIN_CARRIER_CARRYING_SKINS,
    FAT_CARRIER_CARRYING_SKINS,
    THIN_CARRIER_CARRYING_LEATHER,
    FAT_CARRIER_CARRYING_LEATHER,
    THIN_CARRIER_CARRYING_ARMOR,
    FAT_CARRIER_CARRYING_ARMOR,
    DISTRIBUTION_OF_PIGS_ICON,
    SKINS_WARE_ICON,
    LEATHER_WARE_ICON,
    LEATHERWORKING_WARES_TRANSPORT_PRIORITY_TREE_ICON,
    ARMOR_WARE_ICON,
    ARMOR_DELIVER_ICON,
    DISABLE_DELIVERY_ARMOR_ICON,
    SKINNER_JOB_ICON,
    TANNER_JOB_ICON,
    LEATHERWORKER_JOB_ICON,
    SKINS_WARE_ON_GROUND_OF_FLAG,
    DONKEY_BOAT_CARRYING_SKINS_WARE,
    LEATHER_WARE_ON_GROUND_OF_FLAG,
    DONKEY_BOAT_CARRYING_LEATHER_WARE,
    ARMOR_WARE_ON_GROUND_OF_FLAG,
    DONKEY_BOAT_CARRYING_ARMOR_WARE,
    STOP_COINS_X_SIGN_OVERRIDE,
    STOP_ARMOR_X_SIGN
};

constexpr auto maxEnumValue(BobTypes)
{
    return BobTypes::STOP_ARMOR_X_SIGN;
}

extern helpers::EnumArray<unsigned, BobTypes> bobIndex;

ITexture* GetWareTex(GoodType good);
ITexture* GetWareStackTex(GoodType good);
ITexture* GetWareDonkeyTex(GoodType good);
ITexture* GetJobTex(Job job);
bool isAddonActive(const GameWorldBase& gwb);

} // namespace leatheraddon
