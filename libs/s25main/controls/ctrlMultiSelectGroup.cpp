// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "ctrlMultiSelectGroup.h"
struct MouseCoords;

ctrlMultiSelectGroup::ctrlMultiSelectGroup(Window* parent, unsigned id, GroupSelectType select_type)
    : ctrlGroup(parent, id), select_type(select_type)
{}

/**
 *  Selektiert einen neuen Button aus der Gruppe.
 */
void ctrlMultiSelectGroup::AddSelection(unsigned selection, bool notify)
{
    // Neuen Button auswählen
    auto* button = getCtrl<ctrlButton>(selection);
    RTTR_Assert(button);
    switch(select_type)
    {
        case GroupSelectType::Illuminate: button->setIlluminated(true); break;
        case GroupSelectType::Check: button->setChecked(true); break;
        case GroupSelectType::Show: button->setVisible(false); break;
    }

    this->selectedItems_.insert(selection);

    if(notify && getParent())
        getParent()->msgOptionGroupChange(getID(), selection);
}

/**
 *  Entfernt die Selektion eines Buttons aus der Gruppe.
 */
void ctrlMultiSelectGroup::RemoveSelection(unsigned selection, bool notify)
{
    // Neuen Button auswählen
    auto* button = getCtrl<ctrlButton>(selection);
    RTTR_Assert(button);
    switch(select_type)
    {
        case GroupSelectType::Illuminate: button->setIlluminated(false); break;
        case GroupSelectType::Check: button->setChecked(false); break;
        case GroupSelectType::Show: button->setVisible(true); break;
    }

    this->selectedItems_.erase(selection);

    if(notify && getParent())
        getParent()->msgOptionGroupChange(getID(), selection);
}

/**
 *  Wechselt zwischen selektiert/nicht selektiert
 */
void ctrlMultiSelectGroup::ToggleSelection(unsigned selection, bool notify)
{
    if(isSelected(selection))
        RemoveSelection(selection, notify);
    else
        AddSelection(selection, notify);
}

bool ctrlMultiSelectGroup::isSelected(unsigned selection) const
{
    return (this->selectedItems_.count(selection) == 1);
}

void ctrlMultiSelectGroup::msgButtonClick(const unsigned ctrl_id)
{
    ToggleSelection(ctrl_id, true);
}

bool ctrlMultiSelectGroup::msgLeftDown(const MouseCoords& mc)
{
    return RelayMouseMessage(&Window::msgLeftDown, mc);
}

bool ctrlMultiSelectGroup::msgLeftUp(const MouseCoords& mc)
{
    return RelayMouseMessage(&Window::msgLeftUp, mc);
}

bool ctrlMultiSelectGroup::msgWheelUp(const MouseCoords& mc)
{
    return RelayMouseMessage(&Window::msgWheelUp, mc);
}

bool ctrlMultiSelectGroup::msgWheelDown(const MouseCoords& mc)
{
    return RelayMouseMessage(&Window::msgWheelDown, mc);
}

bool ctrlMultiSelectGroup::msgMouseMove(const MouseCoords& mc)
{
    return RelayMouseMessage(&Window::msgMouseMove, mc);
}
