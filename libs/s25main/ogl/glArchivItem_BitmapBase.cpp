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

#include "glArchivItem_BitmapBase.h"
#include "Loader.h"
#include "drivers/VideoDriverWrapper.h"
#include <glad/glad.h>

/** @class glArchivItem_BitmapBase
 *
 *  Basisklasse für GL-Bitmapitems.
 */

/** @var glArchivItem_BitmapBase::texture
 *
 *  OpenGL-Textur des Bildes.
 */

glArchivItem_BitmapBase::glArchivItem_BitmapBase() : texture(0), textureSize_(0, 0), interpolateTexture_(true) {}

glArchivItem_BitmapBase::glArchivItem_BitmapBase(const glArchivItem_BitmapBase& item)
    : ArchivItem_BitmapBase(item), texture(0), textureSize_(item.textureSize_), interpolateTexture_(item.interpolateTexture_)
{}

glArchivItem_BitmapBase::~glArchivItem_BitmapBase()
{
    DeleteTexture();
}

/**
 *  Liefert das GL-Textur-Handle.
 */
unsigned glArchivItem_BitmapBase::GetTexture()
{
    if(texture == 0)
        GenerateTexture();
    return texture;
}

/**
 *  Löscht die GL-Textur (z.B fürs Neuerstellen)
 */
void glArchivItem_BitmapBase::DeleteTexture()
{
    VIDEODRIVER.DeleteTexture(texture);
    texture = 0;
}

void glArchivItem_BitmapBase::setInterpolateTexture(bool interpolate)
{
    if(interpolateTexture_ == interpolate)
        return;

    interpolateTexture_ = interpolate;

    // neugenerierung der Textur anstoßen
    if(texture != 0)
        DeleteTexture();
}

Extent glArchivItem_BitmapBase::GetTexSize() const
{
    RTTR_Assert(texture); // Invalid if no texture exists
    return textureSize_;
}

/**
 *  Erzeugt die Textur.
 */
void glArchivItem_BitmapBase::GenerateTexture()
{
    textureSize_ = CalcTextureSize();
    if(textureSize_.x == 0 || textureSize_.y == 0)
        return;

    texture = VIDEODRIVER.GenerateTexture();
    if(!texture)
        return;

    if(!getPalette() && getFormat() == libsiedler2::TextureFormat::Paletted)
        setPaletteCopy(*LOADER.GetPaletteN("pal5"));

    VIDEODRIVER.BindTexture(texture);

    GLint filter = interpolateTexture_ ? GL_NEAREST : GL_LINEAR;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);

    FillTexture();
}

int glArchivItem_BitmapBase::GetInternalFormat() const
{
    return GL_RGBA;
}
