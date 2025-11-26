// Copyright (C) 2005 - 2025 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "ctrlImageDeepening.h"
#include <ogl/ITexture.h>

ctrlImageDeepening::ctrlImageDeepening(Window* parent, unsigned id, DrawPoint pos, const Extent& size, TextureColor tc,
                                       ITexture* image)
    : ctrlDeepening(parent, id, pos, size, tc), ctrlBaseImage(image)
{}

void ctrlImageDeepening::DrawContent() const
{
    // Adding of origin compensates for its substraction inside ITexture::Draw()
    DrawImage(Rect(GetDrawPos() + GetImage()->GetOrigin(), GetSize()));
}
