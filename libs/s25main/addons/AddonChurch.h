// Copyright (C) 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "AddonBool.h"
#include "mygettext/mygettext.h"

/**
 *  Addon enabling the Church building, blessing production, and soldier blessing services
 */
class AddonChurch : public AddonBool
{
public:
    AddonChurch()
        : AddonBool(AddonId::CHURCH, AddonGroup::Economy, _("Enable church"),
                    _("Allows building the church (dome) near warehouses when wine economy is also active, "
                      "producing blessings and blessing soldiers during periodic services (+50% hitpoints)."))
    {}
};
