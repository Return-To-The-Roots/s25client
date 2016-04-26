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

#include "defines.h" // IWYU pragma: keep
#include "glBitmapItem.h"

#include "libsiedler2/src/ArchivItem_Bitmap.h"
#include "libsiedler2/src/ArchivItem_Bitmap_Player.h"

glBitmapItem::glBitmapItem(libsiedler2::baseArchivItem_Bitmap* b, bool shadow, bool isOwning): bmp(b), type(shadow ? TYPE_ARCHIVITEM_BITMAP_SHADOW : TYPE_ARCHIVITEM_BITMAP), isOwning_(isOwning)
{
    b->getVisibleArea(x, y, w, h);
    nx = b->getNx() - x;
    ny = b->getNy() - y;
}
glBitmapItem::glBitmapItem(libsiedler2::ArchivItem_Bitmap_Player* b, bool isOwning): bmp(b), type(TYPE_ARCHIVITEM_BITMAP_PLAYER), isOwning_(isOwning)
{
    b->getVisibleArea(x, y, w, h);
    nx = b->getNx() - x;
    ny = b->getNy() - y;
}
