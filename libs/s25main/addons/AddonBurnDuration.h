// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "AddonList.h"

/**
 *  changes the duration a fire will burn when a building is destoryed
 */
class AddonBurnDuration : public AddonList
{
public:
    AddonBurnDuration()
        : AddonList(AddonId::BURN_DURATION, AddonGroup::GamePlay, _("Set duration fires burn"),
                    _("Adjusts how long a building will burn for when it is destroyed"),
                    {
                      _("Default"),
                      _("-25%"),
                      _("-50%"),
                      _("-75%"),
                      _("-90%"),
                      _("+50%"),
                      _("+100%"),
                    })
    {}
};
