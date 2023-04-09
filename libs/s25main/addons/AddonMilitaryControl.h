// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "AddonBool.h"

class AddonMilitaryControl : public AddonList
{
public:
    AddonMilitaryControl()
        : AddonList(AddonId::MILITARY_CONTROL, AddonGroup::GamePlay | AddonGroup::Military, _("Military Control"),
                    _("Adds troop controls to military buildings.\n"
                      "Minimal: Adds the 'send home' button which will send all soldiers of the highest available rank to a warehouse.\n"
                      "Full: Allows players to control in each military building how many soldiers of each rank should be stationed there."),
                    {
                      _("None"),
                      _("Minimal Control"),
                      _("Full Control"),
                    })
    {}
};
