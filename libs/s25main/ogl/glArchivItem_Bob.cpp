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

#include "glArchivItem_Bob.h"
#include "glArchivItem_Bitmap_Player.h"
#include "s25util/colors.h"

/**
 *  Zeichnet einen Animationsstep.
 */
void glArchivItem_Bob::Draw(unsigned item, libsiedler2::ImgDir direction, bool fat, unsigned animationstep, DrawPoint drawPt,
                            unsigned color)
{
    unsigned overlayIdx = getOverlayIdx(item, fat, direction, animationstep);
    if(overlayIdx == 188 && fat)
    {
        // No fat version(?)
        overlayIdx = getOverlayIdx(item, false, direction, animationstep);
        fat = false;
    }

    auto* body = dynamic_cast<glArchivItem_Bitmap_Player*>(getBody(fat, direction, animationstep));
    if(body)
        body->DrawFull(drawPt, COLOR_WHITE, color);
    auto* overlay = dynamic_cast<glArchivItem_Bitmap_Player*>(get(overlayIdx));
    if(overlay)
        overlay->DrawFull(drawPt, COLOR_WHITE, color);
}

void glArchivItem_Bob::mergeLinks(const std::map<uint16_t, uint16_t>& overrideLinks)
{
    if(overrideLinks.empty())
        return;
    // Get last key of sorted map
    const auto maxIdx = overrideLinks.rbegin()->first;
    if(maxIdx >= links.size())
        links.resize(maxIdx + 1u);
    for(const auto& newLink : overrideLinks)
    {
        links[newLink.first] = newLink.second;
    }
}
