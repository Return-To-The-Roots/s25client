// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "AddonList.h"
#include "mygettext/mygettext.h"

/**
 *  Addon which controls if coins should be enabled/disabled on capture
 */
class AddonCoinsCapturedBld : public AddonList
{
public:
    AddonCoinsCapturedBld()
        : AddonList(AddonId::COINS_CAPTURED_BLD, AddonGroup::Military, _("Coins on captured buildings"),
                    _("Change the coin setting for captured military buildings."),
                    {
                      _("Keep setting"),
                      _("Enable"),
                      _("Disable"),
                    })
    {}
};
