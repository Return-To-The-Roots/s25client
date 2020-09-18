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
