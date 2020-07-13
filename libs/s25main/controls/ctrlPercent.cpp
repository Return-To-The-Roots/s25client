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

#include "ctrlPercent.h"
#include "helpers/toString.h"
#include "ogl/FontStyle.h"
#include "ogl/glFont.h"

ctrlPercent::ctrlPercent(Window* parent, unsigned id, const DrawPoint& pos, const Extent& size, TextureColor tc, unsigned text_color,
                         const glFont* font, const unsigned short* percentage)
    : Window(parent, id, pos, size), tc(tc), text_color(text_color), font(font), percentage_(percentage)
{}

/**
 *  Zeichenmethode.
 *
 *  @return @p true bei Erfolg, @p false bei Fehler
 */
void ctrlPercent::Draw_()
{
    // Wenn der Prozentsatzpointer = 0, dann wird 0 angezeigt und es soll nich abstürzen!
    unsigned short percentage = (this->percentage_ ? *this->percentage_ : 0);

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
    Draw3D(Rect(GetDrawPos(), GetSize()), tc, false);

    // Fortschritt zeichnen
    Extent progSize = GetSize() - Extent(8, 8);
    progSize.x = (progSize.x * percentage) / 100;
    DrawRectangle(Rect(GetDrawPos() + DrawPoint(4, 4), progSize), color);

    // Text zeichnen
    std::string caption = helpers::toString(percentage) + "%";
    font->Draw(GetDrawPos() + DrawPoint(GetSize()) / 2, caption, FontStyle::CENTER | FontStyle::VCENTER, text_color);
}
