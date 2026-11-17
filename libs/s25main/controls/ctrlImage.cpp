// Copyright (C) 2005 - 2025 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "ctrlImage.h"
#include "CollisionDetection.h"
#include "driver/MouseCoords.h"

ctrlImage::ctrlImage(Window* parent, unsigned id, const DrawPoint& pos, ITexture* image, const std::string& tooltip)
    : Window(parent, id, pos), ctrlBaseTooltip(tooltip), ctrlBaseImage(image)
{}

ctrlImage::~ctrlImage() = default;

void ctrlImage::drawImpl()
{
    drawImage(Rect(getDrawPos(), getImageRect().getSize()));
}

bool ctrlImage::msgMouseMove(const MouseCoords& mc)
{
    if(getImage())
    {
        if(isMouseOver(mc.pos))
            showTooltip();
        else
            hideTooltip();
    }

    return false;
}

Rect ctrlImage::getBoundaryRect() const
{
    return Rect::move(getImageRect(), getDrawPos());
}
