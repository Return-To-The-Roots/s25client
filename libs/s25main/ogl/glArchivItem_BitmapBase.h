// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "DrawPoint.h"
#include "libsiedler2/ArchivItem_BitmapBase.h"

class glArchivItem_BitmapBase : public virtual libsiedler2::ArchivItem_BitmapBase //-V690
{
public:
    glArchivItem_BitmapBase();
    glArchivItem_BitmapBase(const glArchivItem_BitmapBase& item);
    ~glArchivItem_BitmapBase() override;

    /// liefert das GL-Textur-Handle.
    unsigned GetTexture();
    unsigned GetTextureNoCreate() const { return texture; }
    /// Löscht die GL-Textur (z.B fürs Neuerstellen)
    void DeleteTexture();
    /// Setzt den Texturfilter auf einen bestimmten Wert.
    virtual void setInterpolateTexture(bool interpolate);

    /// Return the "Null point"
    DrawPoint GetOrigin() const { return DrawPoint(nx_, ny_); }
    Extent GetSize() const { return Extent(getWidth(), getHeight()); }
    Extent GetTexSize() const;

private:
    /// Erzeugt die Textur.
    void GenerateTexture();

    unsigned texture;         /// Das GL-Textur-Handle
    Extent textureSize_;      /// The size of the texture. Only valid when texture exists
    bool interpolateTexture_; /// Whether the texture (color) should be interpolated or taken from the nearest pixel

protected:
    /// Returns the internal texure format
    int GetInternalFormat() const;
    /// Fill a just generated texture (glTexImage2D calls)
    virtual void FillTexture() = 0;
    /// Calculate the actual texture size
    virtual Extent CalcTextureSize() const = 0;
    /// Returns the currently set texture or 0 if none created
    unsigned GetTexNoCreate() const { return texture; }
};
