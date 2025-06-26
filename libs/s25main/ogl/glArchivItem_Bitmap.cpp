// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "glArchivItem_Bitmap.h"
#include "Point.h"
#include "drivers/VideoDriverWrapper.h"
#include "libsiedler2/PixelBufferBGRA.h"
#ifdef __EMSCRIPTEN__
#include "SDL/SDL.h"
#include "SDL/SDL_opengl.h"
#else
#include <glad/glad.h>
#endif

glArchivItem_Bitmap::glArchivItem_Bitmap() = default;

glArchivItem_Bitmap::glArchivItem_Bitmap(const glArchivItem_Bitmap& item)
    : ArchivItem_BitmapBase(item), baseArchivItem_Bitmap(item), glArchivItem_BitmapBase(item)
{}

/**
 *  Zeichnet die Textur.
 */
void glArchivItem_Bitmap::Draw(Rect dstArea, Rect srcArea, unsigned color /*= COLOR_WHITE*/)
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

    RTTR_Assert(getBobType() != libsiedler2::BobType::BitmapPlayer);

    std::array<Point<GLfloat>, 4> texCoords, vertices;

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

    if (isCursor)
    {
        glVertexPointer(2, GL_FLOAT, 0, vertices.data());
        glTexCoordPointer(2, GL_FLOAT, 0, texCoords.data());
        VIDEODRIVER.BindTexture(GetTexture());
        glColor4ub(GetRed(color), GetGreen(color), GetBlue(color), GetAlpha(color));
        glDrawArrays(GL_QUADS, 0, 4);
    }
    else
    {
        VIDEODRIVER.BindTexture(GetTexture());
        glColor4ub(GetRed(color), GetGreen(color), GetBlue(color), GetAlpha(color));
        glBegin(GL_QUADS);
            glTexCoord2f(texCoords[0].x, texCoords[0].y); glVertex2f(vertices[0].x, vertices[0].y);
            glTexCoord2f(texCoords[1].x, texCoords[1].y); glVertex2f(vertices[1].x, vertices[1].y);
            glTexCoord2f(texCoords[2].x, texCoords[2].y); glVertex2f(vertices[2].x, vertices[2].y);
            glTexCoord2f(texCoords[3].x, texCoords[3].y); glVertex2f(vertices[3].x, vertices[3].y);
        glEnd();
    }
}

void glArchivItem_Bitmap::DrawFull(const Rect& destArea, unsigned color)
{
    Draw(destArea, Rect(Position(0, 0), GetSize()), color);
}

void glArchivItem_Bitmap::DrawFull(const DrawPoint& dstPos, unsigned color)
{
    DrawFull(Rect(dstPos, GetSize()), color);
}

void glArchivItem_Bitmap::DrawCursor(const DrawPoint& dstPos)
{
    isCursor = true;
    DrawFull(Rect(dstPos, GetSize()), COLOR_WHITE);
    isCursor = false;
}


void glArchivItem_Bitmap::DrawPart(const Rect& destArea, const DrawPoint& offset, unsigned color)
{
    Draw(destArea, Rect(offset, destArea.getSize()), color);
}

void glArchivItem_Bitmap::DrawPart(const Rect& destArea, unsigned color /*= COLOR_WHITE*/)
{
    DrawPart(destArea, DrawPoint::all(0), color);
}

void glArchivItem_Bitmap::DrawPercent(const DrawPoint& dstPos, unsigned percent, unsigned color /*= COLOR_WHITE*/)
{
    RTTR_Assert(percent <= 100);
    if(percent == 0u)
        return;
    unsigned drawnHeight = getHeight() * std::min(100u, percent) / 100;
    DrawPoint offset(0, getHeight() - drawnHeight);
    DrawPart(Rect(dstPos + offset, getWidth(), drawnHeight), offset, color);
}

void glArchivItem_Bitmap::FillTexture()
{
    int iformat = GetInternalFormat(), dformat = GL_BGRA;

    const Extent texSize = GetTexSize();
    libsiedler2::PixelBufferBGRA buffer(texSize.x, texSize.y);
    print(buffer);
    glTexImage2D(GL_TEXTURE_2D, 0, iformat, texSize.x, texSize.y, 0, dformat, GL_UNSIGNED_BYTE, buffer.getPixelPtr());
}

Extent glArchivItem_Bitmap::CalcTextureSize() const
{
    return VIDEODRIVER.calcPreferredTextureSize(GetSize());
}
