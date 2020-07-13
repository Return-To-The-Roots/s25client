// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
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

#include "ctrlCheck.h"

#include "CollisionDetection.h"
#include "Loader.h"
#include "driver/MouseCoords.h"
#include "ogl/FontStyle.h"
#include "ogl/glArchivItem_Bitmap.h"
#include "ogl/glFont.h"
#include <utility>

ctrlCheck::ctrlCheck(Window* parent, unsigned id, const DrawPoint& pos, const Extent& size, TextureColor tc, std::string text,
                     const glFont* font, bool readonly)
    : Window(parent, id, pos, size), tc(tc), text(std::move(text)), font(font), check(false), readonly(readonly)
{}

/**
 *  der Messagehandler.
 *
 *  @param[in] msg   Die Nachricht.
 *  @param[in] id    Die ID des Quellsteuerelements.
 *  @param[in] param Ein nachrichtenspezifischer Parameter.
 */

bool ctrlCheck::Msg_LeftDown(const MouseCoords& mc)
{
    if(!readonly && IsPointInRect(mc.GetPos(), GetDrawRect()))
    {
        check = !check;
        GetParent()->Msg_CheckboxChange(GetID(), check);
        return true;
    }

    return false;
}

/**
 *  zeichnet das Fenster.
 */
void ctrlCheck::Draw_()
{
    const unsigned short boxSize = 20;
    short spacing = (GetSize().y - boxSize) / 2;
    if(spacing < 0)
        spacing = 0;
    Rect drawRect = GetDrawRect();
    const bool drawText = font && !text.empty();
    if(!drawText)
    {
        // If we draw only the check mark, draw surrounding box smaller and center checkbox
        drawRect.setSize(Extent(boxSize + 2 * spacing, drawRect.getSize().y));
        drawRect.move(DrawPoint((GetSize().x - drawRect.getSize().x) / 2, 0));
    }
    short boxStartOffsetX = drawRect.getSize().x - spacing - boxSize;
    if(boxStartOffsetX < 0)
        boxStartOffsetX = 0;

    Draw3D(drawRect, tc, false);

    if(drawText)
    {
        int availableWidth = boxStartOffsetX - 4;
        if(availableWidth < 0)
            availableWidth = 0;
        font->Draw(drawRect.getOrigin() + DrawPoint(4, GetSize().y / 2), text, FontStyle::VCENTER, (check ? COLOR_YELLOW : 0xFFBBBBBB),
                   availableWidth);
    }

    DrawPoint boxPos = drawRect.getOrigin() + DrawPoint(boxStartOffsetX, spacing);

    if(!readonly)
        Draw3D(Rect(boxPos, Extent::all(boxSize)), tc, false);

    if(check)
        LOADER.GetImageN("io", 32)->DrawFull(boxPos + DrawPoint(boxSize, boxSize) / 2);
}
