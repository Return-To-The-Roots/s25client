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
#include "glTexturePacker.h"
#include "drivers/VideoDriverWrapper.h"
#include "ogl/glSmartBitmap.h"
#include "ogl/glTexturePackerNode.h"
#include "libsiedler2/PixelBufferARGB.h"
#include <algorithm>
#include <glad/glad.h>

static bool isSizeGreater(glSmartBitmap* a, glSmartBitmap* b)
{
    const Extent sizeA = a->getRequiredTexSize();
    const Extent sizeB = b->getRequiredTexSize();
    return (sizeA.x * sizeA.y) > (sizeB.x * sizeB.y);
}

glTexturePacker::~glTexturePacker()
{
    for(unsigned texture : textures)
        VIDEODRIVER.DeleteTexture(texture);
}

bool glTexturePacker::packHelper(std::vector<glSmartBitmap*>& list)
{
    unsigned texture = VIDEODRIVER.GenerateTexture();

    if(!texture)
        return false;

    textures.push_back(texture);

    VIDEODRIVER.BindTexture(texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // find space needed in total and biggest texture to store (as a start)
    Extent maxBmpSize(0, 0);
    unsigned total = 0;
    for(glSmartBitmap* bmp : list)
    {
        Extent texSize = bmp->getRequiredTexSize();
        maxBmpSize = elMax(maxBmpSize, texSize);

        total += texSize.x * texSize.y;
    }

    // most cards work much better with texture sizes of powers of two.
    maxBmpSize = VIDEODRIVER.calcPreferredTextureSize(maxBmpSize);

    int parTexWidth = 0;
    glTexImage2D(GL_PROXY_TEXTURE_2D, 0, GL_RGBA, maxBmpSize.x, maxBmpSize.y, 0, GL_BGRA, GL_UNSIGNED_BYTE, nullptr);
    glGetTexLevelParameteriv(GL_PROXY_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &parTexWidth);

    if(parTexWidth == 0)
        return false;

    // maximum texture size reached?
    bool maxTex = false;
    std::vector<glTexturePackerNode*> tmpVec;
    tmpVec.reserve(list.size());

    Extent curSize = maxBmpSize;
    do
    {
        // two possibilities: enough space OR maximum texture size reached
        if((curSize.x * curSize.y >= total) || maxTex)
        {
            // texture packer
            auto* root = new glTexturePackerNode(curSize);

            // list to store bitmaps we could not fit in our current texture
            std::vector<glSmartBitmap*> left;

            libsiedler2::PixelBufferARGB buffer(curSize.x, curSize.y);

            // try storing bitmaps in the big texture
            for(glSmartBitmap* bmp : list)
            {
                if(!root->insert(bmp, buffer, tmpVec))
                {
                    // inserting this bitmap failed? just remember it for next texture
                    left.push_back(bmp);
                } else
                {
                    // tell or glSmartBitmap, that it uses a shared texture (so it won't try to delete/free it)
                    bmp->setSharedTexture(texture);
                }
            }
            /*
            std::array<char, 100> tmp;
            snprintf(tmp, sizeof(tmp), "%u-%ux%u.rgba", texture, curSize.x, curSize.y);

            FILE *f = fopen(tmp, "w+");

            for (int y = 0; y < curSize.y; ++y)
            {
            for (int x = 0; x < curSize.x; ++x)
            {
            fputc(buffer[((y * curSize.x + x) << 2) + 2], f);
            fputc(buffer[((y * curSize.x + x) << 2) + 1], f);
            fputc(buffer[((y * curSize.x + x) << 2) + 0], f);
            fputc(buffer[((y * curSize.x + x) << 2) + 3], f);
            }
            }

            fclose(f);
            */
            // free texture packer, as it is not needed any more
            root->destroy(list.size());
            delete root;

            if(left.empty()) // nothing left, just generate texture and return success
            {
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, curSize.x, curSize.y, 0, GL_BGRA, GL_UNSIGNED_BYTE, buffer.getPixelPtr());
                glGetTexLevelParameteriv(GL_PROXY_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &parTexWidth);

                return parTexWidth != 0;
            } else if(maxTex) // maximum texture size reached and something still left
            {
                // generate this texture and release the buffer
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, curSize.x, curSize.y, 0, GL_BGRA, GL_UNSIGNED_BYTE, buffer.getPixelPtr());
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
        if(curSize.x <= curSize.y)
        {
            glTexImage2D(GL_PROXY_TEXTURE_2D, 0, GL_RGBA, curSize.x * 2, curSize.y, 0, GL_BGRA, GL_UNSIGNED_BYTE, nullptr);
            glGetTexLevelParameteriv(GL_PROXY_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &parTexWidth);

            if(parTexWidth == 0)
                maxTex = true;
            else
                curSize.x *= 2;
        } else
        {
            glTexImage2D(GL_PROXY_TEXTURE_2D, 0, GL_RGBA, curSize.x, curSize.y * 2, 0, GL_BGRA, GL_UNSIGNED_BYTE, nullptr);
            glGetTexLevelParameteriv(GL_PROXY_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &parTexWidth);

            if(parTexWidth == 0)
                maxTex = true;
            else
                curSize.y *= 2;
        }
    } while(true);
}

bool glTexturePacker::pack()
{
    std::sort(items.begin(), items.end(), isSizeGreater);

    if(packHelper(items))
        return true;

    // free all textures allocated by us
    for(unsigned tex : textures)
        VIDEODRIVER.DeleteTexture(tex);

    // reset glSmartBitmap textures
    for(glSmartBitmap* bmp : items)
        bmp->setSharedTexture(0);

    return false;
}
