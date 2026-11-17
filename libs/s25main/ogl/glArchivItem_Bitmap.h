// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "DrawPoint.h"
#include "ITexture.h"
#include "Rect.h"
#include "glArchivItem_BitmapBase.h"
#include "libsiedler2/ArchivItem_Bitmap.h"
#include "s25util/colors.h"

/// Basisklasse für GL-Bitmapitems.
class glArchivItem_Bitmap : public virtual libsiedler2::baseArchivItem_Bitmap, public glArchivItem_BitmapBase
{
public:
    glArchivItem_Bitmap();
    glArchivItem_Bitmap(const glArchivItem_Bitmap& item);

    /// Draw the texture in the given rect, stretching if required
    void drawFull(const Rect& destArea, unsigned color = COLOR_WHITE);
    /// Draw the texture to the given position with full size
    void drawFull(const DrawPoint& dstPos, unsigned color = COLOR_WHITE) override;
    /// Draw a rectangular part of the texture. offset specifies the offset from the origin of the texture
    void drawPart(const Rect& destArea, const DrawPoint& offset, unsigned color = COLOR_WHITE);
    /// Draw a rectangular part of the texture from the origin of it
    void drawPart(const Rect& destArea, unsigned color = COLOR_WHITE);
    /// Draw only percent% of the height of the image, counting from the bottom of the image
    void drawPercent(const DrawPoint& dstPos, unsigned percent, unsigned color = COLOR_WHITE);
    /// Draw the texture.
    /// src_w/h default to the full bitmap size
    /// dst_w/h default the src_w/h
    void draw(Rect dstArea, Rect srcArea, unsigned color = COLOR_WHITE) override;

protected:
    void fillTexture() override;
    Extent calcTextureSize() const override;
};
