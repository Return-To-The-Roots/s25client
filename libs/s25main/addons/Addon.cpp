// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "Addon.h"
#include "Loader.h"
#include "Window.h"
#include "s25util/colors.h"

AddonGui::AddonGui(const Addon& addon, Window& window, bool /*readonly*/) : addon_(addon), window_(window)
{
    DrawPoint btPos(20, 0), txtPos(52, 4);
    window.addText(0, txtPos, addon.getName(), COLOR_YELLOW, FontStyle{}, NormalFont);
    window.addImageButton(1, btPos, Extent(22, 22), TextureColor::Grey, LOADER.getImageN("io", 21),
                          addon.getDescription());
}
