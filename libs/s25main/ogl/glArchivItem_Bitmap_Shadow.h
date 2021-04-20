// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "glArchivItem_Bitmap.h"
#include "libsiedler2/ArchivItem_Bitmap_Shadow.h"

/// Klasse für GL-Shadow-Bitmaps.
class glArchivItem_Bitmap_Shadow : public libsiedler2::baseArchivItem_Bitmap_Shadow, public glArchivItem_Bitmap
{
public:
    glArchivItem_Bitmap_Shadow() = default;

    glArchivItem_Bitmap_Shadow(const glArchivItem_Bitmap_Shadow& item)
        : ArchivItem_BitmapBase(item), baseArchivItem_Bitmap(item), baseArchivItem_Bitmap_Shadow(item),
          glArchivItem_Bitmap(item)
    {}
    RTTR_CLONEABLE(glArchivItem_Bitmap_Shadow)
};
