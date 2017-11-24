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

#include "rttrDefines.h" // IWYU pragma: keep
#include "ctrlButton.h"
#include "CollisionDetection.h"
#include "ExtensionList.h"
#include "Loader.h"
#include "driver/MouseCoords.h"
#include "drivers/VideoDriverWrapper.h"
#include "ogl/glArchivItem_Bitmap.h"

ctrlButton::ctrlButton(Window* parent, unsigned id, const DrawPoint& pos, const Extent& size, TextureColor tc, const std::string& tooltip)
    : Window(parent, id, pos, size), ctrlBaseTooltip(tooltip), tc(tc), state(BUTTON_UP), hasBorder(true), isChecked(false),
      isIlluminated(false), isEnabled(true)
{}

ctrlButton::~ctrlButton() {}

void ctrlButton::SetEnabled(bool enable /*= true*/)
{
    isEnabled = enable;
    state = BUTTON_UP;
}

void ctrlButton::SetActive(bool activate)
{
    Window::SetActive(activate);
    if(!activate)
        state = BUTTON_UP;
    else if(IsMouseOver(VIDEODRIVER.GetMousePos()))
        state = BUTTON_HOVER;
}

bool ctrlButton::Msg_MouseMove(const MouseCoords& mc)
{
    if(isEnabled && IsMouseOver(mc.GetPos()))
    {
        if(state != BUTTON_PRESSED)
            state = BUTTON_HOVER;

        ShowTooltip();
        return true;
    } else
    {
        state = BUTTON_UP;
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
        state = BUTTON_PRESSED;
        return true;
    }

    return false;
}

bool ctrlButton::Msg_LeftUp(const MouseCoords& mc)
{
    if(state == BUTTON_PRESSED)
    {
        if(isEnabled && IsMouseOver(mc.GetPos()))
        {
            state = BUTTON_HOVER;
            GetParent()->Msg_ButtonClick(GetID());
            return true;
        } else
            state = BUTTON_UP;
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

    if(tc != TC_INVISIBLE)
    {
        unsigned color = isEnabled ? COLOR_WHITE : 0xFF666666;
        if(hasBorder)
        {
            bool isCurIlluminated = isIlluminated;
            ButtonState type;
            if(isEnabled)
                type = isChecked ? BUTTON_PRESSED : state;
            else
            {
                type = BUTTON_UP;
                isCurIlluminated |= isChecked;
            }
            Draw3D(Rect(GetDrawPos(), GetSize()), tc, type, isCurIlluminated, true, color);
        } else
        {
            unsigned texture;
            if(isEnabled && (state == BUTTON_UP || state == BUTTON_PRESSED))
                texture = tc * 2 + 1;
            else
                texture = tc * 2;
            if(isIlluminated)
            {
                glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_EXT);
                glTexEnvf(GL_TEXTURE_ENV, GL_RGB_SCALE_EXT, 2.0f);
            }
            LOADER.GetImageN("io", texture)->DrawPart(Rect(GetDrawPos(), GetSize()), DrawPoint::all(0), color);
            if(isIlluminated)
                glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
        }
    }

    /// Inhalt malen (Text, Bilder usw.)
    DrawContent();
}
