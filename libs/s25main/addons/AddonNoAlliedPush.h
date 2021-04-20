// Copyright (C) 2018 - 2021 Settlers Freaks <sf-team at siedler25.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "AddonBool.h"

/**
 *  Addon stops allied players from pushing your borders back with new military buildings
 */
class AddonNoAlliedPush : public AddonBool
{
public:
    AddonNoAlliedPush()
        : AddonBool(AddonId::NO_ALLIED_PUSH, AddonGroup::Military, _("Improved Alliance"),
                    _("Allied players can no longer push your borders back with new buildings."))
    {}
};
