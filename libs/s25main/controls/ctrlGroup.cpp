// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "ctrlGroup.h"

struct MouseCoords;
struct KeyEvent;

ctrlGroup::ctrlGroup(Window* parent, unsigned id) : Window(parent, id, DrawPoint(0, 0)) {}

void ctrlGroup::msgButtonClick(const unsigned ctrl_id)
{
    GetParent()->msgGroupButtonClick(this->GetID(), ctrl_id);
}

void ctrlGroup::msgEditEnter(const unsigned ctrl_id)
{
    GetParent()->msgGroupEditEnter(this->GetID(), ctrl_id);
}

void ctrlGroup::msgEditChange(const unsigned ctrl_id)
{
    GetParent()->msgGroupEditChange(this->GetID(), ctrl_id);
}

void ctrlGroup::msgTabChange(const unsigned ctrl_id, const unsigned short tab_id)
{
    GetParent()->msgGroupTabChange(this->GetID(), ctrl_id, tab_id);
}

void ctrlGroup::msgListSelectItem(const unsigned ctrl_id, const int selection)
{
    GetParent()->msgGroupListSelectItem(this->GetID(), ctrl_id, selection);
}

void ctrlGroup::msgComboSelectItem(const unsigned ctrl_id, const unsigned selection)
{
    GetParent()->msgGroupComboSelectItem(this->GetID(), ctrl_id, selection);
}

void ctrlGroup::msgCheckboxChange(const unsigned ctrl_id, const bool checked)
{
    GetParent()->msgGroupCheckboxChange(this->GetID(), ctrl_id, checked);
}

void ctrlGroup::msgProgressChange(const unsigned ctrl_id, const unsigned short position)
{
    GetParent()->msgGroupProgressChange(this->GetID(), ctrl_id, position);
}

void ctrlGroup::msgScrollShow(const unsigned ctrl_id, const bool visible)
{
    GetParent()->msgGroupScrollShow(this->GetID(), ctrl_id, visible);
}

void ctrlGroup::msgOptionGroupChange(const unsigned ctrl_id, const unsigned selection)
{
    GetParent()->msgGroupOptionGroupChange(this->GetID(), ctrl_id, selection);
}

void ctrlGroup::msgTimer(const unsigned ctrl_id)
{
    GetParent()->msgGroupTimer(this->GetID(), ctrl_id);
}

void ctrlGroup::msgTableSelectItem(const unsigned ctrl_id, const std::optional<unsigned>& selection)
{
    GetParent()->msgGroupTableSelectItem(this->GetID(), ctrl_id, selection);
}

void ctrlGroup::msgTableRightButton(const unsigned ctrl_id, const std::optional<unsigned>& selection)
{
    GetParent()->msgGroupTableRightButton(this->GetID(), ctrl_id, selection);
}

void ctrlGroup::msgTableLeftButton(const unsigned ctrl_id, const std::optional<unsigned>& selection)
{
    GetParent()->msgGroupTableLeftButton(this->GetID(), ctrl_id, selection);
}

bool ctrlGroup::msgLeftDown(const MouseCoords& mc)
{
    return RelayMouseMessage(&Window::msgLeftDown, mc);
}

bool ctrlGroup::msgRightDown(const MouseCoords& mc)
{
    return RelayMouseMessage(&Window::msgRightDown, mc);
}

bool ctrlGroup::msgLeftUp(const MouseCoords& mc)
{
    return RelayMouseMessage(&Window::msgLeftUp, mc);
}

bool ctrlGroup::msgRightUp(const MouseCoords& mc)
{
    return RelayMouseMessage(&Window::msgRightUp, mc);
}

bool ctrlGroup::msgWheelUp(const MouseCoords& mc)
{
    return RelayMouseMessage(&Window::msgWheelUp, mc);
}

bool ctrlGroup::msgWheelDown(const MouseCoords& mc)
{
    return RelayMouseMessage(&Window::msgWheelDown, mc);
}

bool ctrlGroup::msgMouseMove(const MouseCoords& mc)
{
    return RelayMouseMessage(&Window::msgMouseMove, mc);
}

bool ctrlGroup::msgKeyDown(const KeyEvent& ke)
{
    return RelayKeyboardMessage(&Window::msgKeyDown, ke);
}

void ctrlGroup::msgGroupButtonClick(const unsigned /*group_id*/, const unsigned ctrl_id)
{
    GetParent()->msgGroupButtonClick(this->GetID(), ctrl_id);
}

void ctrlGroup::msgGroupEditEnter(const unsigned /*group_id*/, const unsigned ctrl_id)
{
    GetParent()->msgGroupEditEnter(this->GetID(), ctrl_id);
}

void ctrlGroup::msgGroupEditChange(const unsigned /*group_id*/, const unsigned ctrl_id)
{
    GetParent()->msgGroupEditChange(this->GetID(), ctrl_id);
}

void ctrlGroup::msgGroupTabChange(const unsigned /*group_id*/, const unsigned ctrl_id, const unsigned short tab_id)
{
    GetParent()->msgGroupTabChange(this->GetID(), ctrl_id, tab_id);
}

void ctrlGroup::msgGroupListSelectItem(const unsigned /*group_id*/, const unsigned ctrl_id, const int selection)
{
    GetParent()->msgGroupListSelectItem(this->GetID(), ctrl_id, selection);
}

void ctrlGroup::msgGroupComboSelectItem(const unsigned /*group_id*/, const unsigned ctrl_id, const unsigned selection)
{
    GetParent()->msgGroupComboSelectItem(this->GetID(), ctrl_id, selection);
}

void ctrlGroup::msgGroupCheckboxChange(const unsigned /*group_id*/, const unsigned ctrl_id, const bool checked)
{
    GetParent()->msgGroupCheckboxChange(this->GetID(), ctrl_id, checked);
}

void ctrlGroup::msgGroupProgressChange(const unsigned /*group_id*/, const unsigned ctrl_id,
                                       const unsigned short position)
{
    GetParent()->msgGroupProgressChange(this->GetID(), ctrl_id, position);
}

void ctrlGroup::msgGroupScrollShow(const unsigned /*group_id*/, const unsigned ctrl_id, const bool visible)
{
    GetParent()->msgGroupScrollShow(this->GetID(), ctrl_id, visible);
}

void ctrlGroup::msgGroupOptionGroupChange(const unsigned /*group_id*/, const unsigned ctrl_id, unsigned selection)
{
    GetParent()->msgGroupOptionGroupChange(this->GetID(), ctrl_id, selection);
}

void ctrlGroup::msgGroupTimer(const unsigned /*group_id*/, const unsigned ctrl_id)
{
    GetParent()->msgGroupTimer(this->GetID(), ctrl_id);
}

void ctrlGroup::msgGroupTableSelectItem(const unsigned /*group_id*/, const unsigned ctrl_id,
                                        const std::optional<unsigned>& selection)
{
    GetParent()->msgGroupTableSelectItem(this->GetID(), ctrl_id, selection);
}

void ctrlGroup::msgGroupTableRightButton(const unsigned /*group_id*/, const unsigned ctrl_id,
                                         const std::optional<unsigned>& selection)
{
    GetParent()->msgGroupTableRightButton(this->GetID(), ctrl_id, selection);
}

void ctrlGroup::msgGroupTableLeftButton(const unsigned /*group_id*/, const unsigned ctrl_id,
                                        const std::optional<unsigned>& selection)
{
    GetParent()->msgGroupTableLeftButton(this->GetID(), ctrl_id, selection);
}
