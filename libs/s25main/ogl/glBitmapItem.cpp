// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "glBitmapItem.h"

#include "libsiedler2/ArchivItem_Bitmap.h"
#include "libsiedler2/ArchivItem_Bitmap_Player.h"

glBitmapItem::glBitmapItem(libsiedler2::baseArchivItem_Bitmap* b, bool shadow, bool isOwning)
    : bmp(b), type(shadow ? glBitmapItemType::ShadowBitmap : glBitmapItemType::Normal), isOwning_(isOwning)
{
    b->getVisibleArea(pos.x, pos.y, size.x, size.y);
    origin = Position(b->getNx(), b->getNy()) - pos;
}
glBitmapItem::glBitmapItem(libsiedler2::ArchivItem_Bitmap_Player* b, bool isOwning)
    : bmp(b), type(glBitmapItemType::PlayerBitmap), isOwning_(isOwning)
{
    b->getVisibleArea(pos.x, pos.y, size.x, size.y);
    origin = Position(b->getNx(), b->getNy()) - pos;
}
