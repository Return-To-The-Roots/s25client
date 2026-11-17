// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "ctrlTextButton.h"
#include "ogl/FontStyle.h"
#include "ogl/glFont.h"

/// Offset of text to origin of the button
static constexpr unsigned contentOffset = 2;

ctrlTextButton::ctrlTextButton(Window* parent, unsigned id, const DrawPoint& pos, const Extent& size,
                               const TextureColor tc, const std::string& text, const glFont* font,
                               const std::string& tooltip)
    : ctrlButton(parent, id, pos, size, tc, tooltip), ctrlBaseText(text, COLOR_YELLOW, font)
{}

void ctrlTextButton::resizeForMaxChars(unsigned numChars)
{
    const auto maxTextWidth = font->getDx() * numChars;
    resize(Extent(maxTextWidth + contentOffset * 2, getSize().y));
}

void ctrlTextButton::drawContent() const
{
    const bool isPressed = state == ButtonState::Pressed || isChecked;
    unsigned color;
    if(this->color_ == COLOR_YELLOW && isPressed)
        color = 0xFFFFAA00;
    else if(!isEnabled)
        color = 0xFF818993;
    else
        color = this->color_;

    const unsigned short maxTextWidth = getSize().x - contentOffset * 2; // reduced by border

    if(getTooltip().empty() && state == ButtonState::Hover)
    {
        unsigned maxNumChars;
        font->getWidth(text, maxTextWidth, &maxNumChars);
        if(maxNumChars < text.length())
            showTooltip(text);
    }

    const unsigned short offset = isPressed ? contentOffset : 0;
    font->draw(getDrawPos() + getSize() / 2u + DrawPoint(offset, offset), text, FontStyle::CENTER | FontStyle::VCENTER,
               color, maxTextWidth);
}
