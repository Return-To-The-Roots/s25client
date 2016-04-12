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
#ifndef GLARCHIVITEM_BITMAP_INCLUDED
#define GLARCHIVITEM_BITMAP_INCLUDED

#pragma once

#include "glArchivItem_BitmapBase.h"
#include "../libsiedler2/src/ArchivItem_Bitmap.h"
#include "libutil/src/colors.h"

/// Basisklasse für GL-Bitmapitems.
class glArchivItem_Bitmap : public virtual libsiedler2::baseArchivItem_Bitmap, public glArchivItem_BitmapBase
{
    public:
        glArchivItem_Bitmap();
        glArchivItem_Bitmap(const glArchivItem_Bitmap& item);

        /// Erzeugt und zeichnet die Textur.
        void Draw(short dst_x, short dst_y, short dst_w = 0, short dst_h = 0, short src_x = 0, short src_y = 0, short src_w = 0, short src_h = 0, const unsigned int color = COLOR_WHITE);

    protected:
        void FillTexture() override;

};

#endif // !GLARCHIVITEM_BITMAP_INCLUDED
