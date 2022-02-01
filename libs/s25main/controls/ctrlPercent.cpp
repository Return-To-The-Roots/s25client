// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "ctrlPercent.h"
#include "helpers/toString.h"
#include "ogl/FontStyle.h"
#include "ogl/glFont.h"

ctrlPercent::ctrlPercent(Window* parent, unsigned id, const DrawPoint& pos, const Extent& size, TextureColor tc,
                         unsigned text_color, const glFont* font, const unsigned short* percentage)
    : Window(parent, id, pos, size), tc(tc), text_color(text_color), font(font), percentage_(percentage)
{}

/**
 *  Zeichenmethode.
 *
 *  @return @p true bei Erfolg, @p false bei Fehler
 */
void ctrlPercent::Draw_()
{
    unsigned short percentage = getPercentage();

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
