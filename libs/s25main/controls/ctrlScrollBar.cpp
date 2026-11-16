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
    setVisible(false);

    addImageButton(0, DrawPoint(0, 0), Extent(size.x, button_height), tc, LOADER.getImageN("io", 33));
    addImageButton(1, DrawPoint(0, (size.y > button_height) ? size.y - button_height : 1),
                   Extent(size.x, button_height), tc, LOADER.getImageN("io", 34));

    resize(size);
}

void ctrlScrollBar::scroll(int distance)
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
        updateSliderFromPos();
        getParent()->msgScrollChange(getID(), scroll_pos);
    }
}

bool ctrlScrollBar::msgLeftUp(const MouseCoords& mc)
{
    isMouseScrolling = false;

    // ButtonMessages weiterleiten
    return relayMouseMessage(&Window::msgLeftUp, mc);
}

bool ctrlScrollBar::msgLeftDown(const MouseCoords& mc)
{
    if(IsPointInRect(mc.pos,
                     Rect(getDrawPos().x, getDrawPos().y + button_height + sliderPos, getSize().x, sliderHeight)))
    {
        // Maus auf dem Scrollbutton
        isMouseScrolling = true;
        return true;
    } else if(IsPointInRect(mc.pos, Rect(getDrawPos().x, getDrawPos().y + button_height, getSize().x, sliderPos)))
    {
        // Clicked above slider -> Move half a slider height up
        if(sliderPos < sliderHeight / 2)
            sliderPos = 0;
        else
            sliderPos -= sliderHeight / 2;

        updatePosFromSlider();
        return true;
    } else
    {
        unsigned short bottomSliderPos = button_height + sliderPos + sliderHeight;

        if(IsPointInRect(mc.pos, Rect(getDrawPos().x, getDrawPos().y + bottomSliderPos, getSize().x,
                                      getSize().y - (bottomSliderPos + button_height))))
        {
            // Clicked below slider -> Move half a slider height down
            sliderPos += sliderHeight / 2;

            if(sliderPos + sliderHeight > scroll_height)
            {
                RTTR_Assert(scroll_height > sliderHeight); // Otherwise the scrollbar should be hidden
                sliderPos = scroll_height - sliderHeight;
            }

            updatePosFromSlider();
            return true;
        }
    }

    return relayMouseMessage(&Window::msgLeftDown, mc);
}

bool ctrlScrollBar::msgMouseMove(const MouseCoords& mc)
{
    if(isMouseScrolling)
    {
        const int moveDist = mc.pos.y - last_y;
        sliderPos += moveDist;
        if(sliderPos + sliderHeight > scroll_height)
            sliderPos = moveDist < 0 ? 0 : (scroll_height - sliderHeight);

        updatePosFromSlider();
    }
    last_y = mc.pos.y;

    // ButtonMessages weiterleiten
    return relayMouseMessage(&Window::msgMouseMove, mc);
}

void ctrlScrollBar::msgButtonClick(const unsigned ctrl_id)
{
    switch(ctrl_id)
    {
        case 0: // Upwards
            scroll(-1);
            break;
        case 1: // Downwards
            scroll(+1);
            break;
    }
}

/**
 *  setzt die Scroll-Position.
 */
void ctrlScrollBar::setScrollPos(unsigned short scroll_pos)
{
    if(this->scroll_pos != scroll_pos)
    {
        this->scroll_pos = scroll_pos;
        updateSliderFromPos();
    }
}

/**
 *  setzt die Scroll-Höhe.
 */
void ctrlScrollBar::setRange(unsigned short scroll_range)
{
    if(this->scroll_range != scroll_range)
    {
        this->scroll_range = scroll_range;
        recalculateSizes();
    }
}

/**
 *  setzt die Seiten-Höhe.
 */
void ctrlScrollBar::setPageSize(unsigned short pagesize)
{
    if(this->pagesize != pagesize)
    {
        this->pagesize = pagesize;
        recalculateSizes();
    }
}

void ctrlScrollBar::resize(const Extent& newSize)
{
    Window::resize(newSize);

    // Up button
    Extent btSize = Extent(newSize.x, button_height);
    getCtrl<ctrlButton>(0)->resize(btSize);
    // Down button
    auto* downButton = getCtrl<ctrlButton>(1);
    downButton->resize(btSize);

    if(newSize.y >= button_height)
    {
        downButton->setVisible(true);
        downButton->setPos(DrawPoint(0, newSize.y - button_height));
    } else
        downButton->setVisible(false);

    recalculateSizes();
}

/**
 *  Zeichenmethode.
 *
 *  @return @p true bei Erfolg, @p false bei Fehler
 */
void ctrlScrollBar::drawImpl()
{
    RTTR_Assert(scroll_range > pagesize); // Don't show unneccessary scrollbars, otherwise invariants might be violated.
    if(scroll_height == 0)
        return;
    DrawPoint pos = getDrawPos();
    // Leiste
    draw3D(Rect(pos + DrawPoint(0, button_height - 2), getSize().x, getSize().y - button_height * 2 + 4), tc, false);

    // Buttons
    Window::drawImpl();

    // Scrollbar
    draw3D(Rect(pos + DrawPoint(0, button_height + sliderPos), getSize().x, sliderHeight), tc, true);
}

void ctrlScrollBar::updatePosFromSlider()
{
    RTTR_Assert(sliderPos + sliderHeight <= scroll_height); // Slider must be inside bar
    unsigned short newScrollPos = (sliderPos * scroll_range) / scroll_height;
    if(scroll_pos != newScrollPos)
    {
        RTTR_Assert(newScrollPos + pagesize <= scroll_range); // Probably slider to small?
        scroll_pos = newScrollPos;
        getParent()->msgScrollChange(getID(), scroll_pos);
    }
}

void ctrlScrollBar::updateSliderFromPos()
{
    if(scroll_pos + pagesize >= scroll_range)
        sliderPos = scroll_height - sliderHeight;
    else
        sliderPos = (scroll_height * scroll_pos) / scroll_range;
}

/**
 *  berechnet die Werte für die Scrollbar.
 */
void ctrlScrollBar::recalculateSizes()
{
    scroll_height = ((getSize().y > 2u * button_height) ? getSize().y - 2u * button_height : 0);

    if(scroll_range > pagesize)
    {
        sliderHeight = (scroll_height * pagesize) / scroll_range;

        updateSliderFromPos();

        if(!isVisible())
        {
            setVisible(true);
            getParent()->msgScrollShow(getID(), isVisible());
        }
    } else
    {
        scroll_pos = 0;

        // nicht nötig, Scrollleiste kann weg
        if(isVisible())
        {
            setVisible(false);
            getParent()->msgScrollShow(getID(), isVisible());
        }
    }
}
