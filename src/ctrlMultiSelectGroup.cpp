// $Id: ctrlMultiSelectGroup.cpp 9357 2014-04-25 15:35:25Z FloSoft $
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
#include "ctrlMultiSelectGroup.h"

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#if defined _WIN32 && defined _DEBUG && defined _MSC_VER
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

///////////////////////////////////////////////////////////////////////////////
/**
 *  Konstruktor von @p ctrlMultiSelectGroup.
 *
 *  @author jh
 */
ctrlMultiSelectGroup::ctrlMultiSelectGroup(Window* parent,
        unsigned int id,
        int select_type,
        bool scale)
    : ctrlGroup(parent, id, scale),
      selection(std::set<unsigned short>()), select_type(select_type)
{
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Zeichenmethode.
 *
 *  @author jh
 */
bool ctrlMultiSelectGroup::Draw_(void)
{
    DrawControls();

    return true;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Selektiert einen neuen Button aus der Gruppe.
 *
 *  @author jh
 */
void ctrlMultiSelectGroup::AddSelection(unsigned short selection, bool notify)
{
    // Neuen Button auswählen
    ctrlButton* button = GetCtrl<ctrlButton>(selection);
    assert(button);
    switch(select_type)
    {
        case ILLUMINATE: button->SetIlluminated(true); break;
        case CHECK:      button->SetCheck(true);       break;
        case SHOW:       button->SetVisible(false);     break;
    }

    this->selection.insert(selection);

    if(notify && parent)
        parent->Msg_OptionGroupChange(GetID(), selection);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Entfernt die Selektion eines Buttons aus der Gruppe.
 *
 *  @author jh
 */
void ctrlMultiSelectGroup::RemoveSelection(unsigned short selection, bool notify)
{
    // Neuen Button auswählen
    ctrlButton* button = GetCtrl<ctrlButton>(selection);
    assert(button);
    switch(select_type)
    {
        case ILLUMINATE: button->SetIlluminated(false); break;
        case CHECK:      button->SetCheck(false);       break;
        case SHOW:       button->SetVisible(true);        break;
    }

    this->selection.erase(selection);

    if(notify && parent)
        parent->Msg_OptionGroupChange(GetID(), selection);
}


///////////////////////////////////////////////////////////////////////////////
/**
 *  Wechselt zwischen selektiert/nicht selektiert
 *
 *  @author jh
 */
void ctrlMultiSelectGroup::ToggleSelection(unsigned short selection, bool notify)
{
    if (IsSelected(selection))
        RemoveSelection(selection, notify);
    else
        AddSelection(selection, notify);
}

bool ctrlMultiSelectGroup::IsSelected(unsigned short selection) const
{
    return (this->selection.count(selection) == 1);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author jh
 */
void ctrlMultiSelectGroup::Msg_ButtonClick(const unsigned int ctrl_id)
{
    ToggleSelection(ctrl_id, true);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author OLiver
 */
bool ctrlMultiSelectGroup::Msg_LeftDown(const MouseCoords& mc)
{
    return RelayMouseMessage(&Window::Msg_LeftDown, mc);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author OLiver
 */
bool ctrlMultiSelectGroup::Msg_LeftUp(const MouseCoords& mc)
{
    return RelayMouseMessage(&Window::Msg_LeftUp, mc);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author Divan
 */
bool ctrlMultiSelectGroup::Msg_WheelUp(const MouseCoords& mc)
{
    return RelayMouseMessage(&Window::Msg_WheelUp, mc);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author Divan
 */
bool ctrlMultiSelectGroup::Msg_WheelDown(const MouseCoords& mc)
{
    return RelayMouseMessage(&Window::Msg_WheelDown, mc);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author OLiver
 */
bool ctrlMultiSelectGroup::Msg_MouseMove(const MouseCoords& mc)
{
    return RelayMouseMessage(&Window::Msg_MouseMove, mc);
}
