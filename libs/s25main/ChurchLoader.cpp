// Copyright (C) 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "ChurchLoader.h"
#include "GlobalGameSettings.h"
#include "Loader.h"
#include "addons/const_addons.h"
#include "helpers/EnumArray.h"
#include "world/GameWorldBase.h"
#include "s25util/enumUtils.h"

namespace churchaddon {

unsigned getNationSpriteId(const Nation nation, const NationSprite sprite)
{
    return rttr::enumCast(nation) * NUM_SPRITES_PER_NATION + static_cast<unsigned>(sprite);
}

int getDoveOffsetY(const Nation nation)
{
    static constexpr helpers::EnumArray<int, Nation> DOVE_OFFSET_Y = {
      {-81, -98, -90, -110, -86},
    };
    return DOVE_OFFSET_Y[nation];
}

ITexture* getIconTex(const Nation nation)
{
    return LOADER.getTextureN("church", getNationSpriteId(nation, NationSprite::Icon));
}

glArchivItem_Bitmap* getIconBitmap(const Nation nation)
{
    return LOADER.getImageN("church", getNationSpriteId(nation, NationSprite::Icon));
}

ITexture* getDoveTex()
{
    return LOADER.getTextureN("church", static_cast<unsigned>(SharedSprite::Dove));
}

ITexture* getBlessingTex()
{
    return LOADER.getTextureN("church", static_cast<unsigned>(SharedSprite::BlessingIcon));
}

bool isAddonActive(const GlobalGameSettings& ggs)
{
    return ggs.isEnabled(AddonId::CHURCH) && ggs.isEnabled(AddonId::WINE);
}

bool isAddonActive(const GameWorldBase& gwb)
{
    return isAddonActive(gwb.getGGS());
}

} // namespace churchaddon
