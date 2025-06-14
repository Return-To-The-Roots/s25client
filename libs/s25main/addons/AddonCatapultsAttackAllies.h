// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "AddonBool.h"
#include "mygettext/mygettext.h"

class AddonCatapultsAttackAllies : public AddonBool
{
public:
    AddonCatapultsAttackAllies()
        : AddonBool(AddonId::CATAPULTS_ATTACK_ALLIES, AddonGroup::Military, _("Catapults attack allies"),
                    _("Catapults will throw stones at allied buildings\n"
                      "(as they did in S2)"))
    {}
};
