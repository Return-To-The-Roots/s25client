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

#pragma once

#ifndef ctrlColorDeepening_h__
#define ctrlColorDeepening_h__

#include "controls/ctrlBaseColor.h"
#include "controls/ctrlDeepening.h"

/// Colored Deepening
class ctrlColorDeepening : public ctrlDeepening, public ctrlBaseColor
{
public:
    ctrlColorDeepening(Window* parent, unsigned id, DrawPoint position, const Extent& size, TextureColor tc, unsigned fillColor);

protected:
    void DrawContent() const override;
};

#endif // ctrlColorDeepening_h__
