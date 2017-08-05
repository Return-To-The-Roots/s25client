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

#include "defines.h" // IWYU pragma: keep
#include "ctrlTextDeepening.h"
#include "ogl/glArchivItem_Font.h"

ctrlTextDeepening::ctrlTextDeepening(Window* parent, unsigned id, DrawPoint pos, const Extent& size, TextureColor tc,
                                     const std::string& text, glArchivItem_Font* font, unsigned color)
    : ctrlDeepening(parent, id, pos, size, tc), ctrlBaseText(text, color, font)
{
}

Rect ctrlTextDeepening::GetBoundaryRect() const
{
    const Rect txtRect =
      font->getBounds(GetDrawPos() + DrawPoint(GetSize()) / 2, text, glArchivItem_Font::DF_CENTER | glArchivItem_Font::DF_VCENTER);
    const Rect parentRect = ctrlDeepening::GetBoundaryRect();
    Rect result;
    result.left = std::min(txtRect.left, parentRect.left);
    result.top = std::min(txtRect.top, parentRect.top);
    result.right = std::max(txtRect.right, parentRect.right);
    result.bottom = std::max(txtRect.bottom, parentRect.bottom);
    return result;
}

void ctrlTextDeepening::DrawContent() const
{
    font->Draw(GetDrawPos() + DrawPoint(GetSize()) / 2, text, glArchivItem_Font::DF_CENTER | glArchivItem_Font::DF_VCENTER, color_);
}
