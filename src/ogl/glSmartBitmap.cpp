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
#include "glSmartBitmap.h"
#include "drivers/VideoDriverWrapper.h"
#include "Loader.h"
#include "libsiedler2/src/ArchivItem_Bitmap.h"
#include "libsiedler2/src/ArchivItem_Bitmap_Player.h"
#include "libutil/src/colors.h"
#include "oglIncludes.h"
#include <boost/foreach.hpp>
#include <limits>
#include <climits>

glSmartBitmap::~glSmartBitmap()
{
    reset();
}

void glSmartBitmap::reset()
{
    if (texture && !sharedTexture)
        VIDEODRIVER.DeleteTexture(texture);
    texture = 0;

    BOOST_FOREACH(glBitmapItem& bmpItem, items)
    {
        if(bmpItem.isOwning_)
            delete bmpItem.bmp;
    }
    items.clear();
}

unsigned glSmartBitmap::nextPowerOfTwo(unsigned k)
{
    if (k == 0)
        return 1;

    k--;

    for (unsigned i = 1; i < sizeof(unsigned)*CHAR_BIT; i *= 2)
    {
        k = k | k >> i;
    }

    return k + 1;
}

void glSmartBitmap::calcDimensions()
{
    if (items.empty())
    {
        origin = Position(0, 0);
        size = Extent(0, 0);
        return;
    }

    Position maxPos(0, 0);

    origin.x = origin.y = std::numeric_limits<int>::min();

    hasPlayer = false;

    BOOST_FOREACH(const glBitmapItem& bmpItem, items)
    {
        if (bmpItem.type == TYPE_ARCHIVITEM_BITMAP_PLAYER)
            hasPlayer = true;

        origin = elMax(origin, bmpItem.origin);
        maxPos = elMax(maxPos, bmpItem.size - bmpItem.origin);
    }

    size = Extent(origin + maxPos);
}

void glSmartBitmap::drawTo(std::vector<uint32_t>& buffer, const Extent& bufferSize, const Extent& bufOffset /*= Extent(0, 0)*/) const
{
    libsiedler2::ArchivItem_Palette* p_colors = LOADER.GetPaletteN("colors");
    libsiedler2::ArchivItem_Palette* p_5 = LOADER.GetPaletteN("pal5");
    std::vector<uint32_t> tmp;

    BOOST_FOREACH(const glBitmapItem& bmpItem, items)
    {
        if ((bmpItem.size.x == 0) || (bmpItem.size.y == 0))
            continue;

        DrawPoint offset = origin - bmpItem.origin;

        if(bmpItem.type == TYPE_ARCHIVITEM_BITMAP_SHADOW)
        {
            tmp.resize(size.x * size.y);

            dynamic_cast<libsiedler2::baseArchivItem_Bitmap*>(bmpItem.bmp)
                ->print(reinterpret_cast<unsigned char*>(&tmp.front()), size.x, size.y, libsiedler2::FORMAT_RGBA, p_5,
                    offset.x, offset.y, bmpItem.pos.x, bmpItem.pos.y, bmpItem.size.x, bmpItem.size.y);

            unsigned tmpIdx = 0;

            for(unsigned y = 0; y < size.y; ++y)
            {
                unsigned idx = (bufOffset.y + y) * bufferSize.x + bufOffset.x;

                for(unsigned x = 0; x < size.x; ++x)
                {
                    if(GetAlpha(tmp[tmpIdx]) != 0x00 && GetAlpha(buffer[idx]) == 0x00)
                    {
                        buffer[idx] = MakeColor(0x40, 0, 0, 0);
                    }

                    idx++;
                    tmpIdx++;
                }
            }
        } else if(!hasPlayer)
        {
            // No player bitmap -> Just (over)write the data
            RTTR_Assert(bmpItem.type == TYPE_ARCHIVITEM_BITMAP);
            dynamic_cast<libsiedler2::baseArchivItem_Bitmap*>(bmpItem.bmp)
                ->print(reinterpret_cast<unsigned char*>(&buffer.front()), bufferSize.x, bufferSize.y, libsiedler2::FORMAT_RGBA, p_5,
                    offset.x + bufOffset.x, offset.y + bufOffset.y, bmpItem.pos.x, bmpItem.pos.y, bmpItem.size.x, bmpItem.size.y);
        } else
        {
            // There is a player bitmap -> First write to temp buffer
            tmp.resize(size.x * size.y);
            if (bmpItem.type == TYPE_ARCHIVITEM_BITMAP)
            {
                dynamic_cast<libsiedler2::baseArchivItem_Bitmap*>(bmpItem.bmp)
                    ->print(reinterpret_cast<unsigned char*>(&tmp.front()), size.x, size.y, libsiedler2::FORMAT_RGBA, p_5,
                        offset.x, offset.y, bmpItem.pos.x, bmpItem.pos.y, bmpItem.size.x, bmpItem.size.y);
            } else
            {
                dynamic_cast<libsiedler2::ArchivItem_Bitmap_Player*>(bmpItem.bmp)
                    ->print(reinterpret_cast<unsigned char*>(&tmp.front()), size.x, size.y, libsiedler2::FORMAT_RGBA, p_colors, 128,
                        offset.x, offset.y, bmpItem.pos.x, bmpItem.pos.y, bmpItem.size.x, bmpItem.size.y, false);
            }
            // Now copy temp buffer to real buffer, but we need to reset all player colors that would be overwritten
            // so it looks like, the first bitmap is fully drawn (including player colors) and then the next
            // overwrites it
            for(unsigned y = offset.y; y < size.y; y++)
            {
                for(unsigned x = offset.x; x < size.x; x++)
                {
                    // Check for non-transparent pixels
                    if(tmp[y * size.x + x])
                    {
                        // Copy to buffer
                        buffer[(y + bufOffset.y) * bufferSize.x + x + bufOffset.x] = tmp[y * size.x + x];
                        // Reset player color to transparent
                        buffer[(y + bufOffset.y) * bufferSize.x + x + bufOffset.x + size.x] = 0;
                    }
                }
            }
            // Finally write the player color part if it has one
            if(bmpItem.type == TYPE_ARCHIVITEM_BITMAP_PLAYER)
            {
                dynamic_cast<libsiedler2::ArchivItem_Bitmap_Player*>(bmpItem.bmp)
                    ->print(reinterpret_cast<unsigned char*>(&buffer.front()), bufferSize.x, bufferSize.y, libsiedler2::FORMAT_RGBA, p_colors, 128,
                        offset.x + size.x + bufOffset.x, offset.y + bufOffset.y, bmpItem.pos.x, bmpItem.pos.y, bmpItem.size.x, bmpItem.size.y, true);
            }
        }
    }
}

