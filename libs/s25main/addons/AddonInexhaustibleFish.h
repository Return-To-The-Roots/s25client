// Copyright (C) 2005 - 2021 Settlers Freaks <sf-team at siedler25.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "AddonBool.h"
#include "mygettext/mygettext.h"

/**
 *  addon decativates reduction of fish population by fishing.
 */
class AddonInexhaustibleFish : public AddonBool
{
public:
    AddonInexhaustibleFish()
        : AddonBool(AddonId::INEXHAUSTIBLE_FISH, AddonGroup::Economy, _("Inexhaustible Fish"),
                    _("Deactivates reduction of fish population."))
    {}
};
