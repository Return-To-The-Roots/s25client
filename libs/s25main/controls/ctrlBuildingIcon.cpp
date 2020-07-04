// Copyright (c) 2005 - 2020 Settlers Freaks (sf-team at siedler25.org)
//
// This file is part of Return To The Roots.
//
// Return To The Roots is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// Return To The Roots is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Return To The Roots. If not, see <http://www.gnu.org/licenses/>.

#include "ctrlBuildingIcon.h"
#include "Loader.h"
#include "files.h"
#include "ogl/glArchivItem_Bitmap.h"
#include "gameTypes/BuildingType.h"
#include <string>

ctrlBuildingIcon::ctrlBuildingIcon(Window* const parent, const unsigned id, const DrawPoint& pos, const BuildingType type,
                                   const Nation nation, const unsigned short size, const std::string& tooltip)
    : ctrlButton(parent, id, pos, Extent(size, size), TC_GREY, tooltip), type(type), nation(nation)
{}

/**
 *  zeichnet das Fenster.
 */
void ctrlBuildingIcon::Draw_()
{
    if(state == BUTTON_HOVER || state == BUTTON_PRESSED)
        LOADER.GetImageN("io", 0)->DrawPart(GetDrawRect());
    glArchivItem_Bitmap* image;
    if(type != BLD_CHARBURNER)
        image = LOADER.GetNationIcon(nation, type);
    else
        image = LOADER.GetImageN("charburner", nation * 8 + 8);
    if(image)
        image->DrawFull(GetDrawPos() + GetSize() / 2, (state == BUTTON_PRESSED ? COLOR_YELLOW : COLOR_WHITE));
}

void ctrlBuildingIcon::DrawContent() const {}
