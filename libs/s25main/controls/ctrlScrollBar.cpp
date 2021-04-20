// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "ctrlScrollBar.h"
#include "CollisionDetection.h"
#include "Loader.h"
#include "ctrlButton.h"
#include "driver/MouseCoords.h"

ctrlScrollBar::ctrlScrollBar(Window* parent, unsigned id, const DrawPoint& pos, const Extent& size,
                             unsigned short button_height, TextureColor tc, unsigned short pagesize)
    : Window(parent, id, pos, size), button_height(button_height), tc(tc), pagesize(pagesize), scroll_range(0),
      scroll_pos(0), scroll_height(0), sliderHeight(0), sliderPos(0), isMouseScrolling(false), last_y(0)
{
    SetVisible(false);

    AddImageButton(0, DrawPoint(0, 0), Extent(size.x, button_height), tc, LOADER.GetImageN("io", 33));
    AddImageButton(1, DrawPoint(0, (size.y > button_height) ? size.y - button_height : 1),
                   Extent(size.x, button_height), tc, LOADER.GetImageN("io", 34));

    Resize(size);
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
        GetParent()->Msg_ScrollChange(GetID(), scroll_pos);
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
    if(IsPointInRect(mc.GetPos(),
                     Rect(GetDrawPos().x, GetDrawPos().y + button_height + sliderPos, GetSize().x, sliderHeight)))
    {
        // Maus auf dem Scrollbutton
        isMouseScrolling = true;
        return true;
    } else if(IsPointInRect(mc.GetPos(), Rect(GetDrawPos().x, GetDrawPos().y + button_height, GetSize().x, sliderPos)))
    {
        // Clicked above slider -> Move half a slider height up
        if(sliderPos < sliderHeight / 2)
            sliderPos = 0;
        else
            sliderPos -= sliderHeight / 2;

        UpdatePosFromSlider();
        return true;
    } else
    {
        unsigned short bottomSliderPos = button_height + sliderPos + sliderHeight;

        if(IsPointInRect(mc.GetPos(), Rect(GetDrawPos().x, GetDrawPos().y + bottomSliderPos, GetSize().x,
                                           GetSize().y - (bottomSliderPos + button_height))))
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
        const int moveDist = mc.pos.y - last_y;
        sliderPos += moveDist;
        if(sliderPos + sliderHeight > scroll_height)
            sliderPos = moveDist < 0 ? 0 : (scroll_height - sliderHeight);

        UpdatePosFromSlider();
    }
    last_y = mc.pos.y;

    // ButtonMessages weiterleiten
    return RelayMouseMessage(&Window::Msg_MouseMove, mc);
}

void ctrlScrollBar::Msg_ButtonClick(const unsigned ctrl_id)
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
void ctrlScrollBar::SetScrollPos(unsigned short scroll_pos)
{
    if(this->scroll_pos != scroll_pos)
    {
        this->scroll_pos = scroll_pos;
        UpdateSliderFromPos();
    }
}

/**
 *  setzt die Scroll-Höhe.
 */
void ctrlScrollBar::SetRange(unsigned short scroll_range)
{
    if(this->scroll_range != scroll_range)
    {
        this->scroll_range = scroll_range;
        RecalculateSizes();
    }
}

/**
 *  setzt die Seiten-Höhe.
 */
void ctrlScrollBar::SetPageSize(unsigned short pagesize)
{
    if(this->pagesize != pagesize)
    {
        this->pagesize = pagesize;
        RecalculateSizes();
    }
}

void ctrlScrollBar::Resize(const Extent& newSize)
{
    Window::Resize(newSize);

    // Up button
    Extent btSize = Extent(newSize.x, button_height);
    GetCtrl<ctrlButton>(0)->Resize(btSize);
    // Down button
    auto* downButton = GetCtrl<ctrlButton>(1);
    downButton->Resize(btSize);

    if(newSize.y >= button_height)
    {
        downButton->SetVisible(true);
        downButton->SetPos(DrawPoint(0, newSize.y - button_height));
    } else
        downButton->SetVisible(false);

    RecalculateSizes();
}

/**
 *  Zeichenmethode.
 *
 *  @return @p true bei Erfolg, @p false bei Fehler
 */
void ctrlScrollBar::Draw_()
{
    RTTR_Assert(scroll_range > pagesize); // Don't show unneccessary scrollbars, otherwise invariants might be violated.
    if(scroll_height == 0)
        return;
    DrawPoint pos = GetDrawPos();
    // Leiste
    Draw3D(Rect(pos + DrawPoint(0, button_height - 2), GetSize().x, GetSize().y - button_height * 2 + 4), tc, false);

    // Buttons
    Window::Draw_();

    // Scrollbar
    Draw3D(Rect(pos + DrawPoint(0, button_height + sliderPos), GetSize().x, sliderHeight), tc, true);
}

void ctrlScrollBar::UpdatePosFromSlider()
{
    RTTR_Assert(sliderPos + sliderHeight <= scroll_height); // Slider must be inside bar
    unsigned short newScrollPos = (sliderPos * scroll_range) / scroll_height;
    if(scroll_pos != newScrollPos)
    {
        RTTR_Assert(newScrollPos + pagesize <= scroll_range); // Probably slider to small?
        scroll_pos = newScrollPos;
        GetParent()->Msg_ScrollChange(GetID(), scroll_pos);
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
    scroll_height = ((GetSize().y > 2u * button_height) ? GetSize().y - 2u * button_height : 0);

    if(scroll_range > pagesize)
    {
        sliderHeight = (scroll_height * pagesize) / scroll_range;

        UpdateSliderFromPos();

        if(!IsVisible())
        {
            SetVisible(true);
            GetParent()->Msg_ScrollShow(GetID(), IsVisible());
        }
    } else
    {
        scroll_pos = 0;

        // nicht nötig, Scrollleiste kann weg
        if(IsVisible())
        {
            SetVisible(false);
            GetParent()->Msg_ScrollShow(GetID(), IsVisible());
        }
    }
}
