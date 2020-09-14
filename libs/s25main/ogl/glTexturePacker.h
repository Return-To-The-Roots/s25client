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

#include "Point.h"
#include <vector>

class glSmartBitmap;

namespace libsiedler2 {
class PixelBufferBGRA;
}

class glTexture
{
    unsigned handle;
    Extent size;

public:
    glTexture();
    ~glTexture();
    glTexture(const glTexture&) = delete;
    glTexture(glTexture&&) noexcept;
    glTexture& operator=(const glTexture&) = delete;
    glTexture& operator=(glTexture&&) noexcept;

    explicit operator bool() const { return handle != 0; }
    const Extent& getSize() const { return size; }
    auto get() const { return handle; }

    void bind();
    bool checkSize(const Extent&);
    bool uploadData(const libsiedler2::PixelBufferBGRA&);
};

class glTexturePacker
{
private:
    std::vector<glTexture> textures;
    std::vector<glSmartBitmap*> items;

    bool packHelper(std::vector<glSmartBitmap*>& list);

public:
    bool pack();
    void add(glSmartBitmap& bmp) { items.push_back(&bmp); }
    const auto& getTextures() const { return textures; }
};
