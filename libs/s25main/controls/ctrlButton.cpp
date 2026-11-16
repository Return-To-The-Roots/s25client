// Copyright (C) 2005 - 2025 Settlers Freaks (sf-team at siedler25.org)
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

void ctrlButton::setEnabled(bool enable /*= true*/)
{
    isEnabled = enable;
    state = ButtonState::Up;
}

void ctrlButton::setActive(bool activate)
{
    Window::setActive(activate);
    if(!activate)
        state = ButtonState::Up;
    else if(isMouseOver())
        state = ButtonState::Hover;
}

bool ctrlButton::msgMouseMove(const MouseCoords& mc)
{
    if(isEnabled && isMouseOver(mc))
    {
        if(state != ButtonState::Pressed)
            state = ButtonState::Hover;

        showTooltip();
        return true;
    } else
    {
        state = ButtonState::Up;
        hideTooltip();
        return false;
    }
}

bool ctrlButton::msgLeftDown(const MouseCoords& mc)
{
    if(isEnabled && isMouseOver(mc))
    {
        state = ButtonState::Pressed;
        return true;
    }

    return false;
}

bool ctrlButton::msgLeftUp(const MouseCoords& mc)
{
    if(state == ButtonState::Pressed)
    {
        if(isEnabled && isMouseOver(mc))
        {
            state = ButtonState::Hover;
            getParent()->msgButtonClick(getID());
            return true;
        } else
            state = ButtonState::Up;
    }

    return false;
}

/**
 *  zeichnet das Fenster.
 */
void ctrlButton::drawImpl()
{
    if(getSize().x == 0 || getSize().y == 0)
        return;

    if(tc != TextureColor::Invisible)
    {
        unsigned color = isEnabled ? COLOR_WHITE : 0xFFBBBBBB;
        bool isCurIlluminated = isIlluminated || (!isEnabled && isChecked);
        bool isElevated = !isChecked && state != ButtonState::Pressed;
        bool isHighlighted = isEnabled && !isChecked && state == ButtonState::Hover;
        if(hasBorder)
            draw3D(getDrawRect(), tc, isElevated, isHighlighted, isCurIlluminated, color);
        else
            draw3DContent(getDrawRect(), tc, isElevated, isHighlighted, isCurIlluminated, color);
    }

    /// Inhalt malen (Text, Bilder usw.)
    drawContent();
}
