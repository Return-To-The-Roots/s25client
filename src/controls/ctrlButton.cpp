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
#include "ctrlButton.h"
#include "Loader.h"
#include "CollisionDetection.h"
#include "drivers/VideoDriverWrapper.h"
#include "driver/src/MouseCoords.h"
#include "ogl/glArchivItem_Bitmap.h"
#include "ogl/glArchivItem_Font.h"
#include "ExtensionList.h"
#include <algorithm>

ctrlButton::ctrlButton(Window* parent, unsigned int id, const DrawPoint& pos,
                       const Extent& size, TextureColor tc, const std::string& tooltip)
    : Window(pos, id, parent, size), tc(tc), ctrlBaseTooltip(tooltip), state(BUTTON_UP), hasBorder(true),
      isChecked(false), isIlluminated(false), isEnabled(true)
{}


ctrlButton::~ctrlButton()
{}

void ctrlButton::SetEnabled(bool enable /*= true*/)
{
    isEnabled = enable;
    state = BUTTON_UP;
}

bool ctrlButton::Msg_MouseMove(const MouseCoords& mc)
{
    if(isEnabled && IsMouseOver(mc.GetPos()))
    {
        if(state != BUTTON_PRESSED)
            state = BUTTON_HOVER;

        ShowTooltip();
        return true;
    }
    else
    {
        state =  BUTTON_UP;
        HideTooltip();
        return false;
    }
}

bool ctrlButton::IsMouseOver(const Point<int>& mousePos) const
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
        state =  BUTTON_UP;

        if(isEnabled && IsMouseOver(mc.GetPos()))
        {
            GetParent()->Msg_ButtonClick(GetID());
            return true;
        }
    }

    return false;
}

// Prüfen, ob bei gehighlighteten Button die Maus auch noch über dem Button ist
void ctrlButton::TestMouseOver()
{
    if(state == BUTTON_HOVER || state == BUTTON_PRESSED)
    {
        if(!IsMouseOver(VIDEODRIVER.GetMousePos()))
            // Nicht mehr drauf --> wieder normalen Zustand
            state = BUTTON_UP;
    }
}

/**
 *  zeichnet das Fenster.
 */
void ctrlButton::Draw_()
{
    if(GetSize().x == 0 || GetSize().y == 0)
        return;

    // Prüfen, ob bei gehighlighteten Button die Maus auch noch über dem Button ist
    TestMouseOver();

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
            LOADER.GetImageN("io", texture)->Draw(GetDrawPos(), 0, 0, 0, 0, GetSize().x, GetSize().y, color);
            if(isIlluminated)
                glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
        }
    }

    /// Inhalt malen (Text, Bilder usw.)
    DrawContent();
}


ctrlTextButton::ctrlTextButton(Window* parent, unsigned int id, const DrawPoint& pos,
                               const Extent& size, const TextureColor tc,
                               const std::string& text,  glArchivItem_Font* font, const std::string& tooltip)
    : ctrlButton(parent, id, pos, size, tc, tooltip), ctrlBaseText(text, COLOR_YELLOW, font)
{}


/// Abgeleitete Klassen müssen erweiterten Button-Inhalt zeichnen (Text in dem Fall)
void ctrlTextButton::DrawContent() const
{
    const bool isPressed = state == BUTTON_PRESSED || isChecked;
    unsigned color;
    if(this->color_ == COLOR_YELLOW && isPressed)
        color = 0xFFFFAA00;
    else if(!isEnabled)
        color = COLOR_GREY;
    else
        color = this->color_;

    const unsigned short maxTextWidth = GetSize().x - 4; // reduced by border

    if(GetTooltip().empty() && state == BUTTON_HOVER)
    {
        unsigned maxNumChars;
        font->getWidth(text, 0, maxTextWidth, &maxNumChars);
        if(maxNumChars < text.length())
            ShowTooltip(text);
    }

    const unsigned short offset = isPressed ? 2 : 0;
    font->Draw(GetDrawPos() + DrawPoint(GetSize()) / 2 + DrawPoint(offset, offset),
               text,
               glArchivItem_Font::DF_CENTER | glArchivItem_Font::DF_VCENTER,
               color,
               0,
               maxTextWidth);
}


ctrlImageButton::ctrlImageButton(Window* parent, unsigned int id, const DrawPoint& pos,
                                 const Extent& size, const TextureColor tc,
                                 glArchivItem_Bitmap* const image, const std::string& tooltip)
    : ctrlButton(parent, id, pos, size, tc, tooltip), ctrlBaseImage(image)
{}

void ctrlImageButton::DrawContent() const
{
    DrawPoint pos = GetDrawPos() + DrawPoint(GetSize()) / 2;
    if((state == BUTTON_PRESSED || isChecked) && isEnabled)
        pos += DrawPoint::all(2);
    if(!isEnabled && GetModulationColor() == COLOR_WHITE)
        DrawImage(pos, 0xFF555555);
    else
        DrawImage(pos);
}


/// Abgeleitete Klassen müssen erweiterten Button-Inhalt zeichnen (Text in dem Fall)
ctrlColorButton::ctrlColorButton(Window* parent, unsigned int id, const DrawPoint& pos,
                                 const Extent& size, const TextureColor tc,
                                 unsigned int fillColor, const std::string& tooltip) :
    ctrlButton(parent, id, pos, size, tc, tooltip),
    ctrlBaseColor(fillColor)
{
}


/// Abgeleitete Klassen müssen erweiterten Button-Inhalt zeichnen (Farbe in dem Fall)
void ctrlColorButton::DrawContent() const
{
    Extent rectSize = GetSize() - Extent(6, 6);
    DrawRectangle(Rect(GetDrawPos() + DrawPoint(3, 3), rectSize), color_);
}
