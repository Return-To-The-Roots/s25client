// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "AddonBool.h"
#include "mygettext/mygettext.h"

/**
 *  Addon for allowing to have unlimited resources.
 */
class AddonInexhaustibleMines : public AddonBool
{
public:
    AddonInexhaustibleMines()
        : AddonBool(AddonId::INEXHAUSTIBLE_MINES, AddonGroup::Economy, _("Inexhaustible Mines"),
                    _("Mines will never be depleted."))
    {}
};
