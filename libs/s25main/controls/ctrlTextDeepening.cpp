// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "ctrlTextDeepening.h"
#include "ogl/FontStyle.h"
#include "ogl/glFont.h"

ctrlTextDeepening::ctrlTextDeepening(Window* parent, unsigned id, DrawPoint pos, const Extent& size, TextureColor tc,
                                     const std::string& text, const glFont* font, unsigned color, FontStyle style)
    : ctrlDeepening(parent, id, pos, size, tc), ctrlBaseText(text, color, font), style_(style)
{}

Rect ctrlTextDeepening::GetBoundaryRect() const
{
    const Rect txtRect = font->getBounds(CalcTextPos(), text, style_);
    const Rect parentRect = ctrlDeepening::GetBoundaryRect();
    Rect result;
    result.left = std::min(txtRect.left, parentRect.left);
    result.top = std::min(txtRect.top, parentRect.top);
    result.right = std::max(txtRect.right, parentRect.right);
    result.bottom = std::max(txtRect.bottom, parentRect.bottom);
    return result;
}

DrawPoint ctrlTextDeepening::CalcTextPos() const
{
    DrawPoint pos = GetDrawPos();
    if(style_.is(FontStyle::CENTER))
        pos.x += GetSize().x / 2;
    else if(style_.is(FontStyle::LEFT))
        pos.x += ctrlTextDeepening::borderSize.x;
    else
        pos.x += GetSize().x - ctrlTextDeepening::borderSize.x;
    if(style_.is(FontStyle::VCENTER))
        pos.y += GetSize().y / 2;
    else if(style_.is(FontStyle::TOP))
        pos.y += ctrlTextDeepening::borderSize.y;
    else
        pos.y += GetSize().y;
    return pos;
}

void ctrlTextDeepening::ResizeForMaxChars(unsigned numChars)
{
    const auto maxTextWidth = font->getDx() * numChars;
    Resize(Extent(maxTextWidth + borderSize.x * 2, GetSize().y));
}

void ctrlTextDeepening::DrawContent() const
{
    font->Draw(CalcTextPos(), text, style_, color_, GetSize().x - 2 * ctrlTextDeepening::borderSize.x);
}
