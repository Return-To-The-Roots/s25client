// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "ctrlBaseImage.h"
#include "ogl/ITexture.h"
#include "s25util/colors.h"

ctrlBaseImage::ctrlBaseImage(ITexture* img /*= nullptr*/) : img_(img), modulationColor_(COLOR_WHITE) {}

void ctrlBaseImage::SwapImage(ctrlBaseImage& other)
{
    std::swap(img_, other.img_);
}

Rect ctrlBaseImage::GetImageRect() const
{
    if(!img_)
        return Rect();
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
