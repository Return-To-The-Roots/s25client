// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "ctrlGroup.h"

struct MouseCoords;
struct KeyEvent;

ctrlGroup::ctrlGroup(Window* parent, unsigned id) : Window(parent, id, DrawPoint(0, 0)) {}

void ctrlGroup::msgButtonClick(const unsigned ctrl_id)
{
    getParent()->msgGroupButtonClick(this->getID(), ctrl_id);
}

void ctrlGroup::msgEditEnter(const unsigned ctrl_id)
{
    getParent()->msgGroupEditEnter(this->getID(), ctrl_id);
}

void ctrlGroup::msgEditChange(const unsigned ctrl_id)
{
    getParent()->msgGroupEditChange(this->getID(), ctrl_id);
}

void ctrlGroup::msgTabChange(const unsigned ctrl_id, const unsigned short tabId)
{
    getParent()->msgGroupTabChange(this->getID(), ctrl_id, tabId);
}

void ctrlGroup::msgListSelectItem(const unsigned ctrl_id, const int selection)
{
    getParent()->msgGroupListSelectItem(this->getID(), ctrl_id, selection);
}

void ctrlGroup::msgComboSelectItem(const unsigned ctrl_id, const unsigned selection)
{
    getParent()->msgGroupComboSelectItem(this->getID(), ctrl_id, selection);
}

void ctrlGroup::msgCheckboxChange(const unsigned ctrl_id, const bool checked)
{
    getParent()->msgGroupCheckboxChange(this->getID(), ctrl_id, checked);
}

void ctrlGroup::msgProgressChange(const unsigned ctrl_id, const unsigned short position)
{
    getParent()->msgGroupProgressChange(this->getID(), ctrl_id, position);
}

void ctrlGroup::msgScrollShow(const unsigned ctrl_id, const bool visible)
{
    getParent()->msgGroupScrollShow(this->getID(), ctrl_id, visible);
}

void ctrlGroup::msgOptionGroupChange(const unsigned ctrl_id, const unsigned selection)
{
    getParent()->msgGroupOptionGroupChange(this->getID(), ctrl_id, selection);
}

void ctrlGroup::msgTimer(const unsigned ctrl_id)
{
    getParent()->msgGroupTimer(this->getID(), ctrl_id);
}

void ctrlGroup::msgTableSelectItem(const unsigned ctrl_id, const std::optional<unsigned>& selection)
{
    getParent()->msgGroupTableSelectItem(this->getID(), ctrl_id, selection);
}

void ctrlGroup::msgTableRightButton(const unsigned ctrl_id, const std::optional<unsigned>& selection)
{
    getParent()->msgGroupTableRightButton(this->getID(), ctrl_id, selection);
}

void ctrlGroup::msgTableLeftButton(const unsigned ctrl_id, const std::optional<unsigned>& selection)
{
    getParent()->msgGroupTableLeftButton(this->getID(), ctrl_id, selection);
}

bool ctrlGroup::msgLeftDown(const MouseCoords& mc)
{
    return relayMouseMessage(&Window::msgLeftDown, mc);
}

bool ctrlGroup::msgRightDown(const MouseCoords& mc)
{
    return relayMouseMessage(&Window::msgRightDown, mc);
}

bool ctrlGroup::msgLeftUp(const MouseCoords& mc)
{
    return relayMouseMessage(&Window::msgLeftUp, mc);
}

bool ctrlGroup::msgRightUp(const MouseCoords& mc)
{
    return relayMouseMessage(&Window::msgRightUp, mc);
}

bool ctrlGroup::msgWheelUp(const MouseCoords& mc)
{
    return relayMouseMessage(&Window::msgWheelUp, mc);
}

bool ctrlGroup::msgWheelDown(const MouseCoords& mc)
{
    return relayMouseMessage(&Window::msgWheelDown, mc);
}

bool ctrlGroup::msgMouseMove(const MouseCoords& mc)
{
    return relayMouseMessage(&Window::msgMouseMove, mc);
}

bool ctrlGroup::msgKeyDown(const KeyEvent& ke)
{
    return relayKeyboardMessage(&Window::msgKeyDown, ke);
}

void ctrlGroup::msgGroupButtonClick(const unsigned /*group_id*/, const unsigned ctrl_id)
{
    getParent()->msgGroupButtonClick(this->getID(), ctrl_id);
}

void ctrlGroup::msgGroupEditEnter(const unsigned /*group_id*/, const unsigned ctrl_id)
{
    getParent()->msgGroupEditEnter(this->getID(), ctrl_id);
}

void ctrlGroup::msgGroupEditChange(const unsigned /*group_id*/, const unsigned ctrl_id)
{
    getParent()->msgGroupEditChange(this->getID(), ctrl_id);
}

void ctrlGroup::msgGroupTabChange(const unsigned /*group_id*/, const unsigned ctrl_id, const unsigned short tabId)
{
    getParent()->msgGroupTabChange(this->getID(), ctrl_id, tabId);
}

void ctrlGroup::msgGroupListSelectItem(const unsigned /*group_id*/, const unsigned ctrl_id, const int selection)
{
    getParent()->msgGroupListSelectItem(this->getID(), ctrl_id, selection);
}

void ctrlGroup::msgGroupComboSelectItem(const unsigned /*group_id*/, const unsigned ctrl_id, const unsigned selection)
{
    getParent()->msgGroupComboSelectItem(this->getID(), ctrl_id, selection);
}

void ctrlGroup::msgGroupCheckboxChange(const unsigned /*group_id*/, const unsigned ctrl_id, const bool checked)
{
    getParent()->msgGroupCheckboxChange(this->getID(), ctrl_id, checked);
}

void ctrlGroup::msgGroupProgressChange(const unsigned /*group_id*/, const unsigned ctrl_id,
                                       const unsigned short position)
{
    getParent()->msgGroupProgressChange(this->getID(), ctrl_id, position);
}

void ctrlGroup::msgGroupScrollShow(const unsigned /*group_id*/, const unsigned ctrl_id, const bool visible)
{
    getParent()->msgGroupScrollShow(this->getID(), ctrl_id, visible);
}

void ctrlGroup::msgGroupOptionGroupChange(const unsigned /*group_id*/, const unsigned ctrl_id, unsigned selection)
{
    getParent()->msgGroupOptionGroupChange(this->getID(), ctrl_id, selection);
}

void ctrlGroup::msgGroupTimer(const unsigned /*group_id*/, const unsigned ctrl_id)
{
    getParent()->msgGroupTimer(this->getID(), ctrl_id);
}

void ctrlGroup::msgGroupTableSelectItem(const unsigned /*group_id*/, const unsigned ctrl_id,
                                        const std::optional<unsigned>& selection)
{
    getParent()->msgGroupTableSelectItem(this->getID(), ctrl_id, selection);
}

void ctrlGroup::msgGroupTableRightButton(const unsigned /*group_id*/, const unsigned ctrl_id,
                                         const std::optional<unsigned>& selection)
{
    getParent()->msgGroupTableRightButton(this->getID(), ctrl_id, selection);
}

void ctrlGroup::msgGroupTableLeftButton(const unsigned /*group_id*/, const unsigned ctrl_id,
                                        const std::optional<unsigned>& selection)
{
    getParent()->msgGroupTableLeftButton(this->getID(), ctrl_id, selection);
}
