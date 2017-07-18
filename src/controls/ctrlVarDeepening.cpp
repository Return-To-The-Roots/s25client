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
#include "ctrlVarDeepening.h"
#include "ogl/glArchivItem_Font.h"

class Window;

ctrlVarDeepening::ctrlVarDeepening(Window* parent,
                                   unsigned int id,
                                   const DrawPoint& pos,
                                   const Extent& size,
                                   TextureColor tc,
                                   const std::string& text,
                                   glArchivItem_Font* font,
                                   unsigned int color,
                                   unsigned int count,
                                   va_list liste)
    : ctrlVarText(parent, id, pos, text, color, 0, font, count, liste),
      tc(tc)
{
    // We don't want to pass these through all those constructors
    // of only-text objects down to the Window class. This is a special
    // situation, as we are a Deepening _and_ a VarText instead
    // of owning the VarText.
    Resize(size);
}

/**
 *  zeichnet das Fenster.
 */
void ctrlVarDeepening::Draw_()
{
    Draw3D(Rect(GetDrawPos(), GetSize()), tc, 2);

    font->Draw(GetDrawPos() + DrawPoint(GetSize()) / 2, GetFormatedText(), glArchivItem_Font::DF_CENTER | glArchivItem_Font::DF_VCENTER, color_);
}
