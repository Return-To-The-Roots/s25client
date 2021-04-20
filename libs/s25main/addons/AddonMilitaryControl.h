// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "AddonBool.h"

/**
 *  Addon adds a 'order home' command button to military buildings. Pressing this button will send all soldiers of the
 *highest rank available to the next warehouse the command was first added so the ai wouldnt have to change the military
 *settings all the time to use the coins more efficiently - but with this addon players can use the command as well
 */
class AddonMilitaryControl : public AddonBool
{
public:
    AddonMilitaryControl()
        : AddonBool(AddonId::MILITARY_CONTROL, AddonGroup::GamePlay | AddonGroup::Military, _("Military Control"),
                    _("Adds the 'send home' button to military buildings.\n"
                      "Pressing this button will send all soldiers of the highest available rank to a warehouse (at "
                      "least 1 Soldier will "
                      "remain in the building)"))
    {}
};
