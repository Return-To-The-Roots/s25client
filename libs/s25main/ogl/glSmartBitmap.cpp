// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "glSmartBitmap.h"
#include "Loader.h"
#include "drivers/VideoDriverWrapper.h"
#include "ogl/glBitmapItem.h"
#include "libsiedler2/ArchivItem_Bitmap.h"
#include "libsiedler2/ArchivItem_Bitmap_Player.h"
#include "libsiedler2/PixelBufferBGRA.h"
#include "s25util/colors.h"
#include <glad/glad.h>
#include <limits>

namespace {
struct GL_RGBAColor
{
    GLbyte r, g, b, a;
};
} // namespace

glSmartBitmap::glSmartBitmap() : origin_(0, 0), size_(0, 0), sharedTexture(false), texture(0), hasPlayer(false) {}

glSmartBitmap::~glSmartBitmap()
{
    reset();
}

void glSmartBitmap::reset()
{
    if(texture && !sharedTexture)
        VIDEODRIVER.DeleteTexture(texture);
    texture = 0;

    for(glBitmapItem& bmpItem : items)
    {
        if(bmpItem.isOwning_)
            delete bmpItem.bmp;
    }
    items.clear();
}

Extent glSmartBitmap::getRequiredTexSize() const
{
    Extent texSize(size_);
    if(hasPlayer)
        texSize.x *= 2;
    return texSize;
}

void glSmartBitmap::add(libsiedler2::ArchivItem_Bitmap_Player* bmp)
{
    if(!bmp)
        return;
    items.push_back(glBitmapItem(bmp));
    calcDimensions();
}

void glSmartBitmap::add(libsiedler2::baseArchivItem_Bitmap* bmp)
{
    if(!bmp)
        return;
    items.push_back(glBitmapItem(bmp));
    calcDimensions();
}

void glSmartBitmap::add(std::unique_ptr<libsiedler2::baseArchivItem_Bitmap> bmp)
{
    if(!bmp)
        return;
    items.push_back(glBitmapItem(bmp.release(), false, true));
    calcDimensions();
}

void glSmartBitmap::addShadow(libsiedler2::baseArchivItem_Bitmap* bmp)
{
    if(!bmp)
        return;
    items.push_back(glBitmapItem(bmp, true));
    calcDimensions();
}

void glSmartBitmap::calcDimensions()
{
    if(items.empty())
    {
        origin_ = Position(0, 0);
        size_ = Extent(0, 0);
        return;
    }

    origin_.x = origin_.y = std::numeric_limits<int>::min();
    Position maxPos = origin_;

    hasPlayer = false;

    for(const glBitmapItem& bmpItem : items)
    {
        if(bmpItem.type == glBitmapItemType::PlayerBitmap)
            hasPlayer = true;

        origin_ = elMax(origin_, bmpItem.origin);
        maxPos = elMax(maxPos, bmpItem.size - bmpItem.origin);
    }

    size_ = Extent(origin_ + maxPos);
}

void glSmartBitmap::drawTo(libsiedler2::PixelBufferBGRA& buffer, const Extent& bufOffset) const
{
    libsiedler2::ArchivItem_Palette* p_colors = LOADER.GetPaletteN("colors");
    libsiedler2::ArchivItem_Palette* p_5 = LOADER.GetPaletteN("pal5");

    for(const glBitmapItem& bmpItem : items)
    {
        if((bmpItem.size.x == 0) || (bmpItem.size.y == 0))
            continue;

        DrawPoint offset = origin_ - bmpItem.origin;

        if(bmpItem.type == glBitmapItemType::ShadowBitmap)
        {
            libsiedler2::PixelBufferBGRA tmp(size_.x, size_.y);

            dynamic_cast<libsiedler2::baseArchivItem_Bitmap*>(bmpItem.bmp) //-V522
              ->print(tmp, p_5, offset.x, offset.y, bmpItem.pos.x, bmpItem.pos.y, bmpItem.size.x, bmpItem.size.y);

            unsigned tmpIdx = 0;
            for(unsigned y = 0; y < size_.y; ++y)
            {
                unsigned idx = buffer.calcIdx(bufOffset.x, bufOffset.y + y);
                for(unsigned x = 0; x < size_.x; ++x)
                {
                    if(tmp.get(tmpIdx).getAlpha() != 0x00 && buffer.get(idx).getAlpha() == 0x00)
                        buffer.set(idx, libsiedler2::ColorBGRA(0, 0, 0, 0x40));
                    idx++;
                    tmpIdx++;
                }
            }
        } else if(!hasPlayer)
        {
            // No player bitmap -> Just (over)write the data
            RTTR_Assert(bmpItem.type == glBitmapItemType::Normal);
            dynamic_cast<libsiedler2::baseArchivItem_Bitmap*>(bmpItem.bmp)
              ->print(buffer, p_5, offset.x + bufOffset.x, offset.y + bufOffset.y, bmpItem.pos.x, bmpItem.pos.y,
                      bmpItem.size.x, bmpItem.size.y);
        } else
        {
            // There is a player bitmap -> First write to temp buffer
            libsiedler2::PixelBufferBGRA tmp(size_.x, size_.y);
            if(bmpItem.type == glBitmapItemType::Normal)
            {
                dynamic_cast<libsiedler2::baseArchivItem_Bitmap*>(bmpItem.bmp)
                  ->print(tmp, p_5, offset.x, offset.y, bmpItem.pos.x, bmpItem.pos.y, bmpItem.size.x, bmpItem.size.y);
            } else
            {
                dynamic_cast<libsiedler2::ArchivItem_Bitmap_Player*>(bmpItem.bmp)
                  ->print(tmp, p_colors, 128, offset.x, offset.y, bmpItem.pos.x, bmpItem.pos.y, bmpItem.size.x,
                          bmpItem.size.y, false);
            }
            // Now copy temp buffer to real buffer, but we need to reset all player colors that would be overwritten
            // so it looks like, the first bitmap is fully drawn (including player colors) and then the next
            // overwrites it
            unsigned tmpIdx = 0;
            for(unsigned y = 0; y < size_.y; ++y)
            {
                unsigned idx = buffer.calcIdx(bufOffset.x + offset.x, bufOffset.y + y);
                tmpIdx += offset.x;
                for(unsigned x = offset.x; x < size_.x; x++)
                {
                    // Check for non-transparent pixels
                    if(tmp.get(tmpIdx).getAlpha())
                    {
                        // Copy to buffer
                        buffer.set(idx, tmp.get(tmpIdx));
                        // Reset player color to transparent
                        buffer.set(idx + size_.x, libsiedler2::ColorBGRA());
                    }
                    ++idx;
                    ++tmpIdx;
                }
            }
            // Finally write the player color part if it has one
            if(bmpItem.type == glBitmapItemType::PlayerBitmap)
            {
                dynamic_cast<libsiedler2::ArchivItem_Bitmap_Player*>(bmpItem.bmp)
                  ->print(buffer, p_colors, 128, offset.x + size_.x + bufOffset.x, offset.y + bufOffset.y,
                          bmpItem.pos.x, bmpItem.pos.y, bmpItem.size.x, bmpItem.size.y, true);
            }
        }
    }
}

