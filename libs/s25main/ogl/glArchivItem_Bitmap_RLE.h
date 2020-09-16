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

#include "glArchivItem_Bitmap.h"
#include "libsiedler2/ArchivItem_Bitmap_RLE.h"

/// Klasse für GL-RLE-Bitmaps.
class glArchivItem_Bitmap_RLE : public libsiedler2::baseArchivItem_Bitmap_RLE, public glArchivItem_Bitmap
{
public:
    glArchivItem_Bitmap_RLE() = default;

    glArchivItem_Bitmap_RLE(const glArchivItem_Bitmap_RLE& item)
        : ArchivItem_BitmapBase(item), baseArchivItem_Bitmap(item), baseArchivItem_Bitmap_RLE(item),
          glArchivItem_Bitmap(item)
    {}
    RTTR_CLONEABLE(glArchivItem_Bitmap_RLE)
};
