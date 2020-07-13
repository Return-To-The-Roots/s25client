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

#include "ctrlGroup.h"

class MouseCoords;
struct KeyEvent;

ctrlGroup::ctrlGroup(Window* parent, unsigned id) : Window(parent, id, DrawPoint(0, 0)) {}

void ctrlGroup::Msg_ButtonClick(const unsigned ctrl_id)
{
    GetParent()->Msg_Group_ButtonClick(this->GetID(), ctrl_id);
}

void ctrlGroup::Msg_EditEnter(const unsigned ctrl_id)
{
    GetParent()->Msg_Group_EditEnter(this->GetID(), ctrl_id);
}

void ctrlGroup::Msg_EditChange(const unsigned ctrl_id)
{
    GetParent()->Msg_Group_EditChange(this->GetID(), ctrl_id);
}

void ctrlGroup::Msg_TabChange(const unsigned ctrl_id, const unsigned short tab_id)
{
    GetParent()->Msg_Group_TabChange(this->GetID(), ctrl_id, tab_id);
}

void ctrlGroup::Msg_ListSelectItem(const unsigned ctrl_id, const int selection)
{
    GetParent()->Msg_Group_ListSelectItem(this->GetID(), ctrl_id, selection);
}

void ctrlGroup::Msg_ComboSelectItem(const unsigned ctrl_id, const int selection)
{
    GetParent()->Msg_Group_ComboSelectItem(this->GetID(), ctrl_id, selection);
}

void ctrlGroup::Msg_CheckboxChange(const unsigned ctrl_id, const bool checked)
{
    GetParent()->Msg_Group_CheckboxChange(this->GetID(), ctrl_id, checked);
}

void ctrlGroup::Msg_ProgressChange(const unsigned ctrl_id, const unsigned short position)
{
    GetParent()->Msg_Group_ProgressChange(this->GetID(), ctrl_id, position);
}

void ctrlGroup::Msg_ScrollShow(const unsigned ctrl_id, const bool visible)
{
    GetParent()->Msg_Group_ScrollShow(this->GetID(), ctrl_id, visible);
}

void ctrlGroup::Msg_OptionGroupChange(const unsigned ctrl_id, const unsigned selection)
{
    GetParent()->Msg_Group_OptionGroupChange(this->GetID(), ctrl_id, selection);
}

void ctrlGroup::Msg_Timer(const unsigned ctrl_id)
{
    GetParent()->Msg_Group_Timer(this->GetID(), ctrl_id);
}

void ctrlGroup::Msg_TableSelectItem(const unsigned ctrl_id, const int selection)
{
    GetParent()->Msg_Group_TableSelectItem(this->GetID(), ctrl_id, selection);
}

void ctrlGroup::Msg_TableRightButton(const unsigned ctrl_id, const int selection)
{
    GetParent()->Msg_Group_TableRightButton(this->GetID(), ctrl_id, selection);
}

void ctrlGroup::Msg_TableLeftButton(const unsigned ctrl_id, const int selection)
{
    GetParent()->Msg_Group_TableLeftButton(this->GetID(), ctrl_id, selection);
}

bool ctrlGroup::Msg_LeftDown(const MouseCoords& mc)
{
    return RelayMouseMessage(&Window::Msg_LeftDown, mc);
}

bool ctrlGroup::Msg_RightDown(const MouseCoords& mc)
{
    return RelayMouseMessage(&Window::Msg_RightDown, mc);
}

bool ctrlGroup::Msg_LeftUp(const MouseCoords& mc)
{
    return RelayMouseMessage(&Window::Msg_LeftUp, mc);
}

bool ctrlGroup::Msg_RightUp(const MouseCoords& mc)
{
    return RelayMouseMessage(&Window::Msg_RightUp, mc);
}

bool ctrlGroup::Msg_WheelUp(const MouseCoords& mc)
{
    return RelayMouseMessage(&Window::Msg_WheelUp, mc);
}

