// Copyright (C) 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "AddonBool.h"
#include "mygettext/mygettext.h"

/**
 *  Addon for a wine industry
 */
class AddonWine : public AddonBool
{
public:
    AddonWine()
        : AddonBool(AddonId::WINE, AddonGroup::Economy, _("Enable wine economy"),
                    _("Allows to build the wine economy buildings."))
    {}
};
