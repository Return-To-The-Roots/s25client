// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "glArchivItem_Bitmap_Direct.h"
#include "drivers/VideoDriverWrapper.h"
#include "libsiedler2/PixelBufferBGRA.h"
#include <glad/glad.h>
#include <stdexcept>

glArchivItem_Bitmap_Direct::glArchivItem_Bitmap_Direct() : isUpdating_(false) {}

glArchivItem_Bitmap_Direct::glArchivItem_Bitmap_Direct(const glArchivItem_Bitmap_Direct& item)
    : ArchivItem_BitmapBase(item), baseArchivItem_Bitmap(item), glArchivItem_Bitmap(item), isUpdating_(false)
{}

void glArchivItem_Bitmap_Direct::beginUpdate()
{
    if(isUpdating_)
        throw std::logic_error("Already updating! Forgot an endUpdate?");
    isUpdating_ = true;
    areaToUpdate_ = Rect(0, 0, 0, 0);
}

void glArchivItem_Bitmap_Direct::endUpdate()
{
    if(!isUpdating_)
        throw std::logic_error("Already updating! Forgot an endUpdate?");
    isUpdating_ = false;
    // Nothing to update or no texture created yet
    if(prodOfComponents(areaToUpdate_.getSize()) == 0 || !GetTexNoCreate())
        return;

    libsiedler2::PixelBufferBGRA buffer(areaToUpdate_.getSize().x, areaToUpdate_.getSize().y);
    Position origin = areaToUpdate_.getOrigin();
    int ec = print(buffer, nullptr, 0, 0, origin.x, origin.y);
    RTTR_Assert(ec == 0);
    VIDEODRIVER.BindTexture(GetTexNoCreate());
    glTexSubImage2D(GL_TEXTURE_2D, 0, origin.x, origin.y, buffer.getWidth(), buffer.getHeight(), GL_BGRA,
                    GL_UNSIGNED_BYTE, buffer.getPixelPtr());
}

void glArchivItem_Bitmap_Direct::updatePixel(const DrawPoint& pos, const libsiedler2::ColorBGRA& clr)
{
    RTTR_Assert(isUpdating_);
    RTTR_Assert(pos.x >= 0 && pos.y >= 0);
    RTTR_Assert(static_cast<unsigned>(pos.x) < GetSize().x && static_cast<unsigned>(pos.y) < GetSize().y);
    setPixel(pos.x, pos.y, clr);
    // If the area is empty, create one
    if(areaToUpdate_.getSize().x == 0)
        areaToUpdate_ = Rect(Position(pos), Extent(1, 1));
    else
    {
        // Else resize if required
        if(pos.x < areaToUpdate_.left)
            areaToUpdate_.left = pos.x;
        if(pos.x >= areaToUpdate_.right)
            areaToUpdate_.right = pos.x + 1;
        if(pos.y < areaToUpdate_.top)
            areaToUpdate_.top = pos.y;
        if(pos.y >= areaToUpdate_.bottom)
            areaToUpdate_.bottom = pos.y + 1;
    }
}