void glSmartBitmap::generateTexture()
{
    if (items.empty())
        return;

    if (!texture)
    {
        texture = VIDEODRIVER.GenerateTexture();

        if (!texture)
            return;
    }

    calcDimensions();

    size = Extent(nextPowerOfTwo(size.x), nextPowerOfTwo(size.y));

    // do we have a player-colored overlay?
    unsigned stride = hasPlayer ? size.x * 2 : size.x;

    std::vector<uint32_t> buffer(stride * size.y);
    drawTo(buffer, Extent(stride, size.y));

    VIDEODRIVER.BindTexture(texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, stride, size.y, 0, GL_BGRA, GL_UNSIGNED_BYTE, &buffer.front());

    texCoords[0].x = texCoords[1].x = 0.0f;
    texCoords[2].x = texCoords[3].x = hasPlayer ? 0.5f : 1.0f;

    texCoords[0].y = texCoords[3].y = texCoords[4].y = texCoords[7].y = 0.0f;
    texCoords[1].y = texCoords[2].y = texCoords[5].y = texCoords[6].y = 1.0f;

    texCoords[4].x = texCoords[5].x = 0.5f;
    texCoords[6].x = texCoords[7].x = 1.0f;
}

void glSmartBitmap::draw(DrawPoint drawPt, unsigned color, unsigned player_color)
{
    drawPercent(drawPt, 100, color, player_color);
}

void glSmartBitmap::drawPercent(DrawPoint drawPt, unsigned percent, unsigned color, unsigned player_color)
{
    if (!texture)
    {
        generateTexture();

        if (!texture)
            return;
    }

    // nothing to draw?
    if (!percent)
        return;
    RTTR_Assert(percent <= 100);

    const float partDrawn = percent / 100.f;
    Point<GLfloat> vertices[8], curTexCoords[8];
    struct
    {
        GLbyte r, g, b, a;
    } colors[8];

    drawPt -= origin;

    vertices[2] = Point<GLfloat>(drawPt + size);

    vertices[0].x = vertices[1].x = GLfloat(drawPt.x);
    vertices[3].x = vertices[2].x;

    vertices[0].y = vertices[3].y = GLfloat(drawPt.y + size.y - size.y * partDrawn);
    vertices[1].y = vertices[2].y;

    colors[0].r = GetRed(color);
    colors[0].g = GetGreen(color);
    colors[0].b = GetBlue(color);
    colors[0].a = GetAlpha(color);
    colors[3] = colors[2] = colors[1] = colors[0];

    curTexCoords[0] = texCoords[0];
    curTexCoords[1] = texCoords[1];
    curTexCoords[2] = texCoords[2];
    curTexCoords[3] = texCoords[3];
    curTexCoords[0].y = curTexCoords[3].y = curTexCoords[1].y - (curTexCoords[1].y - curTexCoords[0].y) * partDrawn;

    int numQuads;
    if (player_color && hasPlayer)
    {
        std::copy(vertices, vertices + 4, vertices + 4);

        colors[4].r = GetRed(player_color);
        colors[4].g = GetGreen(player_color);
        colors[4].b = GetBlue(player_color);
        colors[4].a = GetAlpha(player_color);
        colors[7] = colors[6] = colors[5] = colors[4];

        curTexCoords[4] = texCoords[4];
        curTexCoords[5] = texCoords[5];
        curTexCoords[6] = texCoords[6];
        curTexCoords[7] = texCoords[7];
        curTexCoords[4].y = curTexCoords[7].y = curTexCoords[0].y;

        numQuads = 8;
    } else
        numQuads = 4;

    glEnableClientState(GL_COLOR_ARRAY);
    glVertexPointer(2, GL_FLOAT, 0, vertices);
    glTexCoordPointer(2, GL_FLOAT, 0, curTexCoords);
    glColorPointer(4, GL_UNSIGNED_BYTE, 0, colors);
    VIDEODRIVER.BindTexture(texture);
    glDrawArrays(GL_QUADS, 0, numQuads);
    glDisableClientState(GL_COLOR_ARRAY);
}
