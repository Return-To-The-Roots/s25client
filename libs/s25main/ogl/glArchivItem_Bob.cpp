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
void glArchivItem_Bob::Draw(unsigned item, unsigned direction, bool fat, unsigned animationstep, DrawPoint drawPt, unsigned color)
{
    // Correct dir to image dir
    direction = (direction + 3) % 6;
    // 8 Anim steps, 2 types (fat, not fat), 6 directions -->
    // Array [item][animStep][fat][direction]: [35][8][2][6]
    unsigned good = ((item * 8 + animationstep) * 2 + fat) * 6 + direction;
    // Array: [fat][direction][animStep]: [2][6][8] = 96 entries
    unsigned body = (fat * 6 + direction) * 8 + animationstep;
    if(links[good] == 92 && fat)
    {
        // No fat version(?)
        good -= 6;
        body -= 6 * 8; // 48
    }

    auto* koerper = dynamic_cast<glArchivItem_Bitmap_Player*>(get(body));
    if(koerper)
        koerper->DrawFull(drawPt, COLOR_WHITE, color);
    auto* ware = dynamic_cast<glArchivItem_Bitmap_Player*>(get(96 + links[good]));
    if(ware)
        ware->DrawFull(drawPt, COLOR_WHITE, color);
}

void glArchivItem_Bob::mergeLinks(const std::map<unsigned, uint16_t>& overrideLinks)
{
    if(overrideLinks.empty())
        return;
    const auto maxIdx = overrideLinks.rbegin()->first;
    if(maxIdx >= links.size())
        links.resize(maxIdx + 1u);
    for(const auto& newLink : overrideLinks)
    {
        links[newLink.first] = newLink.second;
    }
}
