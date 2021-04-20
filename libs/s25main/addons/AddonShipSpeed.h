// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "AddonList.h"
#include "mygettext/mygettext.h"

/**
 *  set ship movement speed
 */
class AddonShipSpeed : public AddonList
{
public:
    AddonShipSpeed()
        : AddonList(AddonId::SHIP_SPEED, AddonGroup::Economy, _("Set ship speed"), _("Changes the ship movement speed"),
                    {
                      _("Very slow"),
                      _("Slow"),
                      _("Normal"),
                      _("Fast"),
                      _("Very fast"),
                    },
                    2)
    {}
};
