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

helpers::EnumArray<unsigned, BobType> bobIndex = {0,   21,  69,  117, 125, 173, 221, 244, 292, 340, 388,
                                                  436, 484, 532, 580, 628, 629, 630, 630, 631, 631, 632,
                                                  633, 634, 635, 636, 637, 638, 639, 640, 641, 642, 643};

ITexture* GetWareTex(const GoodType good)
{
    switch(good)
    {
        case GoodType::Skins: return LOADER.GetImageN("leather_bobs", bobIndex[BobType::SkinsWareIcon]);
        case GoodType::Leather: return LOADER.GetImageN("leather_bobs", bobIndex[BobType::LeatherWareIcon]);
        case GoodType::Armor: return LOADER.GetImageN("leather_bobs", bobIndex[BobType::ArmorWareIcon]);
        default: return nullptr;
    }
}

ITexture* GetWareStackTex(const GoodType good)
{
    switch(good)
    {
        case GoodType::Skins: return LOADER.GetImageN("leather_bobs", bobIndex[BobType::SkinsWareOnGroundOfFlag]);
        case GoodType::Leather: return LOADER.GetImageN("leather_bobs", bobIndex[BobType::LeatherWareOnGroundOfFlag]);
        case GoodType::Armor: return LOADER.GetImageN("leather_bobs", bobIndex[BobType::ArmorWareOnGroundOfFlag]);
        default: return nullptr;
    }
}

ITexture* GetWareDonkeyTex(const GoodType good)
{
    switch(good)
    {
        case GoodType::Skins: return LOADER.GetImageN("leather_bobs", bobIndex[BobType::DonkeyBoatCarryingSkinsWare]);
        case GoodType::Leather:
            return LOADER.GetImageN("leather_bobs", bobIndex[BobType::DonkeyBoatCarryingLeatherWare]);
        case GoodType::Armor: return LOADER.GetImageN("leather_bobs", bobIndex[BobType::DonkeyBoatCarryingArmorWare]);
        default: return nullptr;
    }
}

ITexture* GetJobTex(Job job)
{
    switch(job)
    {
        case Job::Skinner: return LOADER.GetImageN("leather_bobs", bobIndex[BobType::SkinnerJobIcon]);
        case Job::Tanner: return LOADER.GetImageN("leather_bobs", bobIndex[BobType::TannerJobIcon]);
        case Job::LeatherWorker: return LOADER.GetImageN("leather_bobs", bobIndex[BobType::LeatherworkerJobIcon]);
        default: return nullptr;
    }
}

bool isAddonActive(const GameWorldBase& gwb)
{
    return gwb.GetGGS().isEnabled(AddonId::LEATHER);
}

BobType wareToCarrierBobIndex(const GoodType good, const bool fat)
{
    if(good == GoodType::Skins)
        return fat ? BobType::FatCarrierCarryingSkins : BobType::ThinCarrierCarryingSkins;
    else if(good == GoodType::Leather)
        return fat ? BobType::FatCarrierCarryingLeather : BobType::ThinCarrierCarryingLeather;
    else if(good == GoodType::Armor)
        return fat ? BobType::FatCarrierCarryingArmor : BobType::ThinCarrierCarryingArmor;
    else
        throw std::runtime_error("Unsupported good type");
}

} // namespace leatheraddon