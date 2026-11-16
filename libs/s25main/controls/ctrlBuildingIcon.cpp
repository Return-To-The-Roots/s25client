// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "ctrlBuildingIcon.h"
#include "Loader.h"
#include "driver/MouseCoords.h"
#include "files.h"
#include "ogl/glArchivItem_Bitmap.h"
#include "gameTypes/BuildingType.h"
#include <string>

ctrlBuildingIcon::ctrlBuildingIcon(Window* const parent, const unsigned id, const DrawPoint& pos,
                                   const BuildingType type, const Nation nation, const unsigned short size,
                                   const std::string& tooltip)
    : ctrlButton(parent, id, pos, Extent(size, size), TextureColor::Grey, tooltip), type(type), nation(nation)
{}

/**
 *  zeichnet das Fenster.
 */
void ctrlBuildingIcon::drawImpl()
{
    if(state == ButtonState::Hover || state == ButtonState::Pressed)
        LOADER.getImageN("io", 0)->drawPart(getDrawRect());
    glArchivItem_Bitmap* image = LOADER.getNationIcon(nation, type);
    if(image)
        image->drawFull(getDrawPos() + getSize() / 2, (state == ButtonState::Pressed ? COLOR_YELLOW : COLOR_WHITE));
}

void ctrlBuildingIcon::drawContent() const {}

bool ctrlBuildingIcon::msgMouseMove(const MouseCoords& mc)
{
    const bool wasHovered = (state == ButtonState::Hover);
    const bool result = ctrlButton::msgMouseMove(mc);
    const bool nowHovered = (state == ButtonState::Hover);
    if(wasHovered != nowHovered && onHoverChanged_)
        onHoverChanged_(nowHovered);
    return result;
}
