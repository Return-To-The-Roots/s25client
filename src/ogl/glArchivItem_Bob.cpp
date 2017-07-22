// Copyright (c) 2005 - 2015 Settlers Freaks (sf-team at siedler25.org)
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

#include "defines.h" // IWYU pragma: keep
#include "glArchivItem_Bob.h"

#include "glArchivItem_Bitmap_Player.h"
#include "libutil/src/colors.h"

/**
 *  Zeichnet einen Animationsstep.
 */
void glArchivItem_Bob::Draw(unsigned item, unsigned direction, bool fat, unsigned animationstep, DrawPoint drawPt, unsigned color)
{
    unsigned good = item * 96 + animationstep * 12 + ( (direction + 3) % 6 ) + fat * 6;
    unsigned body = fat * 48 + ( (direction + 3) % 6 ) * 8 + animationstep;
    if(links[good] == 92)
    {
        good -= fat * 6;
        body -= fat * 48;
    }

    glArchivItem_Bitmap_Player* koerper = dynamic_cast<glArchivItem_Bitmap_Player*>(get(body));
    if(koerper)
        koerper->DrawFull(drawPt, COLOR_WHITE, color);
    glArchivItem_Bitmap_Player* ware = dynamic_cast<glArchivItem_Bitmap_Player*>(get(96 + links[good]));
    if(ware)
        ware->DrawFull(drawPt, COLOR_WHITE, color);
}
