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
#include "defines.h" // IWYU pragma: keep
#include "glArchivItem_Bitmap_Direct.h"
#include "drivers/VideoDriverWrapper.h"
#include "ArchivItem_Palette.h"
#include "ogl/oglIncludes.h"

// Include last!
#include "DebugNew.h" // IWYU pragma: keep

glArchivItem_Bitmap_Direct::glArchivItem_Bitmap_Direct()
{
}

glArchivItem_Bitmap_Direct::glArchivItem_Bitmap_Direct(const glArchivItem_Bitmap_Direct& item)
    : ArchivItem_BitmapBase(item), baseArchivItem_Bitmap(item), glArchivItem_Bitmap(item)
{
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  setzt einen Pixel auf einen bestimmten Wert.
 *
 *  @param[in] x       X Koordinate des Pixels
 *  @param[in] y       Y Koordinate des Pixels
 *  @param[in] color   Farbe des Pixels
 *  @param[in] palette Grundpalette
 *
 *  @author FloSoft
 */
void glArchivItem_Bitmap_Direct::tex_setPixel(unsigned short x, unsigned short y, unsigned char color, const libsiedler2::ArchivItem_Palette* palette)
{
    // Pixel in Puffer setzen
    libsiedler2::baseArchivItem_Bitmap::tex_setPixel(x, y, color, palette);

    // Ist eine GL-Textur bereits erzeugt? Wenn ja, Pixel in Textur austauschen
    if(GetTexNoCreate() != 0)
    {
        if(x < tex_width_ && y < tex_height_)
        {
            struct{
                libsiedler2::Color clr;
                unsigned char a;
            } clr;

            if(color == libsiedler2::TRANSPARENT_INDEX)
                clr.a = 0x00;
            else
            {
                clr.clr = (*this->palette_)[color];
                clr.a = 0xFF;
            }

            VIDEODRIVER.BindTexture(GetTexNoCreate());
            glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, &clr);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  setzt einen Pixel auf einen bestimmten Wert.
 *
 *  @param[in] x X Koordinate des Pixels
 *  @param[in] y Y Koordinate des Pixels
 *  @param[in] r Roter Wert
 *  @param[in] g Grüner Wert
 *  @param[in] b Blauer Wert
 *  @param[in] a Alpha Wert (bei paletted nur 0xFF/0x00 unterstützt)
 *
 *  @author FloSoft
 */
void glArchivItem_Bitmap_Direct::tex_setPixel(unsigned short x, unsigned short y, unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
    // Pixel in Puffer setzen
    libsiedler2::baseArchivItem_Bitmap::tex_setPixel(x, y, r, g, b, a);

    // Ist ein GL-Textur bereits erzeugt? Wenn ja, Pixel in Textur austauschen
    if(GetTexNoCreate() != 0)
    {
        if(x < tex_width_ && y < tex_height_)
        {
            unsigned char buffer[4] = { r, g, b, a };

            VIDEODRIVER.BindTexture(GetTexNoCreate());
            glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, &buffer);
        }
    }
}

/// liefert die Farbwerte eines Pixels als uc-Array: {r,g,b,a}
unsigned char* glArchivItem_Bitmap_Direct::tex_getPixel(const unsigned short x, const unsigned short y)
{
    return &tex_data_[y * tex_width_ + x];
}

