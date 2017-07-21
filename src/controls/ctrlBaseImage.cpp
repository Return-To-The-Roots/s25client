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

#include "defines.h" // IWYU pragma: keep
#include "ctrlBaseImage.h"
#include "ogl/glArchivItem_Bitmap.h"
#include "libutil/src/colors.h"
#include <algorithm>

ctrlBaseImage::ctrlBaseImage(glArchivItem_Bitmap* img /*= NULL*/): img_(img), modulationColor_(COLOR_WHITE)
{}

void ctrlBaseImage::SwapImage(ctrlBaseImage & other)
{
    std::swap(img_, other.img_);
}

Rect ctrlBaseImage::GetImageRect() const
{
    if(!img_)
        return Rect(0, 0, 0, 0);
    return Rect(-img_->GetOrigin(), img_->GetSize());
}

void ctrlBaseImage::DrawImage(const DrawPoint& pos) const
{
    DrawImage(pos, modulationColor_);
}

void ctrlBaseImage::DrawImage(const DrawPoint& pos, unsigned color) const
{
    if(img_)
        img_->DrawFull(pos, color);
}
