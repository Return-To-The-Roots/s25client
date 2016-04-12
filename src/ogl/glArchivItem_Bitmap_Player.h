// Copyright (c) 2005 - 2015 Settlers Freaks (sf-team at siedler25.org)
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
#ifndef GLARCHIVITEM_BITMAP_PLAYER_H_INCLUDED
#define GLARCHIVITEM_BITMAP_PLAYER_H_INCLUDED

#pragma once

#include "../libsiedler2/src/ArchivItem_Bitmap_Player.h"
#include "glArchivItem_BitmapBase.h"
#include "libutil/src/colors.h"

/// Klasse für GL-Player-Bitmaps.
class glArchivItem_Bitmap_Player : public libsiedler2::ArchivItem_Bitmap_Player, public glArchivItem_BitmapBase
{
    public:
        glArchivItem_Bitmap_Player() {}

        glArchivItem_Bitmap_Player(const glArchivItem_Bitmap_Player& item) : ArchivItem_BitmapBase(item), ArchivItem_Bitmap_Player(item), glArchivItem_BitmapBase(item) {}

        void Draw(short dst_x, short dst_y, short dst_w = 0, short dst_h = 0, short src_x = 0, short src_y  = 0, short src_w  = 0, short src_h  = 0, const unsigned int color = COLOR_WHITE, const unsigned int player_color = COLOR_WHITE);

    protected:
        void FillTexture() override;
};

#endif // !GLARCHIVITEM_BITMAP_PLAYER_H_INCLUDED
