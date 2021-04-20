// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "AddonBool.h"
#include "mygettext/mygettext.h"

/**
 *  Addon for indicating military building construction possibility.
 *
 *  Gfx by SpikeOne.
 */
class AddonMilitaryAid : public AddonBool
{
public:
    AddonMilitaryAid()
        : AddonBool(AddonId::MILITARY_AID, AddonGroup::GamePlay | AddonGroup::Military, _("Military Aid"),
                    _("Adds military building indicators in construction aid mode."))
    {}
};
