// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "Addon.h"
#include "Loader.h"
#include "Window.h"
#include "s25util/colors.h"

AddonGui::AddonGui(const Addon& addon, Window& window, bool /*readonly*/)
{
    DrawPoint btPos(20, 0), txtPos(52, 4);
    window.AddText(0, txtPos, addon.getName(), COLOR_YELLOW, FontStyle{}, NormalFont);
    window.AddImageButton(1, btPos, Extent(22, 22), TextureColor::Grey, LOADER.GetImageN("io", 21),
                          addon.getDescription());
}
