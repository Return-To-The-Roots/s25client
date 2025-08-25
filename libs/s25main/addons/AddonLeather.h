// Copyright (C) 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "AddonBool.h"
#include "mygettext/mygettext.h"

/**
 *  Addon for a leather industry
 */
class AddonLeather : public AddonBool
{
public:
    AddonLeather()
        : AddonBool(AddonId::LEATHER, AddonGroup::Economy, _("Enable leather economy"),
                    _("Allows to build the leather economy buildings."))
    {}
};
