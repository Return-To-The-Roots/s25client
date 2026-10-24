// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "AddonBool.h"
#include "mygettext/mygettext.h"

class AddonAdditionalHarborSpots : public AddonBool
{
public:
    AddonAdditionalHarborSpots()
        : AddonBool(AddonId::ADDITIONAL_HARBOR_SPOTS, AddonGroup::GamePlay, _("Dangerous: Add extra harbor spots"),
                    _("Advanced option. Converts a small set of suitable coastal castle sites to harbor spots. "
                      "Caution: May alter intended map seafaring design."))
    {}
};
