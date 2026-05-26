// Copyright (C) 2005 - 2021 Settlers Freaks <sf-team at siedler25.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "AddonMineResourceBehavior.h"
#include "mygettext/mygettext.h"

/**
 *  Granite mine resource behavior list.
 *
 *  Reuses the old boolean INEXHAUSTIBLE_GRANITEMINES id: saved value 0 still means default behavior and saved value 1
 *  now selects the inexhaustible behavior.
 */
class AddonInexhaustibleGraniteMines : public AddonMineResourceBehaviorBase
{
public:
    AddonInexhaustibleGraniteMines()
        : AddonMineResourceBehaviorBase(AddonId::INEXHAUSTIBLE_GRANITEMINES, _("Granite Mine Resource Behavior"),
                                        _("Configures how granite mines consume and exhaust stone deposits."))
    {}
};
