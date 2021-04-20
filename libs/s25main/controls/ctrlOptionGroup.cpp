// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "ctrlOptionGroup.h"

ctrlOptionGroup::ctrlOptionGroup(Window* parent, unsigned id, GroupSelectType select_type)
    : ctrlGroup(parent, id), select_type(select_type)
{}

/**
 *  wählt einen Button aus der Gruppe aus.
 */
void ctrlOptionGroup::SetSelection(unsigned selection, bool notify)
{
    // Aktuellen ausgewählten Button wieder normal machen
    if(this->selection_)
    {
        auto* button = GetCtrl<ctrlButton>(this->selection_.get());
        RTTR_Assert(button);
        switch(select_type)
        {
            case GroupSelectType::Illuminate: button->SetIlluminated(false); break;
            case GroupSelectType::Check: button->SetChecked(false); break;
            case GroupSelectType::Show: button->SetVisible(true); break;
        }
    }

    // Neuen Button auswählen
    auto* button = GetCtrl<ctrlButton>(selection);
    RTTR_Assert(button);
    switch(select_type)
    {
        case GroupSelectType::Illuminate: button->SetIlluminated(true); break;
        case GroupSelectType::Check: button->SetChecked(true); break;
        case GroupSelectType::Show: button->SetVisible(false); break;
    }

    this->selection_ = selection;

    if(notify && GetParent())
        GetParent()->Msg_OptionGroupChange(GetID(), selection);
}

unsigned ctrlOptionGroup::GetSelection() const
{
    if(!selection_)
        throw std::logic_error("No button selected in option group");
    return selection_.get();
}

void ctrlOptionGroup::Msg_ButtonClick(const unsigned ctrl_id)
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