bool ctrlGroup::Msg_WheelDown(const MouseCoords& mc)
{
    return RelayMouseMessage(&Window::Msg_WheelDown, mc);
}

bool ctrlGroup::Msg_MouseMove(const MouseCoords& mc)
{
    return RelayMouseMessage(&Window::Msg_MouseMove, mc);
}

bool ctrlGroup::Msg_KeyDown(const KeyEvent& ke)
{
    return RelayKeyboardMessage(&Window::Msg_KeyDown, ke);
}

void ctrlGroup::Msg_Group_ButtonClick(const unsigned /*group_id*/, const unsigned ctrl_id)
{
    GetParent()->Msg_Group_ButtonClick(this->GetID(), ctrl_id);
}

void ctrlGroup::Msg_Group_EditEnter(const unsigned /*group_id*/, const unsigned ctrl_id)
{
    GetParent()->Msg_Group_EditEnter(this->GetID(), ctrl_id);
}

void ctrlGroup::Msg_Group_EditChange(const unsigned /*group_id*/, const unsigned ctrl_id)
{
    GetParent()->Msg_Group_EditChange(this->GetID(), ctrl_id);
}

void ctrlGroup::Msg_Group_TabChange(const unsigned /*group_id*/, const unsigned ctrl_id, const unsigned short tab_id)
{
    GetParent()->Msg_Group_TabChange(this->GetID(), ctrl_id, tab_id);
}

void ctrlGroup::Msg_Group_ListSelectItem(const unsigned /*group_id*/, const unsigned ctrl_id, const int selection)
{
    GetParent()->Msg_Group_ListSelectItem(this->GetID(), ctrl_id, selection);
}

void ctrlGroup::Msg_Group_ComboSelectItem(const unsigned /*group_id*/, const unsigned ctrl_id, const int selection)
{
    GetParent()->Msg_Group_ComboSelectItem(this->GetID(), ctrl_id, selection);
}

void ctrlGroup::Msg_Group_CheckboxChange(const unsigned /*group_id*/, const unsigned ctrl_id, const bool checked)
{
    GetParent()->Msg_Group_CheckboxChange(this->GetID(), ctrl_id, checked);
}

void ctrlGroup::Msg_Group_ProgressChange(const unsigned /*group_id*/, const unsigned ctrl_id, const unsigned short position)
{
    GetParent()->Msg_Group_ProgressChange(this->GetID(), ctrl_id, position);
}

void ctrlGroup::Msg_Group_ScrollShow(const unsigned /*group_id*/, const unsigned ctrl_id, const bool visible)
{
    GetParent()->Msg_Group_ScrollShow(this->GetID(), ctrl_id, visible);
}

void ctrlGroup::Msg_Group_OptionGroupChange(const unsigned /*group_id*/, const unsigned ctrl_id, unsigned selection)
{
    GetParent()->Msg_Group_OptionGroupChange(this->GetID(), ctrl_id, selection);
}

void ctrlGroup::Msg_Group_Timer(const unsigned /*group_id*/, const unsigned ctrl_id)
{
    GetParent()->Msg_Group_Timer(this->GetID(), ctrl_id);
}

void ctrlGroup::Msg_Group_TableSelectItem(const unsigned /*group_id*/, const unsigned ctrl_id, const int selection)
{
    GetParent()->Msg_Group_TableSelectItem(this->GetID(), ctrl_id, selection);
}

void ctrlGroup::Msg_Group_TableRightButton(const unsigned /*group_id*/, const unsigned ctrl_id, const int selection)
{
    GetParent()->Msg_Group_TableRightButton(this->GetID(), ctrl_id, selection);
}

void ctrlGroup::Msg_Group_TableLeftButton(const unsigned /*group_id*/, const unsigned ctrl_id, const int selection)
{
    GetParent()->Msg_Group_TableLeftButton(this->GetID(), ctrl_id, selection);
}
