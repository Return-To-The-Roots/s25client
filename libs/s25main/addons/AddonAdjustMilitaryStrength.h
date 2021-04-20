// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "AddonList.h"
#include "mygettext/mygettext.h"

/**
 *  Addon for changing military strength of higher ranks.
 *
 *  Default behaviour: (Maximum strength)
 *  - Dice rolling
 *  - Rank+3
 *
 *  Medium strength:
 *  - Dice rolling
 *  - Rank + 8
 *
 *  Minimum strength
 *  - Dice rolling
 *  - All ranks get same dice
 */
class AddonAdjustMilitaryStrength : public AddonList
{
public:
    AddonAdjustMilitaryStrength()
        : AddonList(AddonId::ADJUST_MILITARY_STRENGTH, AddonGroup::Military, _("Adjust military strength"),
                    _("Modify the strength increase of military ranks"),
                    {
                      _("Maximum strength"),
                      _("Medium strength"),
                      _("Minimum strength"),
                    },
                    1)
    {}
};
