// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "glArchivItem_Bitmap.h"
#include "libsiedler2/ArchivItem_Bitmap_Raw.h"

/// Klasse für GL-RAW-Bitmaps.
class glArchivItem_Bitmap_Raw : public libsiedler2::baseArchivItem_Bitmap_Raw, public glArchivItem_Bitmap
{
public:
    glArchivItem_Bitmap_Raw() = default;

    glArchivItem_Bitmap_Raw(const glArchivItem_Bitmap_Raw& item)
        : ArchivItem_BitmapBase(item), baseArchivItem_Bitmap(item), baseArchivItem_Bitmap_Raw(item),
          glArchivItem_Bitmap(item)
    {}
    RTTR_CLONEABLE(glArchivItem_Bitmap_Raw)
};
