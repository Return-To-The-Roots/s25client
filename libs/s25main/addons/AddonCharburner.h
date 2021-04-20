// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "AddonBool.h"
#include "mygettext/mygettext.h"

/**
 *  Addon for a Charburner
 */
class AddonCharburner : public AddonBool
{
public:
    AddonCharburner()
        : AddonBool(AddonId::CHARBURNER, AddonGroup::Economy, _("Enable charburner"),
                    _("Allows to build the charburner."))
    {}
};
