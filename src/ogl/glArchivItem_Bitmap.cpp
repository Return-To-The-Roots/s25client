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
#include "glArchivItem_Bitmap.h"
#include "Point.h"
#include "drivers/VideoDriverWrapper.h"
#include "ogl/oglIncludes.h"
#include <vector>

glArchivItem_Bitmap::glArchivItem_Bitmap()
{
}

glArchivItem_Bitmap::glArchivItem_Bitmap(const glArchivItem_Bitmap& item)
    : ArchivItem_BitmapBase(item), baseArchivItem_Bitmap(item), glArchivItem_BitmapBase(item)
{
}

/**
 *  Zeichnet die Textur.
 */
void glArchivItem_Bitmap::Draw(DrawPoint dst, short dst_w, short dst_h, short src_x, short src_y, short src_w, short src_h, const unsigned int color)
{
    if(GetTexture() == 0)
        return;

    if(src_w == 0)
        src_w = width_;
    if(src_h == 0)
        src_h = height_;
    if(dst_w == 0)
        dst_w = src_w;
    if(dst_h == 0)
        dst_h = src_h;

    RTTR_Assert(getBobType() != libsiedler2::BOBTYPE_BITMAP_PLAYER);

    Point<GLfloat> texCoords[4], vertices[4];

    int x = -nx_ + dst.x;
    int y = -ny_ + dst.y;

    vertices[0].x = vertices[1].x = GLfloat(x);
    vertices[2].x = vertices[3].x = GLfloat(x + dst_w);
    vertices[0].y = vertices[3].y = GLfloat(y);
    vertices[1].y = vertices[2].y = GLfloat(y + dst_h);

    texCoords[0].x = texCoords[1].x = GLfloat(src_x) / tex_width_;
    texCoords[2].x = texCoords[3].x = GLfloat(src_x + src_w) / tex_width_;
    texCoords[0].y = texCoords[3].y = GLfloat(src_y) / tex_height_;
    texCoords[1].y = texCoords[2].y = GLfloat(src_y + src_h) / tex_height_;

    glVertexPointer(2, GL_FLOAT, 0, vertices);
    glTexCoordPointer(2, GL_FLOAT, 0, texCoords);
    VIDEODRIVER.BindTexture(GetTexture());
    glColor4ub(GetRed(color), GetGreen(color), GetBlue(color), GetAlpha(color));
    glDrawArrays(GL_QUADS, 0, 4);
}

void glArchivItem_Bitmap::Draw(const Rect& rect, const unsigned color)
{
    Draw(rect.getOrigin(), 0, 0, 0, 0, rect.getSize().x, rect.getSize().y, color);
}

void glArchivItem_Bitmap::DrawFull(const DrawPoint& dst, const unsigned color /*= COLOR_WHITE*/)
{
    Draw(Rect(dst, width_, height_), color);
}

void glArchivItem_Bitmap::DrawPart(const Rect& rect, const DrawPoint& offset, unsigned color)
 {
     Draw(DrawPoint(rect.getOrigin()), rect.getSize().x, rect.getSize().y, offset.x, offset.y, rect.getSize().x, rect.getSize().y, color);
 }

void glArchivItem_Bitmap::FillTexture()
{
    int iformat = GetInternalFormat(), dformat = GL_BGRA;

    std::vector<unsigned char> buffer(tex_width_ * tex_height_ * 4);

    print(&buffer.front(), tex_width_, tex_height_, libsiedler2::FORMAT_RGBA, palette_, 0, 0, 0, 0, 0, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, iformat, tex_width_, tex_height_, 0, dformat, GL_UNSIGNED_BYTE, &buffer.front());
}

