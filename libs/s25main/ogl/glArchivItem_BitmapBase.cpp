// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

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
    : ArchivItem_BitmapBase(item), texture(0), textureSize_(item.textureSize_),
      interpolateTexture_(item.interpolateTexture_)
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
