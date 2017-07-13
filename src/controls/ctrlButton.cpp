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
#include "WindowManager.h"
#include "drivers/VideoDriverWrapper.h"
#include "ogl/glArchivItem_Bitmap.h"
#include "ogl/glArchivItem_Font.h"
#include <algorithm>

ctrlButton::ctrlButton(Window* parent, unsigned int id, unsigned short x, unsigned short y,
                       unsigned short width, unsigned short height, TextureColor tc, const std::string& tooltip)
    : Window(DrawPoint(x, y), id, parent, width, height), tc(tc), state(BUTTON_UP), hasBorder(true),
      isChecked(false), isIlluminated(false), isEnabled(true)
{
    SetTooltip(tooltip);
}


ctrlButton::~ctrlButton()
{
    WINDOWMANAGER.SetToolTip(this, "");
}

void ctrlButton::SetEnabled(bool enable /*= true*/)
{
    isEnabled = enable;
    state = BUTTON_UP;
}

void ctrlButton::SwapTooltip(ctrlButton* otherBt)
{
    std::swap(tooltip_, otherBt->tooltip_);
}

bool ctrlButton::Msg_MouseMove(const MouseCoords& mc)
{
    if(isEnabled && IsMouseOver(mc.GetPos()))
    {
        if(state != BUTTON_PRESSED)
            state = BUTTON_HOVER;

        WINDOWMANAGER.SetToolTip(this, tooltip_);

        return true;
    }
    else
    {
        state =  BUTTON_UP;
        WINDOWMANAGER.SetToolTip(this, "");

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
            parent_->Msg_ButtonClick(GetID());
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
    if(width_ == 0 || height_ == 0)
        return;

    // Prüfen, ob bei gehighlighteten Button die Maus auch noch über dem Button ist
    TestMouseOver();

    if(tc != TC_INVISIBLE)
    {
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
            Draw3D(GetDrawPos(), width_, height_, tc, type, isCurIlluminated);
        } else
        {
            unsigned texture;
            if(isEnabled && (state == BUTTON_UP || state == BUTTON_PRESSED))
                texture = tc * 2 + 1;
            else
                texture = tc * 2;
            unsigned color = isEnabled ? COLOR_WHITE : 0xFF666666;
            if(isIlluminated)
            {
                glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_EXT);
                glTexEnvf(GL_TEXTURE_ENV, GL_RGB_SCALE_EXT, 2.0f);
            }
            LOADER.GetImageN("io", texture)->Draw(GetDrawPos(), 0, 0, 0, 0, width_, height_, color);
            if(isIlluminated)
                glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
        }
    }

    /// Inhalt malen (Text, Bilder usw.)
    DrawContent();
}


ctrlTextButton::ctrlTextButton(Window* parent, unsigned int id, unsigned short x, unsigned short y,
                               unsigned short width, unsigned short height, const TextureColor tc,
                               const std::string& text,  glArchivItem_Font* font, const std::string& tooltip)
    : ctrlButton(parent, id, x, y, width, height, tc, tooltip), ctrlBaseText(text, COLOR_YELLOW, font)
{}


/// Abgeleitete Klassen müssen erweiterten Button-Inhalt zeichnen (Text in dem Fall)
void ctrlTextButton::DrawContent() const
{
    const bool isHighlighted = state == BUTTON_PRESSED || isChecked;
    unsigned color;
    if(this->color_ == COLOR_YELLOW && isHighlighted)
        color = 0xFFFFAA00;
    else
        color = this->color_;

    const unsigned short maxTextWidth = width_ - 4; // reduced by border

    if(tooltip_.empty() && state == BUTTON_HOVER)
    {
        unsigned maxNumChars;
        font->getWidth(text, 0, maxTextWidth, &maxNumChars);
        if(maxNumChars < text.length())
            WINDOWMANAGER.SetToolTip(this, text);
    }

    const unsigned short offset = isHighlighted ? 2 : 0;
    font->Draw(GetDrawPos() + DrawPoint(width_, height_) / 2 + DrawPoint(offset, offset),
               text,
               glArchivItem_Font::DF_CENTER | glArchivItem_Font::DF_VCENTER,
               color,
               0,
               maxTextWidth);
}


ctrlImageButton::ctrlImageButton(Window* parent, unsigned int id, unsigned short x, unsigned short y,
                                 unsigned short width, unsigned short height, const TextureColor tc,
                                 glArchivItem_Bitmap* const image, const std::string& tooltip)
    : ctrlButton(parent, id, x, y, width, height, tc, tooltip), image(image), modulation_color(COLOR_WHITE)
{
}

void ctrlImageButton::DrawContent() const
{
    // Bild
    if(image)
    {
        const unsigned short offset = ((state == BUTTON_PRESSED || isChecked) && isEnabled) ? 2 : 0;
        unsigned color = modulation_color;
        if(!isEnabled && modulation_color == COLOR_WHITE)
            color = 0xFF555555;
        image->Draw(GetDrawPos() + DrawPoint(width_, height_) / 2 + DrawPoint(offset, offset), 0, 0, 0, 0, 0, 0, color);
    }
}


/// Abgeleitete Klassen müssen erweiterten Button-Inhalt zeichnen (Text in dem Fall)
ctrlColorButton::ctrlColorButton(Window* parent, unsigned int id, unsigned short x, unsigned short y,
                                 unsigned short width, unsigned short height, const TextureColor tc,
                                 unsigned int fillColor, const std::string& tooltip) :
    ctrlButton(parent, id, x, y, width, height, tc, tooltip),
    ctrlBaseColor(fillColor)
{
}


/// Abgeleitete Klassen müssen erweiterten Button-Inhalt zeichnen (Farbe in dem Fall)
void ctrlColorButton::DrawContent() const
{
    DrawRectangle(GetDrawPos() + DrawPoint(3, 3), width_ - 6, height_ - 6, color_);
}
