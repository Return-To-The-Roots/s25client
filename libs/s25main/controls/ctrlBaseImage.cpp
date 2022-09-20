// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
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

void ctrlBaseImage::DrawImage(const Rect& dstArea) const
{
    DrawImage(dstArea, modulationColor_);
}

void ctrlBaseImage::DrawImage(const Rect& dstArea, unsigned color) const
{
    if(img_ == nullptr)
        return;

    auto dst = dstArea;
    auto imageSize = img_->GetSize();
    auto dstSize = dstArea.getSize();
    Rect srcArea = Rect(DrawPoint::all(0), imageSize);

    if(imageSize.x > dstSize.x)
    {
        auto halfDelta = (imageSize.x - dstSize.x) / 2;
        srcArea.left += halfDelta;
        srcArea.right -= halfDelta;
    } else if(imageSize.x < dstSize.x)
    {
        auto halfDelta = (dstSize.x - imageSize.x) / 2;
        dst.left += halfDelta;
        dst.right -= halfDelta;
    }

    if(imageSize.y > dstSize.y)
    {
        auto halfDelta = (imageSize.y - dstSize.y) / 2;
        srcArea.top += halfDelta;
        srcArea.bottom -= halfDelta;
    } else if(imageSize.y < dstSize.y)
    {
        auto halfDelta = (dstSize.y - imageSize.y) / 2;
        dst.top += halfDelta;
        dst.bottom -= halfDelta;
    }

    img_->Draw(dst, srcArea, color);
}
