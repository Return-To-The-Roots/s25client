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
    glTexSubImage2D(GL_TEXTURE_2D, 0, origin.x, origin.y, buffer.getWidth(), buffer.getHeight(), GL_BGRA, GL_UNSIGNED_BYTE,
                    buffer.getPixelPtr());
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
