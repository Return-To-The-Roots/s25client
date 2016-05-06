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
#include "glArchivItem_Bitmap_Player.h"
#include "Point.h"
#include "drivers/VideoDriverWrapper.h"
#include "Loader.h"
#include "oglIncludes.h"
#include <vector>

void glArchivItem_Bitmap_Player::Draw(short dst_x, short dst_y, short dst_w, short dst_h, short src_x, short src_y, short src_w, short src_h, const unsigned int color, const unsigned int player_color)
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

    Point<GLfloat> texCoords[8], vertices[8];

    int x = -nx_ + dst_x;
    int y = -ny_ + dst_y;

    vertices[0].x = vertices[1].x = GLfloat(x);
    vertices[2].x = vertices[3].x = GLfloat(x + dst_w);

    vertices[0].y = vertices[3].y = GLfloat(y);
    vertices[1].y = vertices[2].y = GLfloat(y + dst_h);

    texCoords[0].x = texCoords[1].x = (GLfloat)(src_x) / (GLfloat)tex_width_ / 2.0f;
    texCoords[2].x = texCoords[3].x = (GLfloat)(src_x + src_w) / (GLfloat)tex_width_ / 2.0f;

    texCoords[0].y = texCoords[3].y = (GLfloat)src_y / tex_height_;
    texCoords[1].y = texCoords[2].y = (GLfloat)(src_y + src_h) / tex_height_;

    std::copy(vertices, vertices + 4, vertices + 4);
    std::copy(texCoords, texCoords + 4, texCoords + 4);

    texCoords[4].x += 0.5;
    texCoords[5].x += 0.5;
    texCoords[6].x += 0.5;
    texCoords[7].x += 0.5;

    struct
    {
        GLbyte r, g, b, a;
    } colors[8];
    colors[0].r = GetRed(color);
    colors[0].g = GetGreen(color);
    colors[0].b = GetBlue(color);
    colors[0].a = GetAlpha(color);
    colors[3] = colors[2] = colors[1] = colors[0];

    colors[4].r = GetRed(player_color);
    colors[4].g = GetGreen(player_color);
    colors[4].b = GetBlue(player_color);
    colors[4].a = GetAlpha(player_color);
    colors[7] = colors[6] = colors[5] = colors[4];

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    glVertexPointer(2, GL_FLOAT, 0, vertices);
    glTexCoordPointer(2, GL_FLOAT, 0, texCoords);
    glColorPointer(4, GL_UNSIGNED_BYTE, 0, colors);
    VIDEODRIVER.BindTexture(GetTexture());
    glDrawArrays(GL_QUADS, 0, 8);
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
}

void glArchivItem_Bitmap_Player::FillTexture()
{
    // Spezialpalette (blaue Spielerfarben sind Grau) verwenden,
    // damit man per OpenGL einfärben kann!
    setPalette(LOADER.GetPaletteN("colors"));

    int iformat = GetInternalFormat(), dformat = GL_BGRA; //GL_BGRA_EXT;

    std::vector<unsigned char> buffer(tex_width_ * 2 * tex_height_ * 4);

    print(&buffer.front(), tex_width_ * 2, tex_height_, libsiedler2::FORMAT_RGBA, palette_, 128, 0, 0, 0, 0, 0, 0, false);
    print(&buffer.front(), tex_width_ * 2, tex_height_, libsiedler2::FORMAT_RGBA, palette_, 128, tex_width_, 0, 0, 0, 0, 0, true);
    glTexImage2D(GL_TEXTURE_2D, 0, iformat, tex_width_ * 2, tex_height_, 0, dformat, GL_UNSIGNED_BYTE, &buffer.front());
}
