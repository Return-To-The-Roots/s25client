// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "AddonBool.h"

/**
 *  Addon disables all attacks
 */
class AddonPeacefulMode : public AddonBool
{
public:
    AddonPeacefulMode()
        : AddonBool(AddonId::PEACEFULMODE, AddonGroup::GamePlay | AddonGroup::Military, _("Peaceful"),
                    _("Nobody can attack anyone."))
    {}
};
