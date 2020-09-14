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
#include "Rect.h"
#include "glArchivItem_BitmapBase.h"
#include "libsiedler2/ArchivItem_Bitmap_Player.h"
#include "s25util/colors.h"

/// Klasse für GL-Player-Bitmaps.
class glArchivItem_Bitmap_Player : public libsiedler2::ArchivItem_Bitmap_Player, public glArchivItem_BitmapBase
{
public:
    glArchivItem_Bitmap_Player() = default;
    glArchivItem_Bitmap_Player(const glArchivItem_Bitmap_Player& item)
        : ArchivItem_BitmapBase(item), ArchivItem_Bitmap_Player(item), glArchivItem_BitmapBase(item)
    {}
    RTTR_CLONEABLE(glArchivItem_Bitmap_Player)

    /// Draw the texture in the given rect, stretching if required
    /// equivalent to Draw(destArea, {0, 0, 0, 0}, color)
    void DrawFull(const Rect& destArea, unsigned color = COLOR_WHITE, unsigned player_color = COLOR_WHITE);
    /// Draw the texture to the given position with full size
    /// equivalent to Draw({dst, 0, 0}, {0, 0, 0, 0}, color, player_color)
    void DrawFull(const DrawPoint& dst, unsigned color = COLOR_WHITE, unsigned player_color = COLOR_WHITE);
    /// Draw in player colors
    void drawForPlayer(const DrawPoint& dst, unsigned playerColor);

protected:
    void Draw(Rect dstArea, Rect srcArea, unsigned color = COLOR_WHITE, unsigned player_color = COLOR_WHITE);
    void FillTexture() override;
    Extent CalcTextureSize() const override;
};
