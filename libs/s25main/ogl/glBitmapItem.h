// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

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
