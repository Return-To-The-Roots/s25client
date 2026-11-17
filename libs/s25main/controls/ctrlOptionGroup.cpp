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
void ctrlOptionGroup::setSelection(unsigned selection, bool notify)
{
    // Aktuellen ausgewählten Button wieder normal machen
    if(this->selection_)
    {
        auto* button = getCtrl<ctrlButton>(*this->selection_);
        RTTR_Assert(button);
        switch(select_type)
        {
            case GroupSelectType::Illuminate: button->setIlluminated(false); break;
            case GroupSelectType::Check: button->setChecked(false); break;
            case GroupSelectType::show: button->setVisible(true); break;
        }
    }

    // Neuen Button auswählen
    auto* button = getCtrl<ctrlButton>(selection);
    RTTR_Assert(button);
    switch(select_type)
    {
        case GroupSelectType::Illuminate: button->setIlluminated(true); break;
        case GroupSelectType::Check: button->setChecked(true); break;
        case GroupSelectType::show: button->setVisible(false); break;
    }

    this->selection_ = selection;

    if(notify && getParent())
        getParent()->msgOptionGroupChange(getID(), selection);
}

unsigned ctrlOptionGroup::getSelection() const
{
    if(!selection_)
        throw std::logic_error("No button selected in option group");
    return *selection_;
}

void ctrlOptionGroup::msgButtonClick(const unsigned ctrl_id)
{
    setSelection(ctrl_id, true);
}

bool ctrlOptionGroup::msgLeftDown(const MouseCoords& mc)
{
    return relayMouseMessage(&Window::msgLeftDown, mc);
}

bool ctrlOptionGroup::msgLeftUp(const MouseCoords& mc)
{
    return relayMouseMessage(&Window::msgLeftUp, mc);
}

bool ctrlOptionGroup::msgWheelUp(const MouseCoords& mc)
{
    return relayMouseMessage(&Window::msgWheelUp, mc);
}

bool ctrlOptionGroup::msgWheelDown(const MouseCoords& mc)
{
    return relayMouseMessage(&Window::msgWheelDown, mc);
}

bool ctrlOptionGroup::msgMouseMove(const MouseCoords& mc)
{
    return relayMouseMessage(&Window::msgMouseMove, mc);
}
