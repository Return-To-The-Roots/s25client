// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "ctrlMultiSelectGroup.h"
class MouseCoords;

ctrlMultiSelectGroup::ctrlMultiSelectGroup(Window* parent, unsigned id, GroupSelectType select_type)
    : ctrlGroup(parent, id), select_type(select_type)
{}

/**
 *  Selektiert einen neuen Button aus der Gruppe.
 */
void ctrlMultiSelectGroup::AddSelection(unsigned selection, bool notify)
{
    // Neuen Button auswählen
    auto* button = GetCtrl<ctrlButton>(selection);
    RTTR_Assert(button);
    switch(select_type)
    {
        case GroupSelectType::Illuminate: button->SetIlluminated(true); break;
        case GroupSelectType::Check: button->SetChecked(true); break;
        case GroupSelectType::Show: button->SetVisible(false); break;
    }

    this->selectedItems_.insert(selection);

    if(notify && GetParent())
        GetParent()->Msg_OptionGroupChange(GetID(), selection);
}

/**
 *  Entfernt die Selektion eines Buttons aus der Gruppe.
 */
void ctrlMultiSelectGroup::RemoveSelection(unsigned selection, bool notify)
{
    // Neuen Button auswählen
    auto* button = GetCtrl<ctrlButton>(selection);
    RTTR_Assert(button);
    switch(select_type)
    {
        case GroupSelectType::Illuminate: button->SetIlluminated(false); break;
        case GroupSelectType::Check: button->SetChecked(false); break;
        case GroupSelectType::Show: button->SetVisible(true); break;
    }

    this->selectedItems_.erase(selection);

    if(notify && GetParent())
        GetParent()->Msg_OptionGroupChange(GetID(), selection);
}

/**
 *  Wechselt zwischen selektiert/nicht selektiert
 */
void ctrlMultiSelectGroup::ToggleSelection(unsigned selection, bool notify)
{
    if(IsSelected(selection))
        RemoveSelection(selection, notify);
    else
        AddSelection(selection, notify);
}

bool ctrlMultiSelectGroup::IsSelected(unsigned selection) const
{
    return (this->selectedItems_.count(selection) == 1);
}

void ctrlMultiSelectGroup::Msg_ButtonClick(const unsigned ctrl_id)
{
    ToggleSelection(ctrl_id, true);
}

bool ctrlMultiSelectGroup::Msg_LeftDown(const MouseCoords& mc)
{
    return RelayMouseMessage(&Window::Msg_LeftDown, mc);
}

bool ctrlMultiSelectGroup::Msg_LeftUp(const MouseCoords& mc)
{
    return RelayMouseMessage(&Window::Msg_LeftUp, mc);
}

bool ctrlMultiSelectGroup::Msg_WheelUp(const MouseCoords& mc)
{
    return RelayMouseMessage(&Window::Msg_WheelUp, mc);
}

bool ctrlMultiSelectGroup::Msg_WheelDown(const MouseCoords& mc)
{
    return RelayMouseMessage(&Window::Msg_WheelDown, mc);
}

bool ctrlMultiSelectGroup::Msg_MouseMove(const MouseCoords& mc)
{
    return RelayMouseMessage(&Window::Msg_MouseMove, mc);
}
