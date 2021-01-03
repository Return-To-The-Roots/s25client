// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// This file is part of Return To The Roots.
//
// Return To The Roots is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// Return To The Roots is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Return To The Roots. If not, see <http://www.gnu.org/licenses/>.

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
