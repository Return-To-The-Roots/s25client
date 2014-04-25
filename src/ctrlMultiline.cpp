// $Id: ctrlMultiline.cpp 9357 2014-04-25 15:35:25Z FloSoft $
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
#include "main.h"
#include "ctrlMultiline.h"

#include "ctrlScrollBar.h"

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#if defined _WIN32 && defined _DEBUG && defined _MSC_VER
#define new new(_NORMAL_BLOCK, THIS_FILE, __Line__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

///////////////////////////////////////////////////////////////////////////////
/**
 *  Konstruktor von @p ctrlMultiline.
 *
 *  @author OLiver
 */
ctrlMultiline::ctrlMultiline(Window* parent,
                             unsigned int id,
                             unsigned short x,
                             unsigned short y,
                             unsigned short width,
                             unsigned short height,
                             TextureColor tc,
                             glArchivItem_Font* font,
                             unsigned int format)
    : Window(x, y, id, parent, width, height),
      tc(tc), font(font), format(format), lines_in_control((height - 4) / font->getHeight()), draw_box(true)
{
    AddScrollBar(0, width - SCROLLBAR_WIDTH, 0, SCROLLBAR_WIDTH, height, SCROLLBAR_WIDTH, tc, lines_in_control);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  fügt eine Zeile hinzu.
 *
 *  @author OLiver
 */
void ctrlMultiline::AddString(const std::string& str, unsigned int color, bool scroll)
{
    Line line = { str, color };
    lines.push_back(line);

    ctrlScrollBar* scrollbar = GetCtrl<ctrlScrollBar>(0);
    scrollbar->SetRange(unsigned(lines.size()));

    if (scroll && (scrollbar->GetPos() == (unsigned(lines.size()) - 1) - lines_in_control))
    {
        scrollbar->SetPos(scrollbar->GetPos() + 1);
    }
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  zeichnet das Fenster.
 *
 *  @author OLiver
 */
bool ctrlMultiline::Draw_(void)
{
    if(draw_box)
        Draw3D(GetX(), GetY(), width, height, tc, 2);

    DrawControls();

    unsigned show_lines = std::min(lines_in_control, unsigned(lines.size()));

    ctrlScrollBar* scrollbar = GetCtrl<ctrlScrollBar>(0);

    for(unsigned short i = 0; i < show_lines; ++i)
        font->Draw(GetX() + 2, GetY() + 2 + i * font->getHeight(), lines[i + scrollbar->GetPos()].str, format, lines[i + scrollbar->GetPos()].color);

    return true;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author OLiver
 */
bool ctrlMultiline::Msg_LeftDown(const MouseCoords& mc)
{
    return GetCtrl<Window>(0)->Msg_LeftDown(mc);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author OLiver
 */
bool ctrlMultiline::Msg_LeftUp(const MouseCoords& mc)
{
    return GetCtrl<Window>(0)->Msg_LeftUp(mc);
}
///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author Divan
 */
bool ctrlMultiline::Msg_WheelUp(const MouseCoords& mc)
{
    // Forward to ScrollBar
    ctrlScrollBar* scrollbar = GetCtrl<ctrlScrollBar>(0);

    //If mouse in list
    if(Coll(mc.x, mc.y, GetX() + 2, GetY() + 2, width - /*2*/2, height - 4))
    {
        // Simulate three Button Clicks
        scrollbar->Msg_ButtonClick(0);
        scrollbar->Msg_ButtonClick(0);
        scrollbar->Msg_ButtonClick(0);
        return true;
    }
    else
        return false;
}


///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author Divan
 */
bool ctrlMultiline::Msg_WheelDown(const MouseCoords& mc)
{
    // Forward to ScrollBar
    ctrlScrollBar* scrollbar = GetCtrl<ctrlScrollBar>(0);

    // If mouse in list
    if(Coll(mc.x, mc.y, GetX() + 2, GetY() + 2, width - /*2*/2, height - 4))
    {
        // Simulate three Button Clicks
        scrollbar->Msg_ButtonClick(1);
        scrollbar->Msg_ButtonClick(1);
        scrollbar->Msg_ButtonClick(1);
        return true;
    }
    else
        return false;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author OLiver
 */
bool ctrlMultiline::Msg_MouseMove(const MouseCoords& mc)
{
    return GetCtrl<Window>(0)->Msg_MouseMove(mc);
}


void ctrlMultiline::Resize_(unsigned short width, unsigned short height)
{
    // Position der Scrollbar anpassen
    GetCtrl<ctrlScrollBar>(0)->Move(width - SCROLLBAR_WIDTH, 0);
}

/// Textzeile ersetzen. Klappt bestimmt nicht mit Scrollbar-Kram
void ctrlMultiline::SetLine(const unsigned index, const std::string& str, unsigned int color)
{
    if (index < lines.size())
    {
        Line line = { str, color };
        lines[index] = line;
    }
}
