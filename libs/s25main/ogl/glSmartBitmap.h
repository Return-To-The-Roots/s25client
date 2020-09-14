// Copyright (c) 2005 - 2020 Settlers Freaks (sf-team at siedler25.org)
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

#include "DrawPoint.h"
#include "ITexture.h"
#include <array>
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
    std::array<Point<float>, 8> texCoords;

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
    void draw(DrawPoint drawPt, unsigned color = 0xFFFFFFFF, unsigned player_color = 0);
    void drawForPlayer(DrawPoint drawPt, unsigned player_color) { draw(drawPt, 0xFFFFFFFF, player_color); }
    void drawPercent(DrawPoint drawPt, unsigned percent, unsigned color = 0xFFFFFFFF, unsigned player_color = 0);
    /// Draw the bitmap(s) to the specified buffer at the position starting at bufOffset (must be positive)
    void drawTo(libsiedler2::PixelBufferBGRA& buffer, const Extent& bufOffset = Extent(0, 0)) const;

    void add(libsiedler2::baseArchivItem_Bitmap* bmp, bool transferOwnership = false);
    void add(libsiedler2::ArchivItem_Bitmap_Player* bmp, bool transferOwnership = false);
    void addShadow(libsiedler2::baseArchivItem_Bitmap* bmp, bool transferOwnership = false);
};
