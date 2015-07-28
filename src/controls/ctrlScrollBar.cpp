﻿// $Id: ctrlScrollBar.cpp 9357 2014-04-25 15:35:25Z FloSoft $
//
// Copyright (c) 2005 - 2011 Settlers Freaks (sf-team at siedler25.org)
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
#include "defines.h"
#include "ctrlScrollBar.h"
#include "ctrlButton.h"

#include "Loader.h"

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#if defined _WIN32 && defined _DEBUG && defined _MSC_VER
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

///////////////////////////////////////////////////////////////////////////////
/**
 *  Konstruktor von @p ctrlScrollBar.
 *
 *  @author OLiver
 */
ctrlScrollBar::ctrlScrollBar(Window* parent,
                             unsigned int id,
                             unsigned short x,
                             unsigned short y,
                             unsigned short width,
                             unsigned short height,
                             unsigned short button_height,
                             TextureColor tc,
                             unsigned short pagesize)
    : Window(x, y, id, parent, width, height),
      button_height(button_height), tc(tc), pagesize(pagesize),
      move(false), scroll_range(0), scroll_pos(0), scroll_height(0), scrollbar_height(0), scrollbar_pos(0), last_y(0)
{
    visible = false;

    AddImageButton(0, 0, 0, width, button_height, tc, LOADER.GetImageN("io", 33));
    AddImageButton(1, 0, (height > button_height) ? height - button_height : 1, width, button_height, tc, LOADER.GetImageN("io", 34));

    Resize_(width, height);

    CalculateScrollBar();
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author OLiver
 */
bool ctrlScrollBar::Msg_LeftUp(const MouseCoords& mc)
{
    move = false;

    // ButtonMessages weiterleiten
    return RelayMouseMessage(&Window::Msg_LeftUp, mc);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author OLiver
 */
bool ctrlScrollBar::Msg_LeftDown(const MouseCoords& mc)
{
    if (Coll(mc.x, mc.y, GetX(), GetY() + button_height + scrollbar_pos, width, scrollbar_height))
    {
        // Maus auf dem Scrollbutton
        move = true;

        return true;
    }

    else if (Coll(mc.x, mc.y, GetX(), GetY(), width, button_height) || Coll(mc.x, mc.y, GetX(), GetY() + height - button_height, width, button_height))
    {
        // Maus auf einer Schaltflaeche
        return RelayMouseMessage(&Window::Msg_LeftDown, mc);
    }

    else
    {
        // Maus auf der Leiste
        unsigned short diff = scrollbar_height / 2;

        if (Coll(mc.x, mc.y, GetX(), GetY() + button_height, width, scrollbar_pos))
        {
            if (scrollbar_pos < diff)
                scrollbar_pos = 0;
            else
                scrollbar_pos -= diff;

            CalculatePosition();
            parent->Msg_ScrollChange(id, scroll_pos);
            return true;
        }
        else
        {
            unsigned short sbb = button_height + scrollbar_pos + scrollbar_height;

            if (Coll(mc.x, mc.y, GetX(), GetY() + sbb, width, height - (sbb + button_height)))
            {
                scrollbar_pos += diff;

                if (scrollbar_pos > (scroll_height - scrollbar_height))
                    scrollbar_pos = scroll_height - scrollbar_height;

                CalculatePosition();
                parent->Msg_ScrollChange(id, scroll_pos);
                return true;
            }
        }
    }

    return false;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author OLiver
 */
bool ctrlScrollBar::Msg_MouseMove(const MouseCoords& mc)
{
    if(move)
    {
        scrollbar_pos += (mc.y - last_y);
        if(scrollbar_pos + scrollbar_height > scroll_height)
            scrollbar_pos = ((mc.y - last_y) < 0 ? 0 : (scroll_height - scrollbar_height));

        CalculatePosition();
        if(scroll_pos > scroll_range - pagesize)
            scroll_pos = scroll_range - pagesize;
        parent->Msg_ScrollChange(id, scroll_pos);
    }
    last_y = mc.y;

    // ButtonMessages weiterleiten
    return RelayMouseMessage(&Window::Msg_MouseMove, mc);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author OLiver
 */
void ctrlScrollBar::Msg_ButtonClick(const unsigned int ctrl_id)
{
    switch(ctrl_id)
    {
        case 0: // Upwards
        {
            if(scroll_pos > 0)
            {
                --scroll_pos;
                parent->Msg_ScrollChange(id, scroll_pos);
            }
        } break;
        case 1: // Downwards
        {
            if(scroll_pos < scroll_range - pagesize)
            {
                ++scroll_pos;
                parent->Msg_ScrollChange(id, scroll_pos);
            }
        } break;
    }

    CalculateScrollBar();
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  setzt die Scroll-Position.
 *
 *  @author OLiver
 */
void ctrlScrollBar::SetPos(unsigned short scroll_pos)
{
    this->scroll_pos = scroll_pos;
    CalculateScrollBar();

}

///////////////////////////////////////////////////////////////////////////////
/**
 *  setzt die Scroll-Höhe.
 *
 *  @author OLiver
 */
void ctrlScrollBar::SetRange(unsigned short scroll_range)
{
    this->scroll_range = scroll_range;
    CalculateScrollBar();
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  setzt die Seiten-Höhe.
 *
 *  @author OLiver
 */
void ctrlScrollBar::SetPageSize(unsigned short pagesize)
{
    this->pagesize = pagesize;
    CalculateScrollBar();
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author OLiver
 */
void ctrlScrollBar::Resize_(unsigned short width, unsigned short height)
{
    ctrlButton* button;

    button = GetCtrl<ctrlButton>(0);
    button->Resize(width, button_height);

    button = GetCtrl<ctrlButton>(1);
    button->Resize(width, button_height);

    if(height >= button_height)
    {
        button->SetVisible(true);
        button->Move(0, height - button_height);
    }
    else
        button->SetVisible(false);

    CalculateScrollBar(height);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Zeichenmethode.
 *
 *  @return @p true bei Erfolg, @p false bei Fehler
 *
 *  @author OLiver
 */
bool ctrlScrollBar::Draw_(void)
{
    // Leiste
    Draw3D(GetX(), GetY() + button_height - 2, width, height - button_height * 2 + 4, tc, 2);

    // Buttons
    DrawControls();

    // Scrollbar
    Draw3D(GetX(), GetY() + button_height + scrollbar_pos, width, scrollbar_height, tc, 0);

    return true;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  berechnet die Werte für die Scrollbar.
 *
 *  @author FloSoft
 */
void ctrlScrollBar::CalculateScrollBar(unsigned short height)
{
    // Default parameter
    if(height == 0) height = this->height;

    scroll_height = ((height > 2 * button_height) ? height - 2 * button_height : 0);

    if(scroll_range > pagesize)
    {
        scrollbar_height = (scroll_height * pagesize) / scroll_range;

        if(scroll_pos == scroll_range - pagesize)
            scrollbar_pos = scroll_height - scrollbar_height;
        else
            scrollbar_pos = (scroll_height * scroll_pos) / scroll_range;

        if(!visible)
        {
            visible = true;
            parent->Msg_ScrollShow(id, visible);
        }
    }
    else
    {
        scrollbar_pos = 0;
        scrollbar_height = 0;
        scroll_pos = 0;

        // nicht nötig, Scrollleiste kann weg
        if(visible)
        {
            visible = false;
            parent->Msg_ScrollShow(id, visible);
        }
    }
}
