// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "ctrlImage.h"
#include "CollisionDetection.h"
#include "driver/MouseCoords.h"

ctrlImage::ctrlImage(Window* parent, unsigned id, const DrawPoint& pos, ITexture* image, const std::string& tooltip)
    : Window(parent, id, pos), ctrlBaseTooltip(tooltip), ctrlBaseImage(image)
{}

ctrlImage::~ctrlImage() = default;

/**
 *  zeichnet das Fenster.
 */
void ctrlImage::Draw_()
{
    DrawImage(Rect(GetDrawPos(), GetImageRect().getSize()));
}

bool ctrlImage::Msg_MouseMove(const MouseCoords& mc)
{
    // gültiges Bild?
    if(GetImage())
    {
        // Jeweils Tooltip ein- und ausblenden, wenn die Maus über dem Bild ist
        if(IsPointInRect(mc.GetPos(), Rect::move(GetImageRect(), GetDrawPos())))
            ShowTooltip();
        else
            HideTooltip();
    }

    return false;
}
