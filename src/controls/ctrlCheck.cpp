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
#include "defines.h"
#include "ctrlCheck.h"
#include "Loader.h"
#include "driver/src/MouseCoords.h"

// Include last!
#include "DebugNew.h"

///////////////////////////////////////////////////////////////////////////////
/**
 *  Konstruktor von @p ctrlCheck.
 *
 *  @author OLiver
 */
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
bool ctrlCheck::Draw_(void)
{
    const unsigned short box_size = 20;
    unsigned short distance = (height_ - box_size) / 2;

    Draw3D(GetX(), GetY(), width_, height_, tc, 2);

    if(font)
        font->Draw(GetX() + 4, GetY() + height_ / 2, text, glArchivItem_Font::DF_VCENTER, (check ? COLOR_YELLOW : 0xFFBBBBBB) );

    Draw3D(GetX() + width_ - distance - box_size, GetY() + distance, box_size, box_size, tc, 2);

    if(check)
        LOADER.GetImageN("io", 32)->Draw(GetX() + width_ - distance - box_size / 2, GetY() + distance + box_size / 2, 0, 0, 0, 0, 0, 0);

    return true;
}
