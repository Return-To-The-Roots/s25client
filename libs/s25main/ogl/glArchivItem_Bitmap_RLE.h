// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

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
