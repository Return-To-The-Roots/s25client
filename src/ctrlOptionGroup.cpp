// $Id: ctrlOptionGroup.cpp 9357 2014-04-25 15:35:25Z FloSoft $
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
#include "ctrlOptionGroup.h"

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#if defined _WIN32 && defined _DEBUG && defined _MSC_VER
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

///////////////////////////////////////////////////////////////////////////////
/**
 *  Konstruktor von @p ctrlOptionGroup.
 *
 *  @author OLiver
 */
ctrlOptionGroup::ctrlOptionGroup(Window* parent,
                                 unsigned int id,
                                 int select_type,
                                 bool scale)
    : ctrlGroup(parent, id, scale),
      selection(0xFFFF), select_type(select_type)
{
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Zeichenmethode.
 *
 *  @author OLiver
 */
bool ctrlOptionGroup::Draw_(void)
{
    DrawControls();

    return true;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  wählt einen Button aus der Gruppe aus.
 *
 *  @author OLiver
 */
void ctrlOptionGroup::SetSelection(unsigned short selection, bool notify)
{
    // Aktuellen ausgewählten Button wieder normal machen
    if(this->selection != 0xFFFF)
    {
        ctrlButton* button = GetCtrl<ctrlButton>(this->selection);
        assert(button);
        switch(select_type)
        {
            case ILLUMINATE: button->SetIlluminated(false); break;
            case CHECK:      button->SetCheck(false);       break;
            case SHOW:       button->SetVisible(true);     break;
        }
    }

    // Neuen Button auswählen
    if(selection != 0xFFFF)
    {
        ctrlButton* button = GetCtrl<ctrlButton>(selection);
        assert(button);
        switch(select_type)
        {
            case ILLUMINATE: button->SetIlluminated(true); break;
            case CHECK:      button->SetCheck(true);       break;
            case SHOW:       button->SetVisible(false);     break;
        }
    }

    this->selection = selection;

    if(notify && parent)
        parent->Msg_OptionGroupChange(GetID(), selection);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author OLiver
 */
void ctrlOptionGroup::Msg_ButtonClick(const unsigned int ctrl_id)
{
    SetSelection(ctrl_id, true);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author OLiver
 */
bool ctrlOptionGroup::Msg_LeftDown(const MouseCoords& mc)
{
    return RelayMouseMessage(&Window::Msg_LeftDown, mc);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author OLiver
 */
bool ctrlOptionGroup::Msg_LeftUp(const MouseCoords& mc)
{
    return RelayMouseMessage(&Window::Msg_LeftUp, mc);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author Divan
 */
bool ctrlOptionGroup::Msg_WheelUp(const MouseCoords& mc)
{
    return RelayMouseMessage(&Window::Msg_WheelUp, mc);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author Divan
 */
bool ctrlOptionGroup::Msg_WheelDown(const MouseCoords& mc)
{
    return RelayMouseMessage(&Window::Msg_WheelDown, mc);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author OLiver
 */
bool ctrlOptionGroup::Msg_MouseMove(const MouseCoords& mc)
{
    return RelayMouseMessage(&Window::Msg_MouseMove, mc);
}
