// $Id: glBitmap.cpp 8155 2012-09-06 02:11:55Z Maqs $
//
// Copyright (c) 2005 - 2012 Settlers Freaks (sf-team at siedler25.org)
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

///////////////////////////////////////////////////////////////////////////////
// Header
#include "main.h"
#include "glSmartBitmap.h"
#include "VideoDriverWrapper.h"
#include "Settings.h"

#include "Loader.h"

#include <climits>
#include <list>
#include <cstdio>

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#if defined _WIN32 && defined _DEBUG && defined _MSC_VER
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

bool glSmartTexturePackerNode::insert(glSmartBitmap* b, unsigned char* buffer, unsigned gw, unsigned gh, unsigned reserve)
{
    std::vector<glSmartTexturePackerNode*> todo;

    todo.reserve(reserve);

    todo.push_back(this);

    int bw = b->getTexWidth();
    int bh = b->getTexHeight();

    while (!todo.empty())
    {
        glSmartTexturePackerNode* current = todo.back();
        todo.pop_back();

        if (current->child[0] != NULL)
        {
            todo.push_back(current->child[0]);
            todo.push_back(current->child[1]);
            continue;
        }

        // we are a leaf and do already contain an image
        if (current->bmp != NULL)
        {
            continue;
        }

        // no space left for this item
        if ((bw > current->w) || (bh > current->h))
        {
            continue;
        }

        if ((bw == current->w) && (bh == current->h))
        {
            current->bmp = b;

            b->drawTo(buffer, gw, gh, current->x, current->y);

            b->tmp[0].tx = b->tmp[1].tx = (float) current->x / (float) gw;
            b->tmp[2].tx = b->tmp[3].tx = b->isPlayer() ? (float) (current->x + current->w / 2) / (float) gw : (float) (current->x + current->w) / (float) gw;

            b->tmp[0].ty = b->tmp[3].ty = b->tmp[4].ty = b->tmp[7].ty = (float) current->y / (float) gh;
            b->tmp[1].ty = b->tmp[2].ty = b->tmp[5].ty = b->tmp[6].ty = (float) (current->y + current->h) / (float) gh;

            b->tmp[4].tx = b->tmp[5].tx = (float) (current->x + current->w / 2) / (float) gw;
            b->tmp[6].tx = b->tmp[7].tx = (float) (current->x + current->w) / (float) gw;

            return(true);
        }

        current->child[0] = new glSmartTexturePackerNode();
        current->child[1] = new glSmartTexturePackerNode();

        int dw = current->w - bw;
        int dh = current->h - bh;

        if (dw > dh)
        {
            // split into left and right, put bitmap in left
            current->child[0]->x = current->x;
            current->child[1]->x = current->x + bw;
            current->child[0]->y = current->child[1]->y = current->y;
            current->child[0]->w = bw;
            current->child[1]->w = current->w - bw;
            current->child[0]->h = current->child[1]->h = current->h;
        }
        else
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

    return(false);
}

void glSmartTexturePackerNode::destroy(unsigned reserve)
{
    std::vector<glSmartTexturePackerNode*> todo;

    todo.reserve(reserve);

    if (child[0])
    {
        todo.push_back(child[0]);
        todo.push_back(child[1]);
    }

    while (!todo.empty())
    {
        glSmartTexturePackerNode* current = todo.back();
        todo.pop_back();

        if (current->child[0])
        {
            todo.push_back(current->child[0]);
            todo.push_back(current->child[1]);
        }

        delete current;
    }
}

glSmartTexturePacker::~glSmartTexturePacker()
{
    for (std::vector<unsigned>::const_iterator it = textures.begin(); it != textures.end(); ++it)
    {
        VideoDriverWrapper::inst().DeleteTexture((*it));
    }
}

bool glSmartTexturePacker::sortSmartBitmap(glSmartBitmap* a, glSmartBitmap* b)
{
    return((a->getTexWidth() * a->getTexHeight()) > (b->getTexWidth() * b->getTexHeight()));
}

bool glSmartTexturePacker::packHelper(std::vector<glSmartBitmap*> &list)
{
    int w = 0;
    int h = 0;
    int tmp = 0;
    int total = 0;

    unsigned texture;

    texture = VideoDriverWrapper::inst().GenerateTexture();

    if (!texture)
    {
        return(false);
    }

    textures.push_back(texture);

    VideoDriverWrapper::inst().BindTexture(texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // find space needed in total and biggest texture to store (as a start)
    for (std::vector<glSmartBitmap*>::const_iterator it = list.begin(); it != list.end(); ++it)
    {
        if ((*it)->getTexWidth() > w)
        {
            w = (*it)->getTexWidth();
        }

        if ((*it)->getTexHeight() > h)
        {
            h = (*it)->getTexHeight();
        }

        total += (*it)->getTexWidth() * (*it)->getTexHeight();
    }

    // most cards work much better with texture sizes of powers of two.
    w = glSmartBitmap::nextPowerOfTwo(w);
    h = glSmartBitmap::nextPowerOfTwo(h);

    glTexImage2D(GL_PROXY_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_BGRA, GL_UNSIGNED_BYTE, NULL);


    glGetTexLevelParameteriv(GL_PROXY_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &tmp);

    if (tmp == 0)
    {
        return(false);
    }

    // maximum texture size reached?
    bool maxTex = false;

    do
    {
        // two possibilities: enough space OR maximum texture size reached
        if ((w* h >= total) || maxTex)
        {
            // texture packer
            glSmartTexturePackerNode* root = new glSmartTexturePackerNode(w, h);

            // list to store bitmaps we could not fit in our current texture
            std::vector<glSmartBitmap*> left;

            unsigned char* buffer = new unsigned char[w * h * 4];

            if (buffer == NULL)
            {
                return(false);
            }

            memset(buffer, 0, w * h * 4);

            // try storing bitmaps in the big texture
            for (std::vector<glSmartBitmap*>::const_iterator it = list.begin(); it != list.end(); ++it)
            {
                if (!root->insert((*it), buffer, w, h, list.size()))
                {
                    // inserting this bitmap failed? just remember it for next texture
                    left.push_back((*it));
                }
                else
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

            if (left.empty())   // nothing left, just generate texture and return success
            {
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_BGRA, GL_UNSIGNED_BYTE, buffer);

                delete[] buffer;

                glGetTexLevelParameteriv(GL_PROXY_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &tmp);

                if (tmp == 0)
                {
                    return(false);
                }

                return(true);
            }
            else if (maxTex)    // maximum texture size reached and something still left
            {
                // generate this texture and release the buffer
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_BGRA, GL_UNSIGNED_BYTE, buffer);

                delete[] buffer;

                glGetTexLevelParameteriv(GL_PROXY_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &tmp);

                if (tmp == 0)
                {
                    return(false);
                }

                // recursively generate textures for what is left
                return(packHelper(left));
            }

            // our pre-estimated size if the big texture was not enough for the algorithm to fit all textures in
            // delete buffer and try again with an increased big texture
            left.clear();

            delete[] buffer;
        }

        // increase width or height, try whether opengl is able to handle textures that big
        if (w <= h)
        {
            glTexImage2D(GL_PROXY_TEXTURE_2D, 0, GL_RGBA, w << 1, h, 0, GL_BGRA, GL_UNSIGNED_BYTE, NULL);

            glGetTexLevelParameteriv(GL_PROXY_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &tmp);

            if (tmp == 0)
            {
                maxTex = true;
            }
            else
            {
                w <<= 1;
            }
        }
        else if (h < w)
        {
            glTexImage2D(GL_PROXY_TEXTURE_2D, 0, GL_RGBA, w, h << 1, 0, GL_BGRA, GL_UNSIGNED_BYTE, NULL);

            glGetTexLevelParameteriv(GL_PROXY_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &tmp);

            if (tmp == 0)
            {
                maxTex = true;
            }
            else
            {
                h <<= 1;
            }
        }
    }
    while (0 == 0);
}

bool glSmartTexturePacker::pack()
{
    for (std::vector<glSmartBitmap*>::const_iterator it = items.begin(); it != items.end(); ++it)
    {
        (*it)->calcDimensions();
    }

    std::sort(items.begin(), items.end(), sortSmartBitmap);

    if (packHelper(items))
    {
        return(true);
    }

    // free all textures allocated by us
    for (std::vector<unsigned>::const_iterator it = textures.begin(); it != textures.end(); ++it)
    {
        VideoDriverWrapper::inst().DeleteTexture((*it));
    }

    // reset glSmartBitmap textures
    for (std::vector<glSmartBitmap*>::const_iterator it = items.begin(); it != items.end(); ++it)
    {
        (*it)->setSharedTexture(0);
    }

    return(false);
}

void glSmartBitmap::reset()
{
    if (texture && !sharedTexture)
    {
        VideoDriverWrapper::inst().DeleteTexture(texture);
    }

    items.clear();

    texture = 0;
}

glSmartBitmap::~glSmartBitmap()
{
    reset();
}

unsigned glSmartBitmap::nextPowerOfTwo(unsigned k)
{
    if (k == 0)
    {
        return(1);
    }

    k--;

    for (unsigned i = 1; i < sizeof(unsigned)*CHAR_BIT; i <<= 1)
    {
        k = k | k >> i;
    }

    return(k + 1);
}

void glSmartBitmap::calcDimensions()
{
    if (items.empty())
    {
        nx = ny = 0;
        w = h = 0;
        return;
    }

    int max_x = 0;
    int max_y = 0;

    nx = ny = INT_MIN;

    hasPlayer = false;

    for (std::vector<glBitmapItem>::const_iterator it = items.begin(); it != items.end(); ++it)
    {
        if ((*it).type == TYPE_ARCHIVITEM_BITMAP_PLAYER)
        {
            hasPlayer = true;
        }

        if (nx < (*it).nx)
        {
            nx = (*it).nx;
        }

        if (ny < (*it).ny)
        {
            ny = (*it).ny;
        }

        if (max_x < (*it).w - (*it).nx)
        {
            max_x = (*it).w - (*it).nx;
        }

        if (max_y < (*it).h - (*it).ny)
        {
            max_y = (*it).h - (*it).ny;
        }
    }

    w = nx + max_x;
    h = ny + max_y;
}

void glSmartBitmap::drawTo(unsigned char* buffer, unsigned stride, unsigned height, int x_offset, int y_offset)
{
    libsiedler2::ArchivItem_Palette* p_colors = LOADER.GetPaletteN("colors");
    libsiedler2::ArchivItem_Palette* p_5 = LOADER.GetPaletteN("pal5");

    for (std::vector<glBitmapItem>::const_iterator it = items.begin(); it != items.end(); ++it)
    {
        if (((*it).w == 0) || ((*it).h == 0))
        {
            continue;
        }

        unsigned xo = (nx - (*it).nx);
        unsigned yo = (ny - (*it).ny);

        switch ((*it).type)
        {
            case TYPE_ARCHIVITEM_BITMAP:
            {
                (*it).bmp->print(buffer, stride, height, libsiedler2::FORMAT_RGBA, p_5, xo + x_offset, yo + y_offset, (*it).x, (*it).y, (*it).w, (*it).h);

                break;
            }
            case TYPE_ARCHIVITEM_BITMAP_PLAYER:
            {
                libsiedler2::baseArchivItem_Bitmap_Player* bmp = dynamic_cast<libsiedler2::baseArchivItem_Bitmap_Player*>((*it).bmp);

                bmp->print(buffer, stride, height, libsiedler2::FORMAT_RGBA, p_colors, 128,
                           xo + x_offset, yo + y_offset, (*it).x, (*it).y, (*it).w, (*it).h, false);

                bmp->print(buffer, stride, height, libsiedler2::FORMAT_RGBA, p_colors, 128,
                           xo + w + x_offset, yo + y_offset, (*it).x, (*it).y, (*it).w, (*it).h, true);

                break;
            }
            case TYPE_ARCHIVITEM_BITMAP_SHADOW:
            {
                unsigned char* tmp = new unsigned char[w * h * 4];
                memset(tmp, 0, w * h * 4);

                (*it).bmp->print(tmp, w, h, libsiedler2::FORMAT_RGBA, p_5, xo, yo, (*it).x, (*it).y, (*it).w, (*it).h);

                unsigned tmpIdx = 0;

                for (int y = 0; y < h; ++y)
                {
                    unsigned idx = ((y_offset + y) * stride + x_offset) << 2;

                    for (int x = 0; x < w; ++x)
                    {
                        if (tmp[tmpIdx + 3] != 0x00)
                        {
                            if (buffer[idx + 3] == 0x00)
                            {
                                buffer[idx] = 0x00;
                                buffer[idx + 1] = 0x00;
                                buffer[idx + 2] = 0x00;
                                buffer[idx + 3] = 0x40;
                            }
                        }

                        idx += 4;
                        tmpIdx += 4;
                    }
                }

                delete[] tmp;

                break;
            }
            default:
                break;
        }
    }
}

void glSmartBitmap::generateTexture()
{
    if (items.empty())
    {
        return;
    }

    if (!texture)
    {
        texture = VideoDriverWrapper::inst().GenerateTexture();

        if (!texture)
        {
            return;
        }
    }

    calcDimensions();

    w = nextPowerOfTwo(w);
    h = nextPowerOfTwo(h);

    // do we have a player-colored overlay?
    unsigned stride = hasPlayer ? w * 2 : w;

    unsigned char* buffer = new unsigned char[stride * h * 4];
    memset(buffer, 0, stride * h * 4);

    drawTo(buffer, stride, h);

    VideoDriverWrapper::inst().BindTexture(texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, stride, h, 0, GL_BGRA, GL_UNSIGNED_BYTE, buffer);

    delete[] buffer;

    tmp[0].tx = tmp[1].tx = 0.0f;
    tmp[2].tx = tmp[3].tx = hasPlayer ? 0.5f : 1.0f;

    tmp[0].ty = tmp[3].ty = tmp[4].ty = tmp[7].ty = 0.0f;
    tmp[1].ty = tmp[2].ty = tmp[5].ty = tmp[6].ty = 1.0f;

    tmp[4].tx = tmp[5].tx = 0.5f;
    tmp[6].tx = tmp[7].tx = 1.0f;
}

void glSmartBitmap::draw(int x, int y, unsigned color, unsigned player_color)
{
    if (!texture)
    {
        generateTexture();

        if (!texture)
        {
            return;
        }
    }

    tmp[0].x = tmp[1].x = GLfloat(x - nx);
    tmp[2].x = tmp[3].x = GLfloat(x - nx + w);

    tmp[0].y = tmp[3].y = GLfloat(y - ny);
    tmp[1].y = tmp[2].y = GLfloat(y - ny + h);

    tmp[0].r = tmp[1].r = tmp[2].r = tmp[3].r = GetRed(color);
    tmp[0].g = tmp[1].g = tmp[2].g = tmp[3].g = GetGreen(color);
    tmp[0].b = tmp[1].b = tmp[2].b = tmp[3].b = GetBlue(color);
    tmp[0].a = tmp[1].a = tmp[2].a = tmp[3].a = GetAlpha(color);

    if ((player_color != 0x00000000) && hasPlayer)
    {
        tmp[4].x = tmp[5].x = tmp[0].x;
        tmp[6].x = tmp[7].x = tmp[2].x;
        tmp[4].y = tmp[7].y = tmp[0].y;
        tmp[5].y = tmp[6].y = tmp[1].y;

        tmp[4].r = tmp[5].r = tmp[6].r = tmp[7].r = GetRed(player_color);
        tmp[4].g = tmp[5].g = tmp[6].g = tmp[7].g = GetGreen(player_color);
        tmp[4].b = tmp[5].b = tmp[6].b = tmp[7].b = GetBlue(player_color);
        tmp[4].a = tmp[5].a = tmp[6].a = tmp[7].a = GetAlpha(player_color);

        glInterleavedArrays(GL_T2F_C4UB_V3F, 0, tmp);
        VideoDriverWrapper::inst().BindTexture(texture);
        glDrawArrays(GL_QUADS, 0, 8);

        return;
    }

    glInterleavedArrays(GL_T2F_C4UB_V3F, 0, tmp);
    VideoDriverWrapper::inst().BindTexture(texture);
    glDrawArrays(GL_QUADS, 0, 4);
}

void glSmartBitmap::drawPercent(int x, int y, unsigned percent, unsigned color, unsigned player_color)
{
    if (!texture)
    {
        generateTexture();

        if (!texture)
        {
            return;
        }
    }

    // nothing to draw?
    if (!percent)
    {
        return;
    }

    tmp[0].x = tmp[1].x = GLfloat(x - nx);
    tmp[2].x = tmp[3].x = GLfloat(x - nx + w);

    tmp[0].y = tmp[3].y = GLfloat(y - ny + h - h * (float) percent / 100.0f);
    tmp[1].y = tmp[2].y = GLfloat(y - ny + h);

    tmp[0].r = tmp[1].r = tmp[2].r = tmp[3].r = GetRed(color);
    tmp[0].g = tmp[1].g = tmp[2].g = tmp[3].g = GetGreen(color);
    tmp[0].b = tmp[1].b = tmp[2].b = tmp[3].b = GetBlue(color);
    tmp[0].a = tmp[1].a = tmp[2].a = tmp[3].a = GetAlpha(color);

    float cache = tmp[0].ty;

    tmp[0].ty = tmp[3].ty = tmp[1].ty - (tmp[1].ty - tmp[0].ty) * (float) percent / 100.0f;

    if ((player_color != 0x00000000) && hasPlayer)
    {
        tmp[4].x = tmp[5].x = tmp[0].x;
        tmp[6].x = tmp[7].x = tmp[2].x;
        tmp[4].y = tmp[7].y = tmp[0].y;
        tmp[5].y = tmp[6].y = tmp[1].y;

        tmp[4].r = tmp[5].r = tmp[6].r = tmp[7].r = GetRed(player_color);
        tmp[4].g = tmp[5].g = tmp[6].g = tmp[7].g = GetGreen(player_color);
        tmp[4].b = tmp[5].b = tmp[6].b = tmp[7].b = GetBlue(player_color);
        tmp[4].a = tmp[5].a = tmp[6].a = tmp[7].a = GetAlpha(player_color);

        tmp[4].ty = tmp[7].ty = tmp[3].ty;

        glInterleavedArrays(GL_T2F_C4UB_V3F, 0, tmp);
        VideoDriverWrapper::inst().BindTexture(texture);
        glDrawArrays(GL_QUADS, 0, 8);

        tmp[0].ty = tmp[3].ty = tmp[4].ty = tmp[7].ty = cache;

        return;
    }

    glInterleavedArrays(GL_T2F_C4UB_V3F, 0, tmp);
    VideoDriverWrapper::inst().BindTexture(texture);
    glDrawArrays(GL_QUADS, 0, 4);

    tmp[0].ty = tmp[3].ty = cache;

}

