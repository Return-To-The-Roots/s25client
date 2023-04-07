// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "AddonBool.h"

class AddonMilitaryControl : public AddonBool
{
public:
    AddonMilitaryControl()
        : AddonBool(AddonId::MILITARY_CONTROL, AddonGroup::GamePlay | AddonGroup::Military, _("Military Control"),
                    _("Adds troop controls to military buildings.\n"
                      "Allows players to control in each military building how many soldiers of each rank should be stationed there."))
    {}
};