void glSmartBitmap::generateTexture()
{
    if(items.empty())
        return;

    if(!texture)
    {
        texture = VIDEODRIVER.GenerateTexture();

        if(!texture)
            return;
    }

    const Extent bufSize = VIDEODRIVER.calcPreferredTextureSize(getRequiredTexSize());

    libsiedler2::PixelBufferBGRA buffer(bufSize.x, bufSize.y);
    drawTo(buffer);

    VIDEODRIVER.BindTexture(texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, bufSize.x, bufSize.y, 0, GL_BGRA, GL_UNSIGNED_BYTE, buffer.getPixelPtr());

    using PointF = Point<float>;

    /* 0--3/4--7
     * |  |    |
     * 1--2/5--6
     **/
    texCoords[0] = PointF::all(0);
    texCoords[2] = PointF(getRequiredTexSize()) / PointF(bufSize);
    if(hasPlayer)
    {
        texCoords[6] = texCoords[2];
        texCoords[2].x /= 2.f;
    }
    texCoords[1] = PointF(texCoords[0].x, texCoords[2].y);
    texCoords[3] = PointF(texCoords[2].x, texCoords[0].y);
    if(hasPlayer)
    {
        texCoords[4] = texCoords[3];
        texCoords[5] = texCoords[2];
        texCoords[7] = PointF(texCoords[6].x, texCoords[4].y);
    }
}

void glSmartBitmap::draw(DrawPoint drawPt, unsigned color, unsigned player_color)
{
    drawPercent(drawPt, 100, color, player_color);
}

void glSmartBitmap::drawPercent(DrawPoint drawPt, unsigned percent, unsigned color, unsigned player_color)
{
    if(!texture)
    {
        generateTexture();

        if(!texture)
            return;
    }

    // nothing to draw?
    if(!percent)
        return;
    RTTR_Assert(percent <= 100);

    const float partDrawn = percent / 100.f;
    std::array<Point<GLfloat>, 8> vertices, curTexCoords;
    std::array<GL_RGBAColor, 8> colors;

    drawPt -= origin_;
    vertices[2] = Point<GLfloat>(drawPt) + size_;

    vertices[0].x = vertices[1].x = GLfloat(drawPt.x);
    vertices[3].x = vertices[2].x;

    vertices[0].y = vertices[3].y = GLfloat(drawPt.y + size_.y * (1.f - partDrawn));
    vertices[1].y = vertices[2].y;

    colors[0].r = GetRed(color);
    colors[0].g = GetGreen(color);
    colors[0].b = GetBlue(color);
    colors[0].a = GetAlpha(color);
    colors[3] = colors[2] = colors[1] = colors[0];

    curTexCoords[0] = texCoords[0];
    curTexCoords[1] = texCoords[1];
    curTexCoords[2] = texCoords[2];
    curTexCoords[3] = texCoords[3];
    curTexCoords[0].y = curTexCoords[3].y = curTexCoords[1].y - (curTexCoords[1].y - curTexCoords[0].y) * partDrawn;

    int numQuads;
    if(player_color && hasPlayer)
    {
        std::copy(vertices.begin(), vertices.begin() + 4, vertices.begin() + 4);

        colors[4].r = GetRed(player_color);
        colors[4].g = GetGreen(player_color);
        colors[4].b = GetBlue(player_color);
        colors[4].a = GetAlpha(player_color);
        colors[7] = colors[6] = colors[5] = colors[4];

        curTexCoords[4] = texCoords[4];
        curTexCoords[5] = texCoords[5];
        curTexCoords[6] = texCoords[6];
        curTexCoords[7] = texCoords[7];
        curTexCoords[4].y = curTexCoords[7].y = curTexCoords[0].y;

        numQuads = 8;
    } else
        numQuads = 4;

    glEnableClientState(GL_COLOR_ARRAY);
    glVertexPointer(2, GL_FLOAT, 0, vertices.data());
    glTexCoordPointer(2, GL_FLOAT, 0, curTexCoords.data());
    glColorPointer(4, GL_UNSIGNED_BYTE, 0, colors.data());
    VIDEODRIVER.BindTexture(texture);
    glDrawArrays(GL_QUADS, 0, numQuads);
    glDisableClientState(GL_COLOR_ARRAY);
}
