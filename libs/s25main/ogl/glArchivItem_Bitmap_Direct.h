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

#include "Rect.h"
#include "glArchivItem_Bitmap.h"

namespace libsiedler2 {
struct ColorBGRA;
}

/// Klasse für GL-Direct-Bitmaps.
class glArchivItem_Bitmap_Direct : public glArchivItem_Bitmap //-V690
{
public:
    glArchivItem_Bitmap_Direct();
    glArchivItem_Bitmap_Direct(const glArchivItem_Bitmap_Direct& item);
    RTTR_CLONEABLE(glArchivItem_Bitmap_Direct)

    /// Call before updating texture
    void beginUpdate();
    /// Call after updating texture
    void endUpdate();
    /// Updates a pixels color
    void updatePixel(const DrawPoint& pos, const libsiedler2::ColorBGRA& clr);

    /// lädt die Bilddaten aus einer Datei.
    int load(std::istream& /*file*/, const libsiedler2::ArchivItem_Palette* /*palette*/) override { return 254; }
    /// schreibt die Bilddaten in eine Datei.
    int write(std::ostream& /*file*/, const libsiedler2::ArchivItem_Palette* /*palette*/) const override { return 254; }

private:
    bool isUpdating_;
    Rect areaToUpdate_;
};
