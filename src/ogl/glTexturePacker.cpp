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
#include "glTexturePacker.h"
#include "ogl/glSmartBitmap.h"
#include "ogl/glTexturePackerNode.h"
#include "drivers/VideoDriverWrapper.h"
#include "oglIncludes.h"
#include <algorithm>

glTexturePacker::~glTexturePacker()
{
    for(std::vector<unsigned>::const_iterator it = textures.begin(); it != textures.end(); ++it)
        VIDEODRIVER.DeleteTexture((*it));
}

bool glTexturePacker::sortSmartBitmap(glSmartBitmap* a, glSmartBitmap* b)
{
    return (a->getTexWidth() * a->getTexHeight()) > (b->getTexWidth() * b->getTexHeight());
}

bool glTexturePacker::packHelper(std::vector<glSmartBitmap*> &list)
{
    unsigned texture = VIDEODRIVER.GenerateTexture();

    if(!texture)
        return false;

    textures.push_back(texture);

    VIDEODRIVER.BindTexture(texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // find space needed in total and biggest texture to store (as a start)
    int w = 0;
    int h = 0;
    int total = 0;
    for(std::vector<glSmartBitmap*>::const_iterator it = list.begin(); it != list.end(); ++it)
    {
        if((*it)->getTexWidth() > w)
            w = (*it)->getTexWidth();

        if((*it)->getTexHeight() > h)
            h = (*it)->getTexHeight();

        total += (*it)->getTexWidth() * (*it)->getTexHeight();
    }

    // most cards work much better with texture sizes of powers of two.
    w = glSmartBitmap::nextPowerOfTwo(w);
    h = glSmartBitmap::nextPowerOfTwo(h);

    int parTexWidth = 0;
    glTexImage2D(GL_PROXY_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_BGRA, GL_UNSIGNED_BYTE, NULL);
    glGetTexLevelParameteriv(GL_PROXY_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &parTexWidth);

    if(parTexWidth == 0)
        return false;

    // maximum texture size reached?
    bool maxTex = false;
    std::vector<glTexturePackerNode*> tmpVec;
    tmpVec.reserve(list.size());

    do
    {
        // two possibilities: enough space OR maximum texture size reached
        if((w* h >= total) || maxTex)
        {
            // texture packer
            glTexturePackerNode* root = new glTexturePackerNode(w, h);

            // list to store bitmaps we could not fit in our current texture
            std::vector<glSmartBitmap*> left;

            std::vector<uint32_t> buffer(w * h);

            // try storing bitmaps in the big texture
            for(std::vector<glSmartBitmap*>::const_iterator it = list.begin(); it != list.end(); ++it)
            {
                if(!root->insert((*it), buffer, w, h, tmpVec))
                {
                    // inserting this bitmap failed? just remember it for next texture
                    left.push_back((*it));
                } else
                {
                    // tell or glSmartBitmap, that it uses a shared texture (so it won't try to delete/free it)
                    (*it)->setSharedTexture(texture);
                }
            }
            /*
            char tmp[100];
            snprintf(tmp, sizeof(tmp), "%u-%ux%u.rgba", texture, w, h);

            FILE *f = fopen(tmp, "w+");

            for (int y = 0; y < h; ++y)
            {
            for (int x = 0; x < w; ++x)
            {
            fputc(buffer[((y * w + x) << 2) + 2], f);
            fputc(buffer[((y * w + x) << 2) + 1], f);
            fputc(buffer[((y * w + x) << 2) + 0], f);
            fputc(buffer[((y * w + x) << 2) + 3], f);
            }
            }

            fclose(f);
            */
            // free texture packer, as it is not needed any more
            root->destroy(list.size());
            delete root;

            if(left.empty())   // nothing left, just generate texture and return success
            {
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_BGRA, GL_UNSIGNED_BYTE, &buffer.front());
                glGetTexLevelParameteriv(GL_PROXY_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &parTexWidth);

                return parTexWidth != 0;
            } else if(maxTex)    // maximum texture size reached and something still left
            {
                // generate this texture and release the buffer
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_BGRA, GL_UNSIGNED_BYTE, &buffer.front());
                glGetTexLevelParameteriv(GL_PROXY_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &parTexWidth);

                if(parTexWidth == 0)
                    return false;

                // recursively generate textures for what is left
                return packHelper(left);
            }

            // our pre-estimated size if the big texture was not enough for the algorithm to fit all textures in
            // delete buffer and try again with an increased big texture
            left.clear();
        }

        // increase width or height, try whether opengl is able to handle textures that big
        if(w <= h)
        {
            glTexImage2D(GL_PROXY_TEXTURE_2D, 0, GL_RGBA, w * 2, h, 0, GL_BGRA, GL_UNSIGNED_BYTE, NULL);
            glGetTexLevelParameteriv(GL_PROXY_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &parTexWidth);

            if(parTexWidth == 0)
                maxTex = true;
            else
                w *= 2;
        } else if(h < w)
        {
            glTexImage2D(GL_PROXY_TEXTURE_2D, 0, GL_RGBA, w, h * 2, 0, GL_BGRA, GL_UNSIGNED_BYTE, NULL);
            glGetTexLevelParameteriv(GL_PROXY_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &parTexWidth);

            if(parTexWidth == 0)
                maxTex = true;
            else
                h *= 2;
        }
    } while(true);
}

bool glTexturePacker::pack()
{
    for(std::vector<glSmartBitmap*>::const_iterator it = items.begin(); it != items.end(); ++it)
        (*it)->calcDimensions();

    std::sort(items.begin(), items.end(), sortSmartBitmap);

    if(packHelper(items))
        return true;

    // free all textures allocated by us
    for(std::vector<unsigned>::const_iterator it = textures.begin(); it != textures.end(); ++it)
        VIDEODRIVER.DeleteTexture((*it));

    // reset glSmartBitmap textures
    for(std::vector<glSmartBitmap*>::const_iterator it = items.begin(); it != items.end(); ++it)
        (*it)->setSharedTexture(0);

    return false;
}
