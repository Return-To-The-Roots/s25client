// $Id: ctrlCheck.cpp 9357 2014-04-25 15:35:25Z FloSoft $
//
// Copyright (c) 2005 - 2011 Settlers Freaks (sf-team at siedler25.org)
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
#include "main.h"
#include "ctrlCheck.h"
#include "Loader.h"

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#if defined _WIN32 && defined _DEBUG && defined _MSC_VER
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

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
    if(!readonly && Coll(mc.x, mc.y, GetX(), GetY(), width, height))
    {
        check = !check;
        parent->Msg_CheckboxChange(GetID(), check);
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
    unsigned short distance = (height - box_size) / 2;

    Draw3D(GetX(), GetY(), width, height, tc, 2);

    if(font)
        font->Draw(GetX() + 4, GetY() + height / 2, text, glArchivItem_Font::DF_VCENTER, (check ? COLOR_YELLOW : 0xFFBBBBBB) );

    Draw3D(GetX() + width - distance - box_size, GetY() + distance, box_size, box_size, tc, 2);

    if(check)
        LOADER.GetImageN("io", 32)->Draw(GetX() + width - distance - box_size / 2, GetY() + distance + box_size / 2, 0, 0, 0, 0, 0, 0);

    return true;
}
