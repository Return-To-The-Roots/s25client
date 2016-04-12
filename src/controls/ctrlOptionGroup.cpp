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
#include "ctrlOptionGroup.h"

// Include last!
#include "DebugNew.h" // IWYU pragma: keep
class MouseCoords;

ctrlOptionGroup::ctrlOptionGroup(Window* parent,
                                 unsigned int id,
                                 int select_type,
                                 bool scale)
    : ctrlGroup(parent, id, scale),
      selection_(0xFFFF), select_type(select_type)
{
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Zeichenmethode.
 *
 *  @author OLiver
 */
bool ctrlOptionGroup::Draw_()
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
    if(this->selection_ != 0xFFFF)
    {
        ctrlButton* button = GetCtrl<ctrlButton>(this->selection_);
        RTTR_Assert(button);
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
        RTTR_Assert(button);
        switch(select_type)
        {
            case ILLUMINATE: button->SetIlluminated(true); break;
            case CHECK:      button->SetCheck(true);       break;
            case SHOW:       button->SetVisible(false);     break;
        }
    }

    this->selection_ = selection;

    if(notify && parent_)
        parent_->Msg_OptionGroupChange(GetID(), selection);
}

void ctrlOptionGroup::Msg_ButtonClick(const unsigned int ctrl_id)
{
    SetSelection(ctrl_id, true);
}

bool ctrlOptionGroup::Msg_LeftDown(const MouseCoords& mc)
{
    return RelayMouseMessage(&Window::Msg_LeftDown, mc);
}

bool ctrlOptionGroup::Msg_LeftUp(const MouseCoords& mc)
{
    return RelayMouseMessage(&Window::Msg_LeftUp, mc);
}

bool ctrlOptionGroup::Msg_WheelUp(const MouseCoords& mc)
{
    return RelayMouseMessage(&Window::Msg_WheelUp, mc);
}

bool ctrlOptionGroup::Msg_WheelDown(const MouseCoords& mc)
{
    return RelayMouseMessage(&Window::Msg_WheelDown, mc);
}

bool ctrlOptionGroup::Msg_MouseMove(const MouseCoords& mc)
{
    return RelayMouseMessage(&Window::Msg_MouseMove, mc);
}
