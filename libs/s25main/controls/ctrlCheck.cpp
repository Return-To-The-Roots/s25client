// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "ctrlCheck.h"

#include "CollisionDetection.h"
#include "Loader.h"
#include "driver/MouseCoords.h"
#include "ogl/FontStyle.h"
#include "ogl/glArchivItem_Bitmap.h"
#include "ogl/glFont.h"
#include <utility>

ctrlCheck::ctrlCheck(Window* parent, unsigned id, const DrawPoint& pos, const Extent& size, TextureColor tc,
                     std::string text, const glFont* font, bool readonly)
    : Window(parent, id, pos, size), tc(tc), text(std::move(text)), font(font), check(false), readonly(readonly)
{}

/**
 *  der Messagehandler.
 *
 *  @param[in] msg   Die Nachricht.
 *  @param[in] id    Die ID des Quellsteuerelements.
 *  @param[in] param Ein nachrichtenspezifischer Parameter.
 */

bool ctrlCheck::Msg_LeftDown(const MouseCoords& mc)
{
    if(!readonly && IsPointInRect(mc.GetPos(), GetDrawRect()))
    {
        check = !check;
        GetParent()->Msg_CheckboxChange(GetID(), check);
        return true;
    }

    return false;
}

/**
 *  zeichnet das Fenster.
 */
void ctrlCheck::Draw_()
{
    const unsigned short boxSize = 20;
    short spacing = (GetSize().y - boxSize) / 2;
    if(spacing < 0)
        spacing = 0;
    Rect drawRect = GetDrawRect();
    const bool drawText = font && !text.empty();
    if(!drawText)
    {
        // If we draw only the check mark, draw surrounding box smaller and center checkbox
        drawRect.setSize(Extent(boxSize + 2 * spacing, drawRect.getSize().y));
        drawRect.move(DrawPoint((GetSize().x - drawRect.getSize().x) / 2, 0));
    }
    short boxStartOffsetX = drawRect.getSize().x - spacing - boxSize;
    if(boxStartOffsetX < 0)
        boxStartOffsetX = 0;

    Draw3D(drawRect, tc, false);

    if(drawText)
    {
        int availableWidth = boxStartOffsetX - 4;
        if(availableWidth < 0)
            availableWidth = 0;
        font->Draw(drawRect.getOrigin() + DrawPoint(4, GetSize().y / 2), text, FontStyle::VCENTER,
                   (check ? COLOR_YELLOW : 0xFFBBBBBB), availableWidth);
    }

    DrawPoint boxPos = drawRect.getOrigin() + DrawPoint(boxStartOffsetX, spacing);

    if(!readonly)
        Draw3D(Rect(boxPos, Extent::all(boxSize)), tc, false);

    if(check)
        LOADER.GetImageN("io", 32)->DrawFull(boxPos + DrawPoint(boxSize, boxSize) / 2);
}
