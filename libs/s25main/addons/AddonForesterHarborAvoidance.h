// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "AddonBool.h"
#include "mygettext/mygettext.h"

class AddonForesterHarborAvoidance : public AddonBool
{
public:
    AddonForesterHarborAvoidance()
        : AddonBool(AddonId::FORESTER_HARBOR_AVOIDANCE, AddonGroup::GamePlay | AddonGroup::Economy,
                    _("Foresters avoid harbor spots"),
                    _("Prevents foresters from planting trees on spots that a harbor could be built on."))
    {}
};
