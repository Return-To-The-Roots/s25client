// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "AddonBool.h"

/**
 *  Addon allows a smith to create 1 shield + 1 sword for 1 iron + 1 coal instead of 2 iron + 2 coal
 */
class AddonHalfCostMilEquip : public AddonBool
{
public:
    AddonHalfCostMilEquip()
        : AddonBool(AddonId::HALF_COST_MIL_EQUIP, AddonGroup::Economy, _("Half cost recruits"),
                    _("Smith can create 1 shield & 1 sword "
                      "for 1 iron + 1 coal instead of 2 iron + 2 coal"))
    {}
};
