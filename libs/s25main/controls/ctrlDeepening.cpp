// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "ctrlDeepening.h"

ctrlDeepening::ctrlDeepening(Window* parent, unsigned id, DrawPoint pos, const Extent& size, TextureColor tc)
    : Window(parent, id, pos, size), tc(tc)
{}

/**
 *  zeichnet das Fenster.
 */
void ctrlDeepening::Draw_()
{
    Draw3D(Rect(GetDrawPos(), GetSize()), tc, false);
    DrawContent();
}
