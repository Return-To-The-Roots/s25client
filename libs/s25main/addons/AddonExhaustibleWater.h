// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "AddonList.h"
#include "mygettext/mygettext.h"

/**
 *  Addon for allowing to have unlimited resources.
 */
class AddonExhaustibleWater : public AddonList
{
public:
    AddonExhaustibleWater()
        : AddonList(AddonId::EXHAUSTIBLE_WATER, AddonGroup::Economy, _("Exhaustible Water"),
                    _("If Water is exhaustible wells will now dry out. If water everywhere is enabled, a geologist "
                      "will not notify for water"),
                    {
                      _("Inexhaustible"),
                      _("Inexhaustible and water everywhere"),
                      _("Exhaustible"),
                    })
    {}
};
