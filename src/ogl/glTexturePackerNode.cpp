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

#include "defines.h" // IWYU pragma: keep
#include "glTexturePackerNode.h"
#include "ogl/glSmartBitmap.h"

bool glTexturePackerNode::insert(glSmartBitmap* b, std::vector<uint32_t>& buffer, unsigned gw, unsigned gh, std::vector<glTexturePackerNode*>& todo)
{
    todo.clear();

    todo.push_back(this);

    int bw = b->getTexWidth();
    int bh = b->getTexHeight();

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
        if((bw > current->w) || (bh > current->h))
            continue;

        if((bw == current->w) && (bh == current->h))
        {
            current->bmp = b;

            b->drawTo(buffer, gw, gh, current->x, current->y);

            b->tmpTexData[0].tx = b->tmpTexData[1].tx = (float)current->x / (float)gw;
            b->tmpTexData[2].tx = b->tmpTexData[3].tx = b->isPlayer() ? (float)(current->x + current->w / 2) / (float)gw : (float)(current->x + current->w) / (float)gw;

            b->tmpTexData[0].ty = b->tmpTexData[3].ty = b->tmpTexData[4].ty = b->tmpTexData[7].ty = (float)current->y / (float)gh;
            b->tmpTexData[1].ty = b->tmpTexData[2].ty = b->tmpTexData[5].ty = b->tmpTexData[6].ty = (float)(current->y + current->h) / (float)gh;

            b->tmpTexData[4].tx = b->tmpTexData[5].tx = (float)(current->x + current->w / 2) / (float)gw;
            b->tmpTexData[6].tx = b->tmpTexData[7].tx = (float)(current->x + current->w) / (float)gw;

            return true;
        }

        current->child[0] = new glTexturePackerNode();
        current->child[1] = new glTexturePackerNode();

        int dw = current->w - bw;
        int dh = current->h - bh;

        if(dw > dh)
        {
            // split into left and right, put bitmap in left
            current->child[0]->x = current->x;
            current->child[1]->x = current->x + bw;
            current->child[0]->y = current->child[1]->y = current->y;
            current->child[0]->w = bw;
            current->child[1]->w = current->w - bw;
            current->child[0]->h = current->child[1]->h = current->h;
        } else
        {
            // split into top and bottom, put bitmap in top
            current->child[0]->x = current->child[1]->x = current->x;
            current->child[0]->y = current->y;
            current->child[1]->y = current->y + bh;
            current->child[0]->w = current->child[1]->w = current->w;
            current->child[0]->h = bh;
            current->child[1]->h = current->h - bh;
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
