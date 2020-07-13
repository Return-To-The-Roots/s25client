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

#include "ctrlText.h"

#include "ogl/glFont.h"
#include <utility>

ctrlBaseText::ctrlBaseText(std::string text, const unsigned color, const glFont* font) : text(std::move(text)), color_(color), font(font) {}

void ctrlBaseText::SetText(const std::string& text)
{
    this->text = text;
}

void ctrlBaseText::SetFont(glFont* font)
{
    this->font = font;
}

ctrlText::ctrlText(Window* parent, unsigned id, const DrawPoint& pos, const std::string& text, unsigned color, FontStyle format,
                   const glFont* font)
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
