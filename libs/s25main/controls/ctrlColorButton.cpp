// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "ctrlColorButton.h"

ctrlColorButton::ctrlColorButton(Window* parent, unsigned id, const DrawPoint& pos, const Extent& size,
                                 const TextureColor tc, unsigned fillColor, const std::string& tooltip)
    : ctrlButton(parent, id, pos, size, tc, tooltip), ctrlBaseColor(fillColor)
{}

void ctrlColorButton::DrawContent() const
{
    Extent rectSize = GetSize() - Extent(6, 6);
    DrawRectangle(Rect(GetDrawPos() + DrawPoint(3, 3), rectSize), color_);
}
