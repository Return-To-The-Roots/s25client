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

///////////////////////////////////////////////////////////////////////////////
// Header
#include "defines.h"
#include "glArchivItem_Bitmap.h"

#include "drivers/VideoDriverWrapper.h"
#include "GlobalVars.h"
#include "Loader.h"

#include "../libsiedler2/src/types.h"
#include <vector>

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#if defined _WIN32 && defined _DEBUG && defined _MSC_VER
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

///////////////////////////////////////////////////////////////////////////////
/** @class glArchivItem_Bitmap
 *
 *  Basisklasse für GL-Bitmapitems.
 *
 *  @author FloSoft
 */

///////////////////////////////////////////////////////////////////////////////
/** @var glArchivItem_Bitmap::texture
 *
 *  OpenGL-Textur des Bildes.
 *
 *  @author FloSoft
 */

///////////////////////////////////////////////////////////////////////////////
/**
 *  Konstruktor von @p glArchivItem_Bitmap.
 *
 *  @author FloSoft
 */
glArchivItem_Bitmap::glArchivItem_Bitmap(void)
    : baseArchivItem_Bitmap(), texture(0), filter(GL_NEAREST)
{
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Kopiekonstruktor von @p glArchivItem_Bitmap.
 *
 *  @author FloSoft
 */
glArchivItem_Bitmap::glArchivItem_Bitmap(const glArchivItem_Bitmap& item)
    : baseArchivItem_Bitmap(item), texture(0), filter(item.filter)
{
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Destruktor von @p glArchivItem_Bitmap.
 *
 *  @author FloSoft
 */
glArchivItem_Bitmap::~glArchivItem_Bitmap(void)
{
    DeleteTexture();
}

glArchivItem_Bitmap& glArchivItem_Bitmap::operator=(const glArchivItem_Bitmap& item)
{
    if(this == &item)
        return *this;
    baseArchivItem_Bitmap::operator=(item);
    texture = 0;
    filter = item.filter;
    return *this;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Zeichnet die Textur.
 *
 *  @author FloSoft
 */
void glArchivItem_Bitmap::Draw(short dst_x, short dst_y, short dst_w, short dst_h, short src_x, short src_y, short src_w, short src_h, const unsigned int color, const unsigned int unused)
{
    if(texture == 0)
        GenerateTexture();
    if(texture == 0)
        return;

    if(src_w == 0)
        src_w = width_;
    if(src_h == 0)
        src_h = height_;
    if(dst_w == 0)
        dst_w = src_w;
    if(dst_h == 0)
        dst_h = src_h;

    VIDEODRIVER.BindTexture(texture);

    assert(getBobType() != libsiedler2::BOBTYPE_BITMAP_PLAYER);

    struct GL_T2F_C4UB_V3F_Struct
    {
        GLfloat tx, ty;
        GLubyte r, g, b, a;
        GLfloat x, y, z;
    };

    GL_T2F_C4UB_V3F_Struct tmp[4];

    int x = -nx_ + dst_x;
    int y = -ny_ + dst_y;

    tmp[0].x = tmp[1].x = GLfloat(x);
    tmp[2].x = tmp[3].x = GLfloat(x + dst_w);

    tmp[0].y = tmp[3].y = GLfloat(y);
    tmp[1].y = tmp[2].y = GLfloat(y + dst_h);

    tmp[0].z = tmp[1].z = tmp[2].z = tmp[3].z = 0.0f;

    tmp[0].tx = tmp[1].tx = (GLfloat)src_x / tex_width_;
    tmp[2].tx = tmp[3].tx = (GLfloat)(src_x + src_w) / tex_width_;

    tmp[0].ty = tmp[3].ty = (GLfloat)src_y / tex_height_;
    tmp[1].ty = tmp[2].ty = (GLfloat)(src_y + src_h) / tex_height_;

    tmp[0].r = tmp[1].r = tmp[2].r = tmp[3].r = GetRed(color);
    tmp[0].g = tmp[1].g = tmp[2].g = tmp[3].g = GetGreen(color);
    tmp[0].b = tmp[1].b = tmp[2].b = tmp[3].b = GetBlue(color);
    tmp[0].a = tmp[1].a = tmp[2].a = tmp[3].a = GetAlpha(color);

    glInterleavedArrays(GL_T2F_C4UB_V3F, 0, tmp);
    glDrawArrays(GL_QUADS, 0, 4);

    return;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Liefert das GL-Textur-Handle.
 *
 *  @author FloSoft
 */
unsigned int glArchivItem_Bitmap::GetTexture()
{
    if(texture == 0)
        GenerateTexture();
    return texture;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Löscht die GL-Textur (z.B fürs Neuerstellen)
 *
 *  @author FloSoft
 */
void glArchivItem_Bitmap::DeleteTexture()
{
    VIDEODRIVER.DeleteTexture(texture);
    //glDeleteTextures(1, (const GLuint*)&texture);
    texture = 0;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Setzt den Texturfilter auf einen bestimmten Wert.
 *
 *  @author FloSoft
 */
void glArchivItem_Bitmap::setFilter(unsigned int filter)
{
    if(this->filter == filter)
        return;

    this->filter = filter;

    // neugenerierung der Textur anstoßen
    if(texture != 0)
        DeleteTexture();
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Erzeugt die Textur.
 *
 *  @author FloSoft
 */
void glArchivItem_Bitmap::GenerateTexture(void)
{
    texture = VIDEODRIVER.GenerateTexture();

    if(!palette_)
        setPalette(LOADER.GetPaletteN("pal5"));

    VIDEODRIVER.BindTexture(texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);

    int iformat = GL_RGBA, dformat = GL_BGRA;

    std::vector<unsigned char> buffer(tex_width_ * tex_height_ * 4);

    print(&buffer.front(), tex_width_, tex_height_, libsiedler2::FORMAT_RGBA, palette_, 0, 0, 0, 0, 0, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, iformat, tex_width_, tex_height_, 0, dformat, GL_UNSIGNED_BYTE, &buffer.front());
}

