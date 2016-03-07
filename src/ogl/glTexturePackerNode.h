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

#ifndef glTexturePackerNode_h__
#define glTexturePackerNode_h__

#include <vector>
#include <stdint.h>

class glSmartBitmap;
class glTexturePackerNode;

class glTexturePackerNode
{
    int x, y;
    int w, h;

    glSmartBitmap* bmp;
    glTexturePackerNode* child[2];

public:
    glTexturePackerNode(): x(0), y(0), w(0), h(0), bmp(NULL) { child[0] = child[1] = NULL; }
    glTexturePackerNode(int w, int h): x(0), y(0), w(w), h(h), bmp(NULL) { child[0] = child[1] = NULL; }

    bool insert(glSmartBitmap* b, std::vector<uint32_t>& buffer, unsigned gw, unsigned gh, std::vector<glTexturePackerNode*>& todo);
    void destroy(unsigned reserve = 0);
};

#endif // glTexturePackerNode_h__
