// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "ctrlVarDeepening.h"
#include "ogl/FontStyle.h"
#include "ogl/glFont.h"

ctrlVarDeepening::ctrlVarDeepening(Window* parent, unsigned id, const DrawPoint& pos, const Extent& size,
                                   TextureColor tc, const std::string& fmtString, const glFont* font, unsigned color,
                                   unsigned count, va_list fmtArgs)
    : ctrlDeepening(parent, id, pos, size, tc), ctrlBaseVarText(fmtString, color, font, count, fmtArgs)
{}

void ctrlVarDeepening::drawContent() const
{
    font->draw(getDrawPos() + getSize() / 2, getFormatedText(), FontStyle::CENTER | FontStyle::VCENTER, color_);
}
