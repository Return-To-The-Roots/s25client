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
#include "ctrlPercent.h"
#include "ogl/glArchivItem_Font.h"
#include <cstdio>

ctrlPercent::ctrlPercent(Window* parent,
                         unsigned int id,
                         unsigned short x,
                         unsigned short y,
                         unsigned short width,
                         unsigned short height,
                         TextureColor tc,
                         unsigned int text_color,
                         glArchivItem_Font* font,
                         const unsigned short* percentage)
    : Window(DrawPoint(x, y), id, parent, width, height),
      tc(tc), text_color(text_color), font(font), percentage_(percentage)
{
}

/**
 *  Zeichenmethode.
 *
 *  @return @p true bei Erfolg, @p false bei Fehler
 */
bool ctrlPercent::Draw_()
{
    // Wenn der Prozentsatzpointer = 0, dann wird 0 angezeigt und es soll nich abstürzen!
    unsigned short percentage = (this->percentage_ ?  *this->percentage_ : 0);

    if(percentage > 100)
        percentage = 100;

    // Farbe wählen je nachdem wie viel Prozent
    unsigned color;
    if(percentage >= 60)
        color = COLOR_60_PERCENT;
    else if(percentage >= 30)
        color = COLOR_30_PERCENT;
    else if(percentage >= 20)
        color = COLOR_20_PERCENT;
    else
        color = COLOR_0_PERCENT;

    // Box zeichnen
    Draw3D(GetDrawPos(), width_, height_, tc, 2);

    // Fortschritt zeichnen
    DrawRectangle(GetDrawPos() + DrawPoint(4, 4), (width_ - 8)*percentage / 100, height_ - 8, color);

    // Text zeichnen
    char caption[256];
    sprintf(caption, "%u%%", percentage);
    font->Draw(GetDrawPos() + DrawPoint(width_, height_) / 2, caption, glArchivItem_Font::DF_CENTER | glArchivItem_Font::DF_VCENTER, text_color);

    return true;
}
