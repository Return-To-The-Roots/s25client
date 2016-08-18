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

#include "defines.h" // IWYU pragma: keep
#include "ctrlScrollBar.h"
#include "ctrlButton.h"
#include "CollisionDetection.h"
#include "Loader.h"
#include "driver/src/MouseCoords.h"

ctrlScrollBar::ctrlScrollBar(Window* parent,
                             unsigned int id,
                             unsigned short x,
                             unsigned short y,
                             unsigned short width,
                             unsigned short height,
                             unsigned short button_height,
                             TextureColor tc,
                             unsigned short pagesize)
    : Window(DrawPoint(x, y), id, parent, width, height),
      button_height(button_height), tc(tc), pagesize(pagesize),
      scroll_range(0), scroll_pos(0), scroll_height(0), sliderHeight(0), sliderPos(0), isMouseScrolling(false), last_y(0)
{
    visible_ = false;

    AddImageButton(0, 0, 0, width, button_height, tc, LOADER.GetImageN("io", 33));
    AddImageButton(1, 0, (height > button_height) ? height - button_height : 1, width, button_height, tc, LOADER.GetImageN("io", 34));

    Resize(width, height);
}

void ctrlScrollBar::Scroll(int distance)
{
    if(distance == 0 || scroll_range == 0)
        return;
    // Calc new scroll pos
    int newScrollPos = scroll_pos + distance;
    // But beware of over and underflows. Final value must be >= 0
    if(newScrollPos + pagesize > scroll_range)
        newScrollPos = static_cast<int>(scroll_range) - pagesize;
    if(newScrollPos < 0)
        newScrollPos = 0;
    if(scroll_pos != static_cast<unsigned short>(newScrollPos))
    {
        scroll_pos = static_cast<unsigned short>(newScrollPos);
        UpdateSliderFromPos();
        parent_->Msg_ScrollChange(id_, scroll_pos);
    }
}

bool ctrlScrollBar::Msg_LeftUp(const MouseCoords& mc)
{
    isMouseScrolling = false;

    // ButtonMessages weiterleiten
    return RelayMouseMessage(&Window::Msg_LeftUp, mc);
}

bool ctrlScrollBar::Msg_LeftDown(const MouseCoords& mc)
{
    if (Coll(mc.x, mc.y, GetX(), GetY() + button_height + sliderPos, width_, sliderHeight))
    {
        // Maus auf dem Scrollbutton
        isMouseScrolling = true;
        return true;
    }else if (Coll(mc.x, mc.y, GetX(), GetY() + button_height, width_, sliderPos))
    {
        // Clicked above slider -> Move half a slider height up
        if (sliderPos < sliderHeight / 2)
            sliderPos = 0;
        else
            sliderPos -= sliderHeight / 2;

        UpdatePosFromSlider();
        return true;
    }
    else
    {
        unsigned short bottomSliderPos = button_height + sliderPos + sliderHeight;

        if (Coll(mc.x, mc.y, GetX(), GetY() + bottomSliderPos, width_, height_ - (bottomSliderPos + button_height)))
        {
            // Clicked below slider -> Move half a slider height down
            sliderPos += sliderHeight / 2;

            if(sliderPos + sliderHeight > scroll_height)
            {
                RTTR_Assert(scroll_height > sliderHeight); // Otherwise the scrollbar should be hidden
                sliderPos = scroll_height - sliderHeight;
            }

            UpdatePosFromSlider();
            return true;
        }
    }

    return RelayMouseMessage(&Window::Msg_LeftDown, mc);
}

bool ctrlScrollBar::Msg_MouseMove(const MouseCoords& mc)
{
    if(isMouseScrolling)
    {
        const int moveDist = mc.y - last_y;
        sliderPos += moveDist;
        if(sliderPos + sliderHeight > scroll_height)
            sliderPos = moveDist < 0 ? 0 : (scroll_height - sliderHeight);

        UpdatePosFromSlider();
    }
    last_y = mc.y;

    // ButtonMessages weiterleiten
    return RelayMouseMessage(&Window::Msg_MouseMove, mc);
}

void ctrlScrollBar::Msg_ButtonClick(const unsigned int ctrl_id)
{
    switch(ctrl_id)
    {
        case 0: // Upwards
            Scroll(-1);
            break;
        case 1: // Downwards
            Scroll(+1);
            break;
    }
}

/**
 *  setzt die Scroll-Position.
 */
void ctrlScrollBar::SetPos(unsigned short scroll_pos)
{
    this->scroll_pos = scroll_pos;
    UpdateSliderFromPos();
}

/**
 *  setzt die Scroll-Höhe.
 */
void ctrlScrollBar::SetRange(unsigned short scroll_range)
{
    this->scroll_range = scroll_range;
    RecalculateSizes();
}

/**
 *  setzt die Seiten-Höhe.
 */
void ctrlScrollBar::SetPageSize(unsigned short pagesize)
{
    this->pagesize = pagesize;
    RecalculateSizes();
}

void ctrlScrollBar::Resize(unsigned short width, unsigned short height)
{
    Window::Resize(width, height);

    // Up button
    GetCtrl<ctrlButton>(0)->Resize(width, button_height);
    // Down button
    ctrlButton* downButton = GetCtrl<ctrlButton>(1);
    downButton->Resize(width, button_height);

    if(height >= button_height)
    {
        downButton->SetVisible(true);
        downButton->Move(0, height - button_height);
    }
    else
        downButton->SetVisible(false);

    RecalculateSizes();
}

/**
 *  Zeichenmethode.
 *
 *  @return @p true bei Erfolg, @p false bei Fehler
 */
bool ctrlScrollBar::Draw_()
{
    RTTR_Assert(scroll_range > pagesize); // Don't show unneccessary scrollbars, otherwise invariants might be violated.
    DrawPoint pos = GetDrawPos();
    // Leiste
    Draw3D(pos + DrawPoint(0, button_height - 2), width_, height_ - button_height * 2 + 4, tc, 2);

    // Buttons
    DrawControls();

    // Scrollbar
    Draw3D(pos + DrawPoint(0, button_height + sliderPos), width_, sliderHeight, tc, 0);

    return true;
}

void ctrlScrollBar::UpdatePosFromSlider()
{
    RTTR_Assert(sliderPos + sliderHeight <= scroll_height); // Slider must be inside bar
    unsigned short newScrollPos = (sliderPos * scroll_range) / scroll_height;
    if(scroll_pos != newScrollPos)
    {
        RTTR_Assert(newScrollPos + pagesize <= scroll_range); // Probably slider to small?
        scroll_pos = newScrollPos;
        parent_->Msg_ScrollChange(id_, scroll_pos);
    }
}

void ctrlScrollBar::UpdateSliderFromPos()
{
    if(scroll_pos + pagesize >= scroll_range)
        sliderPos = scroll_height - sliderHeight;
    else
        sliderPos = (scroll_height * scroll_pos) / scroll_range;
}

/**
 *  berechnet die Werte für die Scrollbar.
 */
void ctrlScrollBar::RecalculateSizes()
{
    scroll_height = ((height_ > 2 * button_height) ? height_ - 2 * button_height : 0);

    if(scroll_range > pagesize)
    {
        sliderHeight = (scroll_height * pagesize) / scroll_range;

        UpdateSliderFromPos();

        if(!visible_)
        {
            visible_ = true;
            parent_->Msg_ScrollShow(id_, visible_);
        }
    }
    else
    {
        scroll_pos = 0;

        // nicht nötig, Scrollleiste kann weg
        if(visible_)
        {
            visible_ = false;
            parent_->Msg_ScrollShow(id_, visible_);
        }
    }
}
