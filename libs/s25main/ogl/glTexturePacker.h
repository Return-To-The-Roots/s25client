// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

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

    void bind() const;
    bool checkSize(const Extent&) const;
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
