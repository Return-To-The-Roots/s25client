// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "AddonBool.h"
#include "mygettext/mygettext.h"

class AddonFreeHarborSpots : public AddonBool
{
public:
    AddonFreeHarborSpots()
        : AddonBool(AddonId::FREE_HARBOR_SPOTS, AddonGroup::GamePlay, _("Dangerous: Add limited extra harbor spots"),
                    _("Advanced option. Adds only a small deterministic set of suitable coastal castle sites as extra "
                      "harbor spots. May alter intended map seafaring design."))
    {}
};
