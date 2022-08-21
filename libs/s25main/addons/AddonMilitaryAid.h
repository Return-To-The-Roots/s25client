// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "AddonList.h"
#include "mygettext/mygettext.h"

/**
 *  Addon for indicating military building construction possibility.
 *
 *  Gfx by SpikeOne.
 */
class AddonMilitaryAid : public AddonList
{
public:
    AddonMilitaryAid()
        : AddonList(AddonId::MILITARY_AID, AddonGroup::GamePlay | AddonGroup::Military, _("Military Aid"),
                    _("Adds indicators for constructing or attacking military buildings."),
                    {
                        _("Off"),
                        _("Construction aid only"),
                        _("Construction and attack aid")
                    },
                    0)
    {}
};
