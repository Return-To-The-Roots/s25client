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

#include "defines.h" // IWYU pragma: keep
#include "ctrlCheck.h"
#include "CollisionDetection.h"
#include "Loader.h"
#include "driver/src/MouseCoords.h"
#include "ogl/glArchivItem_Bitmap.h"
#include "ogl/glArchivItem_Font.h"

ctrlCheck::ctrlCheck(Window* parent,
                     unsigned int id,
                     unsigned short x,
                     unsigned short y,
                     unsigned short width,
                     unsigned short height,
                     TextureColor tc,
                     const std::string& text,
                     glArchivItem_Font* font,
                     bool readonly)
    : Window(DrawPoint(x, y), id, parent, width, height),
      tc(tc), text(text), font(font), check(false), readonly(readonly)
{
}

/**
 *  der Messagehandler.
 *
 *  @param[in] msg   Die Nachricht.
 *  @param[in] id    Die ID des Quellsteuerelements.
 *  @param[in] param Ein nachrichtenspezifischer Parameter.
 */

bool ctrlCheck::Msg_LeftDown(const MouseCoords& mc)
{
    if(!readonly && IsPointInRect(mc.x, mc.y, GetX(), GetY(), width_, height_))
    {
        check = !check;
        parent_->Msg_CheckboxChange(GetID(), check);
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
    short spacing = (height_ - boxSize) / 2;
    if(spacing < 0)
        spacing = 0;
    DrawPoint drawPos = GetDrawPos();
    unsigned short curWidth = width_;
    const bool drawText = font && !text.empty();
    if(!drawText)
    {
        // If we draw only the check mark, draw surrounding box smaller and center checkbox
        curWidth = boxSize + 2 * spacing;
        drawPos.x += (width_ - curWidth)  / 2;
    }
    short boxStartOffsetX = curWidth - spacing - boxSize;
    if(boxStartOffsetX < 0)
        boxStartOffsetX = 0;

    Draw3D(drawPos, curWidth, height_, tc, 2);

    if(drawText)
    {
        int availableWidth = boxStartOffsetX - 4;
        if(availableWidth < 0)
            availableWidth = 0;
        font->Draw(drawPos + DrawPoint(4, height_ / 2), text, glArchivItem_Font::DF_VCENTER, (check ? COLOR_YELLOW : 0xFFBBBBBB), 0, availableWidth);
    }

    DrawPoint boxPos = drawPos + DrawPoint(boxStartOffsetX, spacing);

    if(!readonly)
        Draw3D(boxPos, boxSize, boxSize, tc, 2);

    if(check)
        LOADER.GetImageN("io", 32)->Draw(boxPos + DrawPoint(boxSize, boxSize) / 2);
}
