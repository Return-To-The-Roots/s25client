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
#include "ctrlDeepening.h"
#include "Loader.h"
#include "ogl/glArchivItem_Font.h"
class Window;

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
    // of only-text objects down to the Window class.
    this->width_ = width;
    this->height_ = height;
}

/**
 *  zeichnet das Fenster.
 */
bool ctrlDeepening::Draw_()
{
    Draw3D(GetDrawPos(), width_, height_, tc, 2);

    font->Draw(GetDrawPos() + DrawPoint(width_, height_) / 2, text, glArchivItem_Font::DF_CENTER | glArchivItem_Font::DF_VCENTER, color_);

    DrawContent();

    return true;
}

ctrlColorDeepening::ctrlColorDeepening(Window* parent,
                                       unsigned int id,
                                       unsigned short x,
                                       unsigned short y,
                                       unsigned short width,
                                       unsigned short height,
                                       TextureColor tc,
                                       unsigned int fillColor)
    : ctrlDeepening(parent, id, x, y, width, height, tc, "", NormalFont, COLOR_YELLOW),
      fillColor(fillColor)
{
}

/// Setzt die Farbe des Controls
void ctrlColorDeepening::SetColor(const unsigned int fill_color)
{
    this->fillColor = fill_color;
}

/**
 *  zeichnet das Fenster.
 */
void ctrlColorDeepening::DrawContent() const
{
    DrawRectangle(GetDrawPos() + DrawPoint(3, 3), width_ - 6, height_ - 6, fillColor);
}
