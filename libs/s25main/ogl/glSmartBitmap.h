// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "DrawPoint.h"
#include "ITexture.h"
#include <array>
#include <memory>
#include <vector>

namespace libsiedler2 {
class baseArchivItem_Bitmap;
class ArchivItem_Bitmap_Player;
class PixelBufferBGRA;
} // namespace libsiedler2

class glBitmapItem;

class glSmartBitmap : public ITexture
{
private:
    DrawPoint origin_;
    Extent size_;

    bool sharedTexture;
    unsigned texture;

    bool hasPlayer;

    std::vector<glBitmapItem> items;

    /// Calculate size, origin and hasPlayer based on current images
    void calcDimensions();

public:
    std::array<PointF, 8> texCoords;

    glSmartBitmap();
    ~glSmartBitmap();
    void reset();

    Position GetOrigin() const override { return origin_; }
    Extent GetSize() const override { return size_; }
    /// Return the space required on the texture
    Extent getRequiredTexSize() const;

    bool isGenerated() const { return texture != 0; }
    bool isPlayer() const { return hasPlayer; }
    bool empty() const { return items.empty(); }

    void setSharedTexture(unsigned tex)
    {
        texture = tex;
        sharedTexture = (tex != 0);
    }
    unsigned getTexture() const { return texture; }

    void generateTexture();
    void DrawFull(const Position& dstPos, unsigned color = 0xFFFFFFFF) override { draw(dstPos, color); }
    void Draw(Rect dstArea, Rect srcArea, unsigned color = 0xFFFFFFFF) override;
    void drawRect(Rect dstArea, Rect srcArea, unsigned color = 0xFFFFFFFF, unsigned player_color = 0);
    void draw(DrawPoint drawPt, unsigned color = 0xFFFFFFFF, unsigned player_color = 0);
    void drawForPlayer(DrawPoint drawPt, unsigned player_color) { draw(drawPt, 0xFFFFFFFF, player_color); }
    /// Draw only percent% of the height of the image, counting from the bottom of the image
    void drawPercent(DrawPoint drawPt, unsigned percent, unsigned color = 0xFFFFFFFF, unsigned player_color = 0);
    /// Draw the bitmap(s) to the specified buffer at the position starting at bufOffset (must be positive)
    void drawTo(libsiedler2::PixelBufferBGRA& buffer, const Extent& bufOffset = Extent(0, 0)) const;

    void add(libsiedler2::baseArchivItem_Bitmap* bmp);
    void add(std::unique_ptr<libsiedler2::baseArchivItem_Bitmap> bmp);
    void add(libsiedler2::ArchivItem_Bitmap_Player* bmp);
    void addShadow(libsiedler2::baseArchivItem_Bitmap* bmp);
};
