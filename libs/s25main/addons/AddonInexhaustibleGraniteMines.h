// Copyright (C) 2005 - 2021 Settlers Freaks <sf-team at siedler25.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "AddonBool.h"
#include "mygettext/mygettext.h"

/**
 *  Addon for allowing to have unlimited resources.
 */
class AddonInexhaustibleGraniteMines : public AddonBool
{
public:
    AddonInexhaustibleGraniteMines()
        : AddonBool(AddonId::INEXHAUSTIBLE_GRANITEMINES, AddonGroup::Economy, _("Inexhaustible Granite Mines"),
                    _("Granite mines will never be depleted."))
    {}
};
