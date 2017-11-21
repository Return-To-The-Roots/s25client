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

#include "rttrDefines.h" // IWYU pragma: keep
#include "ctrlTextButton.h"
#include "ogl/FontStyle.h"
#include "ogl/glArchivItem_Bitmap.h"
#include "ogl/glArchivItem_Font.h"

ctrlTextButton::ctrlTextButton(Window* parent, unsigned id, const DrawPoint& pos, const Extent& size, const TextureColor tc,
                               const std::string& text, glArchivItem_Font* font, const std::string& tooltip)
    : ctrlButton(parent, id, pos, size, tc, tooltip), ctrlBaseText(text, COLOR_YELLOW, font)
{}

void ctrlTextButton::DrawContent() const
{
    const bool isPressed = state == BUTTON_PRESSED || isChecked;
    unsigned color;
    if(this->color_ == COLOR_YELLOW && isPressed)
        color = 0xFFFFAA00;
    else if(!isEnabled)
        color = COLOR_GREY;
    else
        color = this->color_;

    const unsigned short maxTextWidth = GetSize().x - 4; // reduced by border

    if(GetTooltip().empty() && state == BUTTON_HOVER)
    {
        unsigned maxNumChars;
        font->getWidth(text, 0, maxTextWidth, &maxNumChars);
        if(maxNumChars < text.length())
            ShowTooltip(text);
    }

    const unsigned short offset = isPressed ? 2 : 0;
    font->Draw(GetDrawPos() + DrawPoint(GetSize()) / 2 + DrawPoint(offset, offset), text, FontStyle::CENTER | FontStyle::VCENTER, color, 0,
               maxTextWidth);
}
