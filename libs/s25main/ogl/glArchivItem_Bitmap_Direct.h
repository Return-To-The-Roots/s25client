// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

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
