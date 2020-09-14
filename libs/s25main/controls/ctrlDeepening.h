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

#include "DrawPoint.h"
#include "Window.h"

/// Control with a "deepened" look by a 3D border
class ctrlDeepening : public Window
{
public:
    static constexpr DrawPoint borderSize{2, 2};

    ctrlDeepening(Window* parent, unsigned id, DrawPoint pos, const Extent& size, TextureColor tc);

protected:
    void Draw_() override;
    /// Derived classes have to draw the content
    virtual void DrawContent() const = 0;

private:
    TextureColor tc;
};
