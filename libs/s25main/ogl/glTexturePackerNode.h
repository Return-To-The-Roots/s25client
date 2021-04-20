// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "Point.h"
#include <vector>

class glSmartBitmap;
namespace libsiedler2 {
class PixelBufferBGRA;
} // namespace libsiedler2

class glTexturePackerNode
{
    /// Position on the packed texture (can't be negative)
    Extent pos;
    /// Size of all the subnodes combined (makes up area covered)
    Extent size;

    glSmartBitmap* bmp;
    glTexturePackerNode* child[2];

public:
    glTexturePackerNode() : pos(0, 0), size(0, 0), bmp(nullptr) { child[0] = child[1] = nullptr; }
    glTexturePackerNode(const Extent& size) : pos(0, 0), size(size), bmp(nullptr) { child[0] = child[1] = nullptr; }
    /// Find a position in the buffer to draw the bitmap starting at this node
    /// todo list is cleared and used to avoid frequent allocations
    bool insert(glSmartBitmap* b, libsiedler2::PixelBufferBGRA& buffer, std::vector<glTexturePackerNode*>& todo);
    void destroy(unsigned reserve = 0);
};
