// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "AddonList.h"
#include "mygettext/mygettext.h"

/**
 *  settings for sea attacks
 *
 *  enemy harbors do not block attacks
 *
 *  enemy harbors block attacks
 *
 *  no sea attacks
 */
class AddonSeaAttack : public AddonList
{
public:
    AddonSeaAttack()
        : AddonList(AddonId::SEA_ATTACK, AddonGroup::Military, _("Sea attack settings"),
                    _("Set restriction level for sea attacks"),
                    {
                      _("Enemy harbors don't block"),
                      _("Enemy harbors block"),
                      _("No sea attacks"),
                    },
                    2)
    {}
};
