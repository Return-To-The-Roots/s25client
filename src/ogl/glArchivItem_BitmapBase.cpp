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
#include "glArchivItem_BitmapBase.h"
#include "drivers/VideoDriverWrapper.h"
#include "Loader.h"

// Include last!
#include "DebugNew.h" // IWYU pragma: keep

///////////////////////////////////////////////////////////////////////////////
/** @class glArchivItem_BitmapBase
 *
 *  Basisklasse für GL-Bitmapitems.
 *
 *  @author FloSoft
 */

///////////////////////////////////////////////////////////////////////////////
/** @var glArchivItem_BitmapBase::texture
 *
 *  OpenGL-Textur des Bildes.
 *
 *  @author FloSoft
 */

glArchivItem_BitmapBase::glArchivItem_BitmapBase()
    : texture(0), filter(GL_NEAREST)
{
}

glArchivItem_BitmapBase::glArchivItem_BitmapBase(const glArchivItem_BitmapBase& item)
    : ArchivItem_BitmapBase(item), texture(0), filter(item.filter)
{
}

glArchivItem_BitmapBase::~glArchivItem_BitmapBase()
{
    DeleteTexture();
}

glArchivItem_BitmapBase& glArchivItem_BitmapBase::operator=(const glArchivItem_BitmapBase& item)
{
    if(this == &item)
        return *this;
    ArchivItem_BitmapBase::operator=(item);
    texture = 0;
    filter = item.filter;
    return *this;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Liefert das GL-Textur-Handle.
 *
 *  @author FloSoft
 */
unsigned int glArchivItem_BitmapBase::GetTexture()
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
void glArchivItem_BitmapBase::DeleteTexture()
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
void glArchivItem_BitmapBase::setFilter(unsigned int filter)
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
void glArchivItem_BitmapBase::GenerateTexture()
{
    if(tex_width_ == 0 || tex_height_ == 0)
        return;

    texture = VIDEODRIVER.GenerateTexture();

    if(!palette_)
        setPalette(LOADER.GetPaletteN("pal5"));

    VIDEODRIVER.BindTexture(texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);

    FillTexture();
}

int glArchivItem_BitmapBase::GetInternalFormat() const
{
    return GL_RGBA;
}
