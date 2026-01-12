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

inline bool isLeatherAddonBuildingType(BuildingType bld)
{
    return bld == BuildingType::Skinner || bld == BuildingType::Tannery || bld == BuildingType::LeatherWorks;
}

inline bool isLeatherAddonGoodType(GoodType good)
{
    return good == GoodType::Skins || good == GoodType::Leather || good == GoodType::Armor;
}

inline bool isLeatherAddonJobType(Job job)
{
    return job == Job::Skinner || job == Job::Tanner || job == Job::LeatherWorker;
}

enum class BobType
{
    SkinnerSkinningAnimalCarcassAnimation,
    SkinnerWalking,
    SkinnerCarryingSkins,
    TanneryWorkWindowAnimation,
    TannerWalking,
    TannerCarryingLeatherInOut,
    LeatherworksWorkWindowAnimation,
    LeatherworkerWalking,
    LeatherworkerCarryingArmorInOut,
    ThinCarrierCarryingSkins,
    FatCarrierCarryingSkins,
    ThinCarrierCarryingLeather,
    FatCarrierCarryingLeather,
    ThinCarrierCarryingArmor,
    FatCarrierCarryingArmor,
    DistributionOfPigsIcon,
    SkinsWareIcon,
    LeatherWareIcon,
    LeatherworkingWaresTransportPriorityTreeIcon,
    ArmorWareIcon,
    ArmorDeliverIcon,
    DisableDeliveryArmorIcon,
    SkinnerJobIcon,
    TannerJobIcon,
    LeatherworkerJobIcon,
    SkinsWareOnGroundOfFlag,
    DonkeyBoatCarryingSkinsWare,
    LeatherWareOnGroundOfFlag,
    DonkeyBoatCarryingLeatherWare,
    ArmorWareOnGroundOfFlag,
    DonkeyBoatCarryingArmorWare,
    ArmorIconAboveArmoredSoldier,
    StopCoinsXSignOverride,
    StopArmorXSign,
};

constexpr auto maxEnumValue(BobType)
{
    return BobType::StopArmorXSign;
}

extern helpers::EnumArray<unsigned, BobType> bobIndex;

ITexture* GetWareTex(GoodType good);
ITexture* GetWareStackTex(GoodType good);
ITexture* GetWareDonkeyTex(GoodType good);
ITexture* GetJobTex(Job job);
bool isAddonActive(const GameWorldBase& gwb);
BobType wareToCarrierBobIndex(GoodType good, bool fat);

} // namespace leatheraddon
