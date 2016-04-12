// Copyright (c) 2005 - 2015 Settlers Freaks (sf-team at siedler25.org)
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

///////////////////////////////////////////////////////////////////////////////
// Header
#include "defines.h" // IWYU pragma: keep
#include "ctrlButton.h"
#include "Loader.h"
#include "CollisionDetection.h"
#include "WindowManager.h"
#include "drivers/VideoDriverWrapper.h"
#include "ogl/glArchivItem_Bitmap.h"
#include "ogl/glArchivItem_Font.h"

// Include last!
#include "DebugNew.h" // IWYU pragma: keep

ctrlButton::ctrlButton(Window* parent, unsigned int id, unsigned short x, unsigned short y,
                       unsigned short width, unsigned short height, TextureColor tc, const std::string& tooltip)
    : Window(x, y, id, parent, width, height), tc(tc), state(BUTTON_UP), border(true),
      check(false), illuminated(false), enabled(true)
{
    SetTooltip(tooltip);
}


ctrlButton::~ctrlButton()
{
    WINDOWMANAGER.SetToolTip(this, "");
}

bool ctrlButton::Msg_MouseMove(const MouseCoords& mc)
{
    if(enabled && IsMouseOver(mc.x, mc.y))
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

bool ctrlButton::IsMouseOver(const int mouseX, const int mouseY) const
{
    return Coll(mouseX, mouseY, GetX(), GetY(), width_, height_);
}

bool ctrlButton::Msg_LeftDown(const MouseCoords& mc)
{
    if(enabled && IsMouseOver(mc.x, mc.y))
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

        if(enabled && IsMouseOver(mc.x, mc.y))
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
        if(!IsMouseOver(VIDEODRIVER.GetMouseX(), VIDEODRIVER.GetMouseY()))
            // Nicht mehr drauf --> wieder normalen Zustand
            state = BUTTON_UP;
    }
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  zeichnet das Fenster.
 *
 *  @author OLiver
 */
bool ctrlButton::Draw_()
{
    if(width_ == 0 || height_ == 0)
        return true;

    // Prüfen, ob bei gehighlighteten Button die Maus auch noch über dem Button ist
    TestMouseOver();

    Rect buttonrect(GetX(), GetY(), width_, height_);

    if(tc != TC_INVISIBLE)
    {
        if(border)
            Draw3D(buttonrect.left, buttonrect.top, width_, height_, tc, (unsigned short)((check) ? 2 : state), illuminated);
        else
        {
            if(state == BUTTON_UP || state == BUTTON_PRESSED)
                LOADER.GetImageN("io", tc * 2 + 1)->Draw(buttonrect.left, buttonrect.top, 0, 0, 0, 0, width_, height_);
            else
                LOADER.GetImageN("io", tc * 2)->Draw(buttonrect.left, buttonrect.top,  0, 0, 0, 0, width_, height_);
        }
    }

    /// Inhalt malen (Text, Bilder usw.)
    DrawContent();

    return true;
}


ctrlTextButton::ctrlTextButton(Window* parent, unsigned int id, unsigned short x, unsigned short y,
                               unsigned short width, unsigned short height, const TextureColor tc,
                               const std::string& text,  glArchivItem_Font* font, const std::string& tooltip)
    : ctrlButton(parent, id, x, y, width, height, tc, tooltip), ctrlBaseText(text, COLOR_YELLOW, font)
{}


/// Abgeleitete Klassen müssen erweiterten Button-Inhalt zeichnen (Text in dem Fall)
void ctrlTextButton::DrawContent() const
{
    const bool isHighlighted = state == BUTTON_PRESSED || check;
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
    font->Draw(GetX() + width_ / 2 + offset,
               GetY() + height_ / 2 + offset,
               text,
               glArchivItem_Font::DF_CENTER | glArchivItem_Font::DF_VCENTER,
               color,
               0,
               maxTextWidth);
}


ctrlImageButton::ctrlImageButton(Window* parent, unsigned int id, unsigned short x, unsigned short y,
                                 unsigned short width, unsigned short height, const TextureColor tc,
                                 glArchivItem_Bitmap* const image, const std::string& tooltip)
    : ctrlButton(parent, id, x, y, width, height, tc, tooltip), image(image), modulation_color(0xFFFFFFFF)
{
}

void ctrlImageButton::DrawContent() const
{
    // Bild
    if(image)
    {
        const unsigned short offset = (state == BUTTON_PRESSED || check) ? 2 : 0;
        image->Draw(GetX() + width_ / 2 + offset, GetY() + height_ / 2 + offset, 0, 0, 0, 0, 0, 0, modulation_color);
    }
}


/// Abgeleitete Klassen müssen erweiterten Button-Inhalt zeichnen (Text in dem Fall)
ctrlColorButton::ctrlColorButton(Window* parent, unsigned int id, unsigned short x, unsigned short y,
                                 unsigned short width, unsigned short height, const TextureColor tc,
                                 unsigned int fillColor, const std::string& tooltip) :
    ctrlButton(parent, id, x, y, width, height, tc, tooltip),
    fillColor(fillColor)
{
}


/// Abgeleitete Klassen müssen erweiterten Button-Inhalt zeichnen (Farbe in dem Fall)
void ctrlColorButton::DrawContent() const
{
    DrawRectangle(x_ + 3, y_ + 3, width_ - 6, height_ - 6, fillColor);
}


void ctrlColorButton::SetColor(const unsigned int fill_color)
{
    this->fillColor = fill_color;
}
