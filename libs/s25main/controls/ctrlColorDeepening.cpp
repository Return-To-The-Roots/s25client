// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "ctrlColorDeepening.h"

ctrlColorDeepening::ctrlColorDeepening(Window* parent, unsigned id, DrawPoint pos, const Extent& size, TextureColor tc,
                                       unsigned fillColor)
    : ctrlDeepening(parent, id, pos, size, tc), ctrlBaseColor(fillColor)
{}

void ctrlColorDeepening::DrawContent() const
{
    DrawRectangle(Rect(GetDrawPos() + DrawPoint(3, 3), GetSize() - Extent(6, 6)), color_);
}
