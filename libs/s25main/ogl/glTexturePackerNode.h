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
