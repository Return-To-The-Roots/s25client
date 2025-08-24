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

void ctrlImage::Draw_()
{
    DrawImage(Rect(GetDrawPos(), GetImageRect().getSize()));
}

bool ctrlImage::Msg_MouseMove(const MouseCoords& mc)
{
    if(GetImage())
    {
        if(IsMouseOver(mc.GetPos()))
            ShowTooltip();
        else
            HideTooltip();
    }

    return false;
}

Rect ctrlImage::GetBoundaryRect() const
{
    return Rect::move(GetImageRect(), GetDrawPos());
}
