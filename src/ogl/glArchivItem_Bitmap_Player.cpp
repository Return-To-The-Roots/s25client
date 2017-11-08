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

#include "rttrDefines.h" // IWYU pragma: keep
#include "glArchivItem_Bitmap_Player.h"
#include "Loader.h"
#include "Point.h"
#include "drivers/VideoDriverWrapper.h"
#include "oglIncludes.h"
#include <vector>

Extent glArchivItem_Bitmap_Player::CalcTextureSize() const
{
    // We have the texture 2 times: one with non-player colors and one with them
    return VIDEODRIVER.calcPreferredTextureSize(Extent(GetSize().x * 2, GetSize().y));
}

void glArchivItem_Bitmap_Player::DrawFull(const Rect& destArea, unsigned color, unsigned player_color)
{
    Draw(destArea, Rect(Position::all(0), GetSize()), color, player_color);
}

void glArchivItem_Bitmap_Player::DrawFull(const DrawPoint& dst, unsigned color, unsigned player_color)
{
    DrawFull(Rect(dst, GetSize()), color, player_color);
}

void glArchivItem_Bitmap_Player::Draw(Rect dstArea, Rect srcArea, unsigned color /*= COLOR_WHITE*/, unsigned player_color /*= COLOR_WHITE*/)
{
    if(GetTexture() == 0)
        return;

    RTTR_Assert(dstArea.getSize().x > 0 && dstArea.getSize().y > 0);
    RTTR_Assert(srcArea.getSize().x > 0 && srcArea.getSize().y > 0);
    // Compatibility only!
    Extent srcSize = srcArea.getSize();
    if(srcSize.x == 0)
        srcSize.x = getWidth();
    if(srcSize.y == 0)
        srcSize.y = getHeight();
    srcArea.setSize(srcSize);
    Extent dstSize = dstArea.getSize();
    if(dstSize.x == 0)
        dstSize.x = srcSize.x;
    if(dstSize.y == 0)
        dstSize.y = srcSize.y;
    dstArea.setSize(dstSize);

    Point<GLfloat> texCoords[8], vertices[8];

    dstArea.move(-GetOrigin());

    vertices[0].x = vertices[1].x = GLfloat(dstArea.left);
    vertices[2].x = vertices[3].x = GLfloat(dstArea.right);
    vertices[0].y = vertices[3].y = GLfloat(dstArea.top);
    vertices[1].y = vertices[2].y = GLfloat(dstArea.bottom);

    Point<GLfloat> srcOrig = Point<GLfloat>(srcArea.getOrigin()) / GetTexSize();
    Point<GLfloat> srcEndPt = Point<GLfloat>(srcArea.getEndPt()) / GetTexSize();
    texCoords[0].x = texCoords[1].x = srcOrig.x;
    texCoords[2].x = texCoords[3].x = srcEndPt.x;
    texCoords[0].y = texCoords[3].y = srcOrig.y;
    texCoords[1].y = texCoords[2].y = srcEndPt.y;

    std::copy(vertices, vertices + 4, vertices + 4);
    std::copy(texCoords, texCoords + 4, texCoords + 4);

    texCoords[4].x += 0.5f;
    texCoords[5].x += 0.5f;
    texCoords[6].x += 0.5f;
    texCoords[7].x += 0.5f;

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

    glEnableClientState(GL_COLOR_ARRAY);
    glVertexPointer(2, GL_FLOAT, 0, vertices);
    glTexCoordPointer(2, GL_FLOAT, 0, texCoords);
    glColorPointer(4, GL_UNSIGNED_BYTE, 0, colors);
    VIDEODRIVER.BindTexture(GetTexture());
    glDrawArrays(GL_QUADS, 0, 8);
    glDisableClientState(GL_COLOR_ARRAY);
}

void glArchivItem_Bitmap_Player::FillTexture()
{
    // Spezialpalette (blaue Spielerfarben sind Grau) verwenden,
    // damit man per OpenGL einfärben kann!
    const libsiedler2::ArchivItem_Palette* palette = LOADER.GetPaletteN("colors");

    int iformat = GetInternalFormat(), dformat = GL_BGRA; // GL_BGRA_EXT;

    Extent texSize = GetTexSize();
    std::vector<unsigned char> buffer(prodOfComponents(texSize) * 4);

    print(&buffer.front(), texSize.x, texSize.y, libsiedler2::FORMAT_BGRA, palette, 128, 0, 0, 0, 0, 0, 0, false);
    print(&buffer.front(), texSize.x, texSize.y, libsiedler2::FORMAT_BGRA, palette, 128, texSize.x / 2u, 0, 0, 0, 0, 0, true);
    glTexImage2D(GL_TEXTURE_2D, 0, iformat, texSize.x, texSize.y, 0, dformat, GL_UNSIGNED_BYTE, &buffer.front());
}
