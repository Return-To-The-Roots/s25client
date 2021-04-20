// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "ctrlButton.h"
#include "CollisionDetection.h"
#include "driver/MouseCoords.h"
#include "drivers/VideoDriverWrapper.h"

ctrlButton::ctrlButton(Window* parent, unsigned id, const DrawPoint& pos, const Extent& size, TextureColor tc,
                       const std::string& tooltip)
    : Window(parent, id, pos, size), ctrlBaseTooltip(tooltip), tc(tc), state(ButtonState::Up), hasBorder(true),
      isChecked(false), isIlluminated(false), isEnabled(true)
{}

ctrlButton::~ctrlButton() = default;

void ctrlButton::SetEnabled(bool enable /*= true*/)
{
    isEnabled = enable;
    state = ButtonState::Up;
}

void ctrlButton::SetActive(bool activate)
{
    Window::SetActive(activate);
    if(!activate)
        state = ButtonState::Up;
    else if(IsMouseOver(VIDEODRIVER.GetMousePos()))
        state = ButtonState::Hover;
}

bool ctrlButton::Msg_MouseMove(const MouseCoords& mc)
{
    if(isEnabled && IsMouseOver(mc.GetPos()))
    {
        if(state != ButtonState::Pressed)
            state = ButtonState::Hover;

        ShowTooltip();
        return true;
    } else
    {
        state = ButtonState::Up;
        HideTooltip();
        return false;
    }
}

bool ctrlButton::IsMouseOver(const Position& mousePos) const
{
    return IsPointInRect(mousePos, GetDrawRect());
}

bool ctrlButton::Msg_LeftDown(const MouseCoords& mc)
{
    if(isEnabled && IsMouseOver(mc.GetPos()))
    {
        state = ButtonState::Pressed;
        return true;
    }

    return false;
}

bool ctrlButton::Msg_LeftUp(const MouseCoords& mc)
{
    if(state == ButtonState::Pressed)
    {
        if(isEnabled && IsMouseOver(mc.GetPos()))
        {
            state = ButtonState::Hover;
            GetParent()->Msg_ButtonClick(GetID());
            return true;
        } else
            state = ButtonState::Up;
    }

    return false;
}

/**
 *  zeichnet das Fenster.
 */
void ctrlButton::Draw_()
{
    if(GetSize().x == 0 || GetSize().y == 0)
        return;

    if(tc != TextureColor::Invisible)
    {
        unsigned color = isEnabled ? COLOR_WHITE : 0xFFBBBBBB;
        bool isCurIlluminated = isIlluminated || (!isEnabled && isChecked);
        bool isElevated = !isChecked && state != ButtonState::Pressed;
        bool isHighlighted = isEnabled && !isChecked && state == ButtonState::Hover;
        if(hasBorder)
            Draw3D(GetDrawRect(), tc, isElevated, isHighlighted, isCurIlluminated, color);
        else
            Draw3DContent(GetDrawRect(), tc, isElevated, isHighlighted, isCurIlluminated, color);
    }

    /// Inhalt malen (Text, Bilder usw.)
    DrawContent();
}
