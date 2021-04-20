// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "AddonList.h"
#include "mygettext/mygettext.h"

/**
 *  Addon for changing the maximum length of waterways.
 */
class AddonStatisticsVisibility : public AddonList
{
public:
    AddonStatisticsVisibility()
        : AddonList(AddonId::STATISTICS_VISIBILITY, AddonGroup::Other,
                    _("Change the visibility of your ingame statistics"),
                    _("Decides to whom your statistics are visible.\n\n"
                      "\"Allies\" applies to team members as well as to allies by treaty."),
                    {
                      _("Everyone"),
                      _("Allies"),
                      _("Nobody else but you"),
                    })
    {}
};
