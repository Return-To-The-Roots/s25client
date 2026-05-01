// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "AddonBool.h"
#include "mygettext/mygettext.h"

class AddonSingleSoldierCoinTraining : public AddonBool
{
public:
    AddonSingleSoldierCoinTraining()
        : AddonBool(AddonId::SINGLE_SOLDIER_COIN_TRAINING, AddonGroup::Military,
                    _("Coins train only one soldier"),
                    _("Gold coins promote only one lowest-rank soldier instead of all eligible lower-rank soldiers."))
    {}
};
