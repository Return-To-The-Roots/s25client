// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "AddonList.h"
#include "mygettext/mygettext.h"

/**
 *  Addon for limiting the count of catapults
 */
class AddonLimitCatapults : public AddonList
{
public:
    AddonLimitCatapults()
        : AddonList(AddonId::LIMIT_CATAPULTS, AddonGroup::Military, _("Limit number of catapults"),
                    _("Limits the number of catapults per player.\n\n"
                      "Proportional uses the following ratios of military buildings to catapults:\n"
                      "Barracks: 8\n"
                      "Guardhouse: 4\n"
                      "Watchtower: 2\n"
                      "Fortress: 1"),
                    {
                      _("Unlimited"),
                      _("Proportional"),
                      _("No catapults"),
                      _("3 catapults"),
                      _("5 catapults"),
                      _("10 catapults"),
                      _("20 catapults"),
                      _("30 catapults"),
                    })
    {}
};
