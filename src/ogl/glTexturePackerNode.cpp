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

#include "rttrDefines.h" // IWYU pragma: keep
#include "glTexturePackerNode.h"
#include "ogl/glSmartBitmap.h"
#include "libsiedler2/PixelBufferARGB.h"

bool glTexturePackerNode::insert(glSmartBitmap* b, libsiedler2::PixelBufferARGB& buffer, std::vector<glTexturePackerNode*>& todo)
{
    todo.clear();

    todo.push_back(this);

    const Extent texSize = b->getTexSize();

    while(!todo.empty())
    {
        glTexturePackerNode* current = todo.back();
        todo.pop_back();

        if(current->child[0])
        {
            todo.push_back(current->child[0]);
            todo.push_back(current->child[1]);
            continue;
        }

        // we are a leaf and do already contain an image
        if(current->bmp)
            continue;

        // no space left for this item
        if((texSize.x > current->size.x) || (texSize.y > current->size.y))
            continue;

        if(texSize == current->size)
        {
            current->bmp = b;

            b->drawTo(buffer, current->pos);

            b->texCoords[0].x = b->texCoords[1].x = (float)current->pos.x / (float)buffer.getWidth();
            b->texCoords[2].x = b->texCoords[3].x = b->isPlayer() ?
                                                      (float)(current->pos.x + current->size.x / 2) / (float)buffer.getWidth() :
                                                      (float)(current->pos.x + current->size.x) / (float)buffer.getWidth();

            b->texCoords[0].y = b->texCoords[3].y = b->texCoords[4].y = b->texCoords[7].y =
              (float)current->pos.y / (float)buffer.getHeight();
            b->texCoords[1].y = b->texCoords[2].y = b->texCoords[5].y = b->texCoords[6].y =
              (float)(current->pos.y + current->size.y) / (float)buffer.getHeight();

            b->texCoords[4].x = b->texCoords[5].x = (float)(current->pos.x + current->size.x / 2) / (float)buffer.getWidth();
            b->texCoords[6].x = b->texCoords[7].x = (float)(current->pos.x + current->size.x) / (float)buffer.getWidth();

            return true;
        }

        current->child[0] = new glTexturePackerNode();
        current->child[1] = new glTexturePackerNode();

        Extent deltaSize = current->size - texSize;

        if(deltaSize.x > deltaSize.y)
        {
            // split into left and right, put bitmap in left
            current->child[0]->pos = current->child[1]->pos = current->pos;
            current->child[1]->pos.x += texSize.x;
            current->child[0]->size.x = texSize.x;
            current->child[1]->size.x = current->size.x - texSize.x;
            current->child[0]->size.y = current->child[1]->size.y = current->size.y;
        } else
        {
            // split into top and bottom, put bitmap in top
            current->child[0]->pos = current->child[1]->pos = current->pos;
            current->child[1]->pos.y += texSize.y;
            current->child[0]->size.x = current->child[1]->size.x = current->size.x;
            current->child[0]->size.y = texSize.y;
            current->child[1]->size.y = current->size.y - texSize.y;
        }

        todo.push_back(current->child[0]);
    }

    return false;
}

void glTexturePackerNode::destroy(unsigned reserve)
{
    std::vector<glTexturePackerNode*> todo;

    todo.reserve(reserve);

    if(child[0])
    {
        todo.push_back(child[0]);
        todo.push_back(child[1]);
    }

    while(!todo.empty())
    {
        glTexturePackerNode* current = todo.back();
        todo.pop_back();

        if(current->child[0])
        {
            todo.push_back(current->child[0]);
            todo.push_back(current->child[1]);
        }

        delete current;
    }
}
