// Copyright (C) 2005 - 2025 Settlers Freaks <sf-team at siedler25.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "AddonBool.h"
#include "mygettext/mygettext.h"

/**
 *  Addon for disable armor by default
 */
class AddonNoArmorDefault : public AddonBool
{
public:
    AddonNoArmorDefault()
        : AddonBool(AddonId::NO_ARMOR_DEFAULT, AddonGroup::Military, _("Disable armor by default"),
                    _("Receiving armor is disabled for military buildings by default."))
    {}
};
