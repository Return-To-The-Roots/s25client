// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "ctrlText.h"

#include "ogl/glFont.h"
#include <utility>

ctrlBaseText::ctrlBaseText(std::string text, const unsigned color, const glFont* font)
    : text(std::move(text)), color_(color), font(font)
{}

void ctrlBaseText::SetText(const std::string& text)
{
    this->text = text;
}

void ctrlBaseText::SetFont(glFont* font)
{
    this->font = font;
}

ctrlText::ctrlText(Window* parent, unsigned id, const DrawPoint& pos, const std::string& text, unsigned color,
                   FontStyle format, const glFont* font)
    : Window(parent, id, pos), ctrlBaseText(text, color, font), format(format)
{}

Rect ctrlText::GetBoundaryRect() const
{
    if(text.empty())
        return Rect(GetDrawPos(), 0, 0);
    else
        return font->getBounds(GetDrawPos(), text, format);
}

/**
 *  zeichnet das Fenster.
 */
void ctrlText::Draw_()
{
    if(!text.empty())
        font->Draw(GetDrawPos(), text, format, color_);
}
