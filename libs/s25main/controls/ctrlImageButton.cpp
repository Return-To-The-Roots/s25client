// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "ctrlImageButton.h"

ctrlImageButton::ctrlImageButton(Window* parent, unsigned id, const DrawPoint& pos, const Extent& size,
                                 const TextureColor tc, ITexture* const image, const std::string& tooltip)
    : ctrlButton(parent, id, pos, size, tc, tooltip), ctrlBaseImage(image)
{}

void ctrlImageButton::DrawContent() const
{
    DrawPoint pos = GetDrawPos() + DrawPoint(GetSize()) / 2;
    if((state == ButtonState::Pressed || isChecked) && isEnabled)
        pos += DrawPoint::all(2);
    if(!isEnabled && GetModulationColor() == COLOR_WHITE)
        DrawImage(pos, 0xFF555555);
    else
        DrawImage(pos);
}
