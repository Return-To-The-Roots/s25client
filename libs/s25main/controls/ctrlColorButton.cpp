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

#include "ctrlColorButton.h"

ctrlColorButton::ctrlColorButton(Window* parent, unsigned id, const DrawPoint& pos, const Extent& size, const TextureColor tc,
                                 unsigned fillColor, const std::string& tooltip)
    : ctrlButton(parent, id, pos, size, tc, tooltip), ctrlBaseColor(fillColor)
{}

void ctrlColorButton::DrawContent() const
{
    Extent rectSize = GetSize() - Extent(6, 6);
    DrawRectangle(Rect(GetDrawPos() + DrawPoint(3, 3), rectSize), color_);
}
