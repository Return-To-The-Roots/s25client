// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "AddonBool.h"

/**
 *  Addon adds shows the hitpoints of a soldier above his picture in military buildings.
 */
class AddonMilitaryHitpoints : public AddonBool
{
public:
    AddonMilitaryHitpoints()
        : AddonBool(AddonId::MILITARY_HITPOINTS, AddonGroup::GamePlay | AddonGroup::Military, _("Military Hitpoints"),
                    _("Display the hitpoints of units in military buildings."))
    {}
};
