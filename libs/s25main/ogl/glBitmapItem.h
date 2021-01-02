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

#include "Point.h"

namespace libsiedler2 {
class baseArchivItem_Bitmap;
class ArchivItem_Bitmap_Player;
class ArchivItem_BitmapBase;
} // namespace libsiedler2

enum class glBitmapItemType
{
    Normal,
    PlayerBitmap,
    ShadowBitmap
};

class glBitmapItem
{
public:
    glBitmapItem(libsiedler2::baseArchivItem_Bitmap* b, bool shadow = false, bool isOwning = false);
    glBitmapItem(libsiedler2::ArchivItem_Bitmap_Player* b, bool isOwning = false);

    libsiedler2::ArchivItem_BitmapBase* bmp;
    glBitmapItemType type;
    /// If this is true, the owner of the bitmap item should also delete the bitmap
    bool isOwning_;

    /// Start of the data in the original bitmap (skipping transparent pixels)
    Position pos;
    /// Size of the non-transparent data
    Extent size;
    /// Adjusted origin
    Position origin;
};
