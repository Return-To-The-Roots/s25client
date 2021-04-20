// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

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
