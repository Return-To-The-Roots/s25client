// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "AddonBool.h"

/**
 *  Addon for Battlefield Promotions
 */
class AddonBattlefieldPromotion : public AddonBool
{
public:
    AddonBattlefieldPromotion()
        : AddonBool(AddonId::BATTLEFIELD_PROMOTION, AddonGroup::Military, _("Enable battlefield promotions"),
                    _("Soldiers winning a fight increase in rank."))
    {}
};
