// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "glArchivItem_Bob.h"
#include "glArchivItem_Bitmap_Player.h"
#include "s25util/colors.h"

/**
 *  Zeichnet einen Animationsstep.
 */
void glArchivItem_Bob::Draw(unsigned item, libsiedler2::ImgDir direction, bool fat, unsigned animationstep,
                            DrawPoint drawPt, unsigned color)
{
    auto* body = dynamic_cast<glArchivItem_Bitmap_Player*>(getBody(fat, direction, animationstep));
    if(body)
        body->DrawFull(drawPt, COLOR_WHITE, color);
    auto* overlay = dynamic_cast<glArchivItem_Bitmap_Player*>(getOverlay(item, fat, direction, animationstep));
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
