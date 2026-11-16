// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "noSign.h"
#include "GlobalGameSettings.h"
#include "Loader.h"
#include "SerializedGameData.h"
#include "addons/AddonDurableGeologistSigns.h"
#include "addons/const_addons.h"
#include "ogl/glArchivItem_Bitmap.h"
#include "ogl/glArchivItem_Bitmap_Player.h"
#include "world/GameWorld.h"
#include <algorithm>

noSign::noSign(const MapPoint pos, Resource resource)
    : noDisappearingEnvObject(
      pos, 8500 * (signDurabilityFactor[world->getGGS().getSelection(AddonId::DURABLE_GEOLOGIST_SIGNS)]), 500),
      resource(resource)
{
    // As this is only for drawing we set the type to nothing if the resource is depleted
    if(resource.getAmount() == 0u)
        this->resource.setType(ResourceType::Nothing);
}

void noSign::serialize(SerializedGameData& sgd) const
{
    noDisappearingEnvObject::serialize(sgd);

    sgd.PushUnsignedChar(resource.getValue());
}

noSign::noSign(SerializedGameData& sgd, const unsigned obj_id)
    : noDisappearingEnvObject(sgd, obj_id), resource(sgd.PopUnsignedChar())
{}

void noSign::draw(DrawPoint drawPt)
{
    // Schild selbst
    unsigned imgId;
    switch(resource.getType())
    {
        case ResourceType::Iron: imgId = 680; break;
        case ResourceType::Gold: imgId = 683; break;
        case ResourceType::Coal: imgId = 686; break;
        case ResourceType::Granite: imgId = 689; break;
        case ResourceType::Water: imgId = 692; break;
        case ResourceType::Nothing: imgId = 695; break;
        default: return;
    }
    imgId += std::min(resource.getAmount() / 3u, 2u);
    LOADER.getMapTexture(imgId)->drawFull(drawPt, getDrawColor());

    // Schatten des Schildes
    LOADER.getMapTexture(700)->drawFull(drawPt, getDrawShadowColor());
}
