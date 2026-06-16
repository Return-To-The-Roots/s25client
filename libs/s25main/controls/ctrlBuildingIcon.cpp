// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "ctrlBuildingIcon.h"
#include "Loader.h"
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
void ctrlBuildingIcon::Draw_()
{
    if(state == ButtonState::Hover || state == ButtonState::Pressed)
        LOADER.GetImageN("io", 0)->DrawPart(GetDrawRect());
    glArchivItem_Bitmap* image = LOADER.GetNationIcon(nation, type);
    if(image)
        image->DrawFull(GetDrawPos() + GetSize() / 2, (state == ButtonState::Pressed ? COLOR_YELLOW : COLOR_WHITE));
}

void ctrlBuildingIcon::DrawContent() const {}
