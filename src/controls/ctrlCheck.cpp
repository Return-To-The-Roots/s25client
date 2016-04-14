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

///////////////////////////////////////////////////////////////////////////////
// Header
#include "defines.h" // IWYU pragma: keep
#include "ctrlCheck.h"
#include "CollisionDetection.h"
#include "Loader.h"
#include "driver/src/MouseCoords.h"
#include "ogl/glArchivItem_Bitmap.h"
#include "ogl/glArchivItem_Font.h"

// Include last!
#include "DebugNew.h" // IWYU pragma: keep

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
    : Window(x, y, id, parent, width, height),
      tc(tc), text(text), font(font), check(false), readonly(readonly)
{
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  der Messagehandler.
 *
 *  @param[in] msg   Die Nachricht.
 *  @param[in] id    Die ID des Quellsteuerelements.
 *  @param[in] param Ein nachrichtenspezifischer Parameter.
 *
 *  @author OLiver
 */

bool ctrlCheck::Msg_LeftDown(const MouseCoords& mc)
{
    if(!readonly && Coll(mc.x, mc.y, GetX(), GetY(), width_, height_))
    {
        check = !check;
        parent_->Msg_CheckboxChange(GetID(), check);
        return true;
    }

    return false;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  zeichnet das Fenster.
 *
 *  @author OLiver
 */
bool ctrlCheck::Draw_()
{
    const unsigned short boxSize = 20;
    short spacing = (height_ - boxSize) / 2;
    if(spacing < 0)
        spacing = 0;
    unsigned short xPos = GetX();
    const unsigned short yPos = GetY();
    unsigned short curWidth = width_;
    const bool drawText = font && !text.empty();
    if(!drawText)
    {
        // If we draw only the check mark, draw surrounding box smaller and center checkbox
        curWidth = boxSize + 2 * spacing;
        xPos += (width_ - curWidth)  / 2;
    }
    short boxStartOffsetX = curWidth - spacing - boxSize;
    if(boxStartOffsetX < 0)
        boxStartOffsetX = 0;

    Draw3D(xPos, yPos, curWidth, height_, tc, 2);

    if(drawText)
    {
        int availableWidth = boxStartOffsetX - 4;
        if(availableWidth < 0)
            availableWidth = 0;
        font->Draw(xPos + 4, yPos + height_ / 2, text, glArchivItem_Font::DF_VCENTER, (check ? COLOR_YELLOW : 0xFFBBBBBB), 0, availableWidth);
    }

    if(!readonly)
        Draw3D(xPos + boxStartOffsetX, yPos + spacing, boxSize, boxSize, tc, 2);

    if(check)
        LOADER.GetImageN("io", 32)->Draw(xPos + boxStartOffsetX + boxSize / 2, yPos + spacing + boxSize / 2, 0, 0, 0, 0, 0, 0);

    return true;
}
