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

#ifndef GLBITMAP_H_INCLUDED
#define GLBITMAP_H_INCLUDED

#pragma once

#include "ogl/glBitmapItem.h"
#include "DrawPoint.h"
#include <vector>
#include <stdint.h>

namespace libsiedler2
{
    class baseArchivItem_Bitmap;
    class ArchivItem_Bitmap_Player;
}

class glSmartBitmap
{
    private:
        Position origin;
        Extent size;

        bool sharedTexture;
        unsigned int texture;

        bool hasPlayer;

        std::vector<glBitmapItem> items;

    public:
        Point<float> texCoords[8];

        glSmartBitmap() : origin(0, 0), size(0, 0), sharedTexture(false), texture(0), hasPlayer(false) {}
        ~glSmartBitmap();
        void reset();

        Extent getSize() const {return size;}

        Extent getTexSize() const {return hasPlayer ? Extent(size.x *2, size.y) : size;}

        bool isGenerated() const {return texture != 0;}
        bool isPlayer() const {return hasPlayer;}
        bool empty() const { return items.empty(); }

        void setSharedTexture(unsigned tex) { texture = tex; sharedTexture = (tex != 0); }

        void calcDimensions();

        void generateTexture();
        void draw(DrawPoint drawPt, unsigned color = 0xFFFFFFFF, unsigned player_color = 0);
        void drawPercent(DrawPoint drawPt, unsigned percent, unsigned color = 0xFFFFFFFF, unsigned player_color = 0);

        void drawTo(std::vector<uint32_t>& buffer, const Extent& bufferSize, const Extent& bufOffset = Extent(0, 0));

        void add(libsiedler2::baseArchivItem_Bitmap* bmp, bool transferOwnership = false) {if (bmp) items.push_back(glBitmapItem(bmp, false, transferOwnership));}
        void add(libsiedler2::ArchivItem_Bitmap_Player* bmp, bool transferOwnership = false) {if (bmp) items.push_back(glBitmapItem(bmp, transferOwnership));}
        void addShadow(libsiedler2::baseArchivItem_Bitmap* bmp, bool transferOwnership = false) {if (bmp) items.push_back(glBitmapItem(bmp, true, transferOwnership));}

        static unsigned nextPowerOfTwo(unsigned k);
};

#endif
