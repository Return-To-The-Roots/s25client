// Copyright (C) 2005 - 2025 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "AddonList.h"
#include "mygettext/mygettext.h"

/**
 *  Addon which controls if armor should be enabled/disabled on capture
 */
class AddonArmorCapturedBld : public AddonList
{
public:
    AddonArmorCapturedBld()
        : AddonList(AddonId::ARMOR_CAPTURED_BLD, AddonGroup::Military, _("Armor on captured buildings"),
                    _("Change the armor setting for captured military buildings."),
                    {
                      _("Keep setting"),
                      _("Enable"),
                      _("Disable"),
                    })
    {}
};
