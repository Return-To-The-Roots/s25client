// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "ctrlImageButton.h"
#include <ogl/ITexture.h>

ctrlImageButton::ctrlImageButton(Window* parent, unsigned id, const DrawPoint& pos, const Extent& size,
                                 const TextureColor tc, ITexture* const image, const std::string& tooltip)
    : ctrlButton(parent, id, pos, size, tc, tooltip), ctrlBaseImage(image)
{}

void ctrlImageButton::DrawContent() const
{
    // Adding of origin compensates for its substraction inside ITexture::Draw()
    auto pos = GetDrawPos() + GetImage()->GetOrigin();
    auto size = GetSize();

    if(hasBorder)
    {
        // Ensure that 3D border is not drawn on
        const unsigned borderThickness = 2;
        pos += DrawPoint::all(borderThickness);
        size -= Extent::all(2 * borderThickness);
    }

    if((state == ButtonState::Pressed || isChecked) && isEnabled)
    {
        pos += DrawPoint::all(2);
        size -= Extent::all(2);
    }

    Rect drawRect(pos, size);
    if(!isEnabled && GetModulationColor() == COLOR_WHITE)
        DrawImage(drawRect, 0xFF555555);
    else
        DrawImage(drawRect);
}
