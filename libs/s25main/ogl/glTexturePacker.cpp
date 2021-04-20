// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "glTexturePacker.h"
#include "drivers/VideoDriverWrapper.h"
#include "ogl/glSmartBitmap.h"
#include "ogl/glTexturePackerNode.h"
#include "ogl/saveBitmap.h"
#include "libsiedler2/PixelBufferBGRA.h"
#include <glad/glad.h>
#include <algorithm>
#include <memory>
#include <utility>

static bool isSizeGreater(glSmartBitmap* a, glSmartBitmap* b)
{
    const Extent sizeA = a->getRequiredTexSize();
    const Extent sizeB = b->getRequiredTexSize();
    return (sizeA.x * sizeA.y) > (sizeB.x * sizeB.y);
}

bool glTexturePacker::packHelper(std::vector<glSmartBitmap*>& list)
{
    glTexture texture;

    if(!texture)
        return false;

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

    if(!texture.checkSize(maxBmpSize))
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
            auto root = std::make_unique<glTexturePackerNode>(curSize);

            // list to store bitmaps we could not fit in our current texture
            std::vector<glSmartBitmap*> left;

            libsiedler2::PixelBufferBGRA buffer(curSize.x, curSize.y);

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
                    bmp->setSharedTexture(texture.get());
                }
            }
            if((false))
            {
                bfs::path outFilepath = std::to_string(texture.get()) + "-" + std::to_string(curSize.x) + "x"
                                        + std::to_string(curSize.y) + ".bmp";
                saveBitmap(buffer, outFilepath);
            }
            // free texture packer, as it is not needed any more
            root->destroy(list.size());
            root.reset();

            if(!texture.uploadData(buffer))
                return false;

            if(left.empty()) // nothing left, just generate texture and return success
            {
                textures.emplace_back(std::move(texture));
                return true;
            } else if(maxTex) // maximum texture size reached and something still left
            {
                textures.emplace_back(std::move(texture));
                // recursively generate textures for what is left
                return packHelper(left);
            }

            // our pre-estimated size if the big texture was not enough for the algorithm to fit all textures in
            // delete buffer and try again with an increased big texture
            left.clear();
        }

        // increase width or height, try whether opengl is able to handle textures that big
        const auto newSize =
          (curSize.x <= curSize.y) ? Extent(curSize.x * 2, curSize.y) : Extent(curSize.x, curSize.y * 2);
        if(!texture.checkSize(newSize))
            maxTex = true;
        else
            curSize = newSize;
    } while(true);
}

bool glTexturePacker::pack()
{
    std::sort(items.begin(), items.end(), isSizeGreater);

    if(packHelper(items))
        return true;

    // reset glSmartBitmap textures
    for(glSmartBitmap* bmp : items)
        bmp->setSharedTexture(0);

    textures.clear();

    return false;
}

glTexture::glTexture() : handle(VIDEODRIVER.GenerateTexture()), size(0, 0)
{
    if(!handle)
        return;
    bind();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
}

glTexture::glTexture(glTexture&& rhs) noexcept : handle(rhs.handle), size(rhs.size)
{
    rhs.handle = 0;
}
glTexture& glTexture::operator=(glTexture&& rhs) noexcept
{
    std::swap(handle, rhs.handle);
    std::swap(size, rhs.size);
    return *this;
}

void glTexture::bind() const
{
    VIDEODRIVER.BindTexture(handle);
}

glTexture::~glTexture()
{
    if(handle)
        VIDEODRIVER.DeleteTexture(handle);
}

bool glTexture::checkSize(const Extent& size) const
{
    if(!handle)
        return false;
    VIDEODRIVER.BindTexture(handle);
    glTexImage2D(GL_PROXY_TEXTURE_2D, 0, GL_RGBA, size.x, size.y, 0, GL_BGRA, GL_UNSIGNED_BYTE, nullptr);
    int resultWidth;
    glGetTexLevelParameteriv(GL_PROXY_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &resultWidth);

    return resultWidth > 0;
}

bool glTexture::uploadData(const libsiedler2::PixelBufferBGRA& buffer)
{
    if(!handle)
        return false;
    VIDEODRIVER.BindTexture(handle);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, buffer.getWidth(), buffer.getHeight(), 0, GL_BGRA, GL_UNSIGNED_BYTE,
                 buffer.getPixelPtr());
    size = Extent(buffer.getWidth(), buffer.getHeight());
    int resultWidth;
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &resultWidth);
    return resultWidth > 0;
}
