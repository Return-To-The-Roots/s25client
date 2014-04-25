// $Id: glArchivItem_Bitmap_Player.cpp 9357 2014-04-25 15:35:25Z FloSoft $
//
// Copyright (c) 2005 - 2011 Settlers Freaks (sf-team at siedler25.org)
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
#include "main.h"
#include "glArchivItem_Bitmap_Player.h"

#include "VideoDriverWrapper.h"
#include "GlobalVars.h"
#include "Loader.h"

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#if defined _WIN32 && defined _DEBUG && defined _MSC_VER
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

void glArchivItem_Bitmap_Player::Draw(short dst_x, short dst_y, short dst_w, short dst_h, short src_x, short src_y, short src_w, short src_h, const unsigned int color, const unsigned int player_color)
{
    if(texture == 0)
        GenerateTexture();
    if(texture == 0)
        return;

    if(src_w == 0)
        src_w = width;
    if(src_h == 0)
        src_h = height;
    if(dst_w == 0)
        dst_w = src_w;
    if(dst_h == 0)
        dst_h = src_h;

    struct GL_T2F_C4UB_V3F_Struct
    {
        GLfloat tx, ty;
        GLubyte r, g, b, a;
        GLfloat x, y, z;
    };

    GL_T2F_C4UB_V3F_Struct tmp[8];

    tmp[0].z = tmp[1].z = tmp[2].z = tmp[3].z = 0.0;

    int x = -nx + dst_x;
    int y = -ny + dst_y;

    tmp[0].x = tmp[1].x = GLfloat(x);
    tmp[2].x = tmp[3].x = GLfloat(x + dst_w);

    tmp[0].y = tmp[3].y = GLfloat(y);
    tmp[1].y = tmp[2].y = GLfloat(y + dst_h);

    tmp[0].tx = tmp[1].tx = (GLfloat)(src_x) / (GLfloat)tex_width / 2.0f;
    tmp[2].tx = tmp[3].tx = (GLfloat)(src_x + src_w) / (GLfloat)tex_width / 2.0f;

    tmp[0].ty = tmp[3].ty = (GLfloat)src_y / tex_height;
    tmp[1].ty = tmp[2].ty = (GLfloat)(src_y + src_h) / tex_height;

    tmp[4] = tmp[0];
    tmp[5] = tmp[1];
    tmp[6] = tmp[2];
    tmp[7] = tmp[3];

    tmp[4].tx += 0.5;
    tmp[5].tx += 0.5;
    tmp[6].tx += 0.5;
    tmp[7].tx += 0.5;

    tmp[0].r = tmp[1].r = tmp[2].r = tmp[3].r = GetRed(color);
    tmp[0].g = tmp[1].g = tmp[2].g = tmp[3].g = GetGreen(color);
    tmp[0].b = tmp[1].b = tmp[2].b = tmp[3].b = GetBlue(color);
    tmp[0].a = tmp[1].a = tmp[2].a = tmp[3].a = GetAlpha(color);

    tmp[4].r = tmp[5].r = tmp[6].r = tmp[7].r = GetRed(player_color);
    tmp[4].g = tmp[5].g = tmp[6].g = tmp[7].g = GetGreen(player_color);
    tmp[4].b = tmp[5].b = tmp[6].b = tmp[7].b = GetBlue(player_color);
    tmp[4].a = tmp[5].a = tmp[6].a = tmp[7].a = GetAlpha(player_color);

    glInterleavedArrays(GL_T2F_C4UB_V3F, 0, tmp);

    VideoDriverWrapper::inst().BindTexture(texture);

    glDrawArrays(GL_QUADS, 0, 8);
}

void glArchivItem_Bitmap_Player::GenerateTexture(void)
{
    texture = VideoDriverWrapper::inst().GenerateTexture();

    // Spezialpalette (blaue Spielerfarben sind Grau) verwenden,
    // damit man per OpenGL einfärben kann!
    setPalette(LOADER.GetPaletteN("colors"));

    int iformat = GL_RGBA, dformat = GL_BGRA; //GL_BGRA_EXT;

    unsigned char* buffer = new unsigned char[tex_width * 2 * tex_height * 4];

    VideoDriverWrapper::inst().BindTexture(texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);

    memset(buffer, 0, tex_width * 2 * tex_height * 4);
    print(buffer, tex_width * 2, tex_height, libsiedler2::FORMAT_RGBA, palette, 128, 0, 0, 0, 0, 0, 0, false);
    print(buffer, tex_width * 2, tex_height, libsiedler2::FORMAT_RGBA, palette, 128, tex_width, 0, 0, 0, 0, 0, true);
    glTexImage2D(GL_TEXTURE_2D, 0, iformat, tex_width * 2, tex_height, 0, dformat, GL_UNSIGNED_BYTE, buffer);

    delete[] buffer;
}
