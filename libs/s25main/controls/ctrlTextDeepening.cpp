// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "ctrlTextDeepening.h"
#include "ogl/FontStyle.h"
#include "ogl/glFont.h"

ctrlTextDeepening::ctrlTextDeepening(Window* parent, unsigned id, DrawPoint pos, const Extent& size, TextureColor tc,
                                     const std::string& text, const glFont* font, unsigned color, FontStyle style)
    : ctrlDeepening(parent, id, pos, size, tc), ctrlBaseText(text, color, font), style_(style)
{}

Rect ctrlTextDeepening::getBoundaryRect() const
{
    const Rect txtRect = font->getBounds(calcTextPos(), text, style_);
    const Rect parentRect = ctrlDeepening::getBoundaryRect();
    Rect result;
    result.left = std::min(txtRect.left, parentRect.left);
    result.top = std::min(txtRect.top, parentRect.top);
    result.right = std::max(txtRect.right, parentRect.right);
    result.bottom = std::max(txtRect.bottom, parentRect.bottom);
    return result;
}

DrawPoint ctrlTextDeepening::calcTextPos() const
{
    DrawPoint pos = getDrawPos();
    if(style_.is(FontStyle::CENTER))
        pos.x += getSize().x / 2;
    else if(style_.is(FontStyle::LEFT))
        pos.x += ctrlTextDeepening::borderSize.x;
    else
        pos.x += getSize().x - ctrlTextDeepening::borderSize.x;
    if(style_.is(FontStyle::VCENTER))
        pos.y += getSize().y / 2;
    else if(style_.is(FontStyle::TOP))
        pos.y += ctrlTextDeepening::borderSize.y;
    else
        pos.y += getSize().y;
    return pos;
}

void ctrlTextDeepening::resizeForMaxChars(unsigned numChars)
{
    const auto maxTextWidth = font->getDx() * numChars;
    resize(Extent(maxTextWidth + borderSize.x * 2, getSize().y));
}

void ctrlTextDeepening::drawContent() const
{
    font->draw(calcTextPos(), text, style_, color_, getSize().x - 2 * ctrlTextDeepening::borderSize.x);
}
