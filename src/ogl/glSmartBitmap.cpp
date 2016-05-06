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
#include <limits>
#include <climits>

void glSmartBitmap::reset()
{
    if (texture && !sharedTexture)
        VIDEODRIVER.DeleteTexture(texture);
    texture = 0;

    for(std::vector<glBitmapItem>::iterator it = items.begin(); it != items.end(); ++it)
    {
        if(it->isOwning_)
            delete it->bmp;
    }
    items.clear();
}

glSmartBitmap::~glSmartBitmap()
{
    reset();
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
        nx = ny = 0;
        w = h = 0;
        return;
    }

    int max_x = 0;
    int max_y = 0;

    nx = ny = std::numeric_limits<int>::min();

    hasPlayer = false;

    for (std::vector<glBitmapItem>::const_iterator it = items.begin(); it != items.end(); ++it)
    {
        if (it->type == TYPE_ARCHIVITEM_BITMAP_PLAYER)
            hasPlayer = true;

        if (nx < it->nx)
            nx = it->nx;
        if (ny < it->ny)
            ny = it->ny;

        if (max_x < it->w - it->nx)
            max_x = it->w - it->nx;
        if (max_y < it->h - it->ny)
            max_y = it->h - it->ny;
    }

    w = nx + max_x;
    h = ny + max_y;
}

void glSmartBitmap::drawTo(std::vector<uint32_t>& buffer, unsigned const stride, unsigned const height, int const x_offset, int const y_offset)
{
    libsiedler2::ArchivItem_Palette* p_colors = LOADER.GetPaletteN("colors");
    libsiedler2::ArchivItem_Palette* p_5 = LOADER.GetPaletteN("pal5");

    for (std::vector<glBitmapItem>::const_iterator it = items.begin(); it != items.end(); ++it)
    {
        if ((it->w == 0) || (it->h == 0))
            continue;

        const unsigned xo = (nx - it->nx);
        const unsigned yo = (ny - it->ny);

        if(it->type == TYPE_ARCHIVITEM_BITMAP_SHADOW)
        {
            std::vector<uint32_t> tmp(w * h);

            dynamic_cast<libsiedler2::baseArchivItem_Bitmap*>(it->bmp)
                ->print(reinterpret_cast<unsigned char*>(&tmp.front()), w, h, libsiedler2::FORMAT_RGBA, p_5,
                    xo, yo, it->x, it->y, it->w, it->h);

            unsigned tmpIdx = 0;

            for(int y = 0; y < h; ++y)
            {
                unsigned idx = (y_offset + y) * stride + x_offset;

                for(int x = 0; x < w; ++x)
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
            RTTR_Assert(it->type == TYPE_ARCHIVITEM_BITMAP);
            dynamic_cast<libsiedler2::baseArchivItem_Bitmap*>(it->bmp)
                ->print(reinterpret_cast<unsigned char*>(&buffer.front()), stride, height, libsiedler2::FORMAT_RGBA, p_5,
                    xo + x_offset, yo + y_offset, it->x, it->y, it->w, it->h);
        } else
        {
            // There is a player bitmap -> First write to temp buffer
            std::vector<uint32_t> tmp(w * h);
            if (it->type == TYPE_ARCHIVITEM_BITMAP)
            {
                dynamic_cast<libsiedler2::baseArchivItem_Bitmap*>(it->bmp)
                    ->print(reinterpret_cast<unsigned char*>(&tmp.front()), w, h, libsiedler2::FORMAT_RGBA, p_5,
                        xo, yo, it->x, it->y, it->w, it->h);
            } else
            {
                dynamic_cast<libsiedler2::ArchivItem_Bitmap_Player*>(it->bmp)
                    ->print(reinterpret_cast<unsigned char*>(&tmp.front()), w, h, libsiedler2::FORMAT_RGBA, p_colors, 128,
                        xo, yo, it->x, it->y, it->w, it->h, false);
            }
            // Now copy temp buffer to real buffer, but we need to reset all player colors that would be overwritten
            // so it looks like, the first bitmap is fully drawn (including player colors) and then the next
            // overwrites it
            for(int y = yo; y < h; y++)
            {
                for(int x = xo; x < w; x++)
                {
                    // Check for non-transparent pixels
                    if(tmp[y * w + x])
                    {
                        // Copy to buffer
                        buffer[(y + y_offset) * stride + x + x_offset] = tmp[y * w + x];
                        // Reset player color to transparent
                        buffer[(y + y_offset) * stride + x + x_offset + w] = 0;
                    }
                }
            }
            // Finally write the player color part if it has one
            if(it->type == TYPE_ARCHIVITEM_BITMAP_PLAYER)
            {
                dynamic_cast<libsiedler2::ArchivItem_Bitmap_Player*>(it->bmp)
                    ->print(reinterpret_cast<unsigned char*>(&buffer.front()), stride, height, libsiedler2::FORMAT_RGBA, p_colors, 128,
                        xo + w + x_offset, yo + y_offset, it->x, it->y, it->w, it->h, true);
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

    w = nextPowerOfTwo(w);
    h = nextPowerOfTwo(h);

    // do we have a player-colored overlay?
    unsigned stride = hasPlayer ? w * 2 : w;

    std::vector<uint32_t> buffer(stride * h);
    drawTo(buffer, stride, h);

    VIDEODRIVER.BindTexture(texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, stride, h, 0, GL_BGRA, GL_UNSIGNED_BYTE, &buffer.front());

    texCoords[0].x = texCoords[1].x = 0.0f;
    texCoords[2].x = texCoords[3].x = hasPlayer ? 0.5f : 1.0f;

    texCoords[0].y = texCoords[3].y = texCoords[4].y = texCoords[7].y = 0.0f;
    texCoords[1].y = texCoords[2].y = texCoords[5].y = texCoords[6].y = 1.0f;

    texCoords[4].x = texCoords[5].x = 0.5f;
    texCoords[6].x = texCoords[7].x = 1.0f;
}

void glSmartBitmap::draw(int x, int y, unsigned color, unsigned player_color)
{
    drawPercent(x, y, 100, color, player_color);
}

void glSmartBitmap::drawPercent(int x, int y, unsigned percent, unsigned color, unsigned player_color)
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

    vertices[0].x = vertices[1].x = GLfloat(x - nx);
    vertices[2].x = vertices[3].x = GLfloat(x - nx + w);

    vertices[0].y = vertices[3].y = GLfloat(y - ny + h - h * partDrawn);
    vertices[1].y = vertices[2].y = GLfloat(y - ny + h);

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

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    glVertexPointer(2, GL_FLOAT, 0, vertices);
    glTexCoordPointer(2, GL_FLOAT, 0, curTexCoords);
    glColorPointer(4, GL_UNSIGNED_BYTE, 0, colors);
    VIDEODRIVER.BindTexture(texture);
    glDrawArrays(GL_QUADS, 0, numQuads);
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
}
