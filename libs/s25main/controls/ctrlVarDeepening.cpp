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

#include "ctrlVarDeepening.h"
#include "ogl/FontStyle.h"
#include "ogl/glFont.h"

ctrlVarDeepening::ctrlVarDeepening(Window* parent, unsigned id, const DrawPoint& pos, const Extent& size,
                                   TextureColor tc, const std::string& fmtString, const glFont* font, unsigned color,
                                   unsigned count, va_list fmtArgs)
    : ctrlDeepening(parent, id, pos, size, tc), ctrlBaseVarText(fmtString, color, font, count, fmtArgs)
{}

void ctrlVarDeepening::DrawContent() const
{
    font->Draw(GetDrawPos() + GetSize() / 2, GetFormatedText(), FontStyle::CENTER | FontStyle::VCENTER, color_);
}
