// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "AddonList.h"
#include "mygettext/mygettext.h"

/**
 *  Set the number of scouts required for an exploration expedition
 */
class AddonNumScoutsExploration : public AddonList
{
public:
    AddonNumScoutsExploration()
        : AddonList(AddonId::NUM_SCOUTS_EXPLORATION, AddonGroup::Economy,
                    _("Number of scouts required for exploration expedition"),
                    _("Change the required number of scouts for an exploration via ship\n"
                      "Note: Setting this to low might make some maps imbalanced!"),

                    {
                      _("Minimal"),
                      _("Fewer"),
                      _("Normal"),
                      _("More"),
                      _("Maximal"),
                    },
                    2)
    {}
};
