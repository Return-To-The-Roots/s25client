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
#include "ctrlVarDeepening.h"
#include "ogl/glArchivItem_Font.h"


// Include last!
#include "DebugNew.h" // IWYU pragma: keep
class Window;

ctrlVarDeepening::ctrlVarDeepening(Window* parent,
                                   unsigned int id,
                                   unsigned short x,
                                   unsigned short y,
                                   unsigned short width,
                                   unsigned short height,
                                   TextureColor tc,
                                   const std::string& text,
                                   glArchivItem_Font* font,
                                   unsigned int color,
                                   unsigned int count,
                                   va_list liste)
    : ctrlVarText(parent, id, x, y, text, color, 0, font, count, liste),
      tc(tc)
{
    // We don't want to pass these through all those constructors
    // of only-text objects down to the Window class. This is a special
    // situation, as we are a Deepening _and_ a VarText instead
    // of owning the VarText.
    this->width_  = width;
    this->height_ = height;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  zeichnet das Fenster.
 *
 *  @author FloSoft
 */
bool ctrlVarDeepening::Draw_()
{
    Draw3D(x_, y_, width_, height_, tc, 2);

    font->Draw(x_ + width_ / 2, y_ + height_ / 2, GetFormatedText(), glArchivItem_Font::DF_CENTER | glArchivItem_Font::DF_VCENTER, color_);

    return true;
}
