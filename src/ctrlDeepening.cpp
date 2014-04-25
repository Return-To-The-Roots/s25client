// $Id: ctrlDeepening.cpp 9357 2014-04-25 15:35:25Z FloSoft $
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
#include "ctrlDeepening.h"
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
 *  Konstruktor von @p ctrlDeepening.
 *
 *  @author OLiver
 */
ctrlDeepening::ctrlDeepening(Window* parent,
                             unsigned int id,
                             unsigned short x,
                             unsigned short y,
                             unsigned short width,
                             unsigned short height,
                             TextureColor tc,
                             const std::string& text,
                             glArchivItem_Font* font,
                             unsigned int color)
    : ctrlText(parent, id, x, y, text, color, 0, font),
      tc(tc)
{
    // We don't want to pass these through all those constructors
    // of only-text objects down to the Window class. This is a special
    // situation, as we are a Deepening _and_ a VarText instead
    // of owning the VarText.
    this->width  = width;
    this->height = height;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  zeichnet das Fenster.
 *
 *  @author OLiver
 */
bool ctrlDeepening::Draw_(void)
{
    Draw3D(GetX(), GetY(), width, height, tc, 2);

    font->Draw(GetX() + width / 2, GetY() + height / 2, text.c_str(), glArchivItem_Font::DF_CENTER | glArchivItem_Font::DF_VCENTER, color);

    DrawContent();

    return true;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Konstruktor von @p ctrlColorDeepening.
 *
 *  @author Divan
 */
ctrlColorDeepening::ctrlColorDeepening(Window* parent,
                                       unsigned int id,
                                       unsigned short x,
                                       unsigned short y,
                                       unsigned short width,
                                       unsigned short height,
                                       TextureColor tc,
                                       unsigned int fillColor)
    : ctrlDeepening(parent, id, x, y, width, height, tc, "", NormalFont, COLOR_YELLOW),
      width(width),
      height(height),
      fillColor(fillColor)
{
}

/// Setzt die Farbe des Controls
void ctrlColorDeepening::SetColor(const unsigned int fill_color)
{
    this->fillColor = fill_color;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  zeichnet das Fenster.
 *
 *  @author Divan
 */
void ctrlColorDeepening::DrawContent(void) const
{
    DrawRectangle(x + 3, y + 3, width - 6, height - 6, fillColor);
}
