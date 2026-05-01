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
        : AddonBool(AddonId::FREE_HARBOR_SPOTS, AddonGroup::GamePlay, _("Build harbors without map markers"),
                    _("Allows harbors on suitable coastal castle sites even if the map does not define harbor spots."))
    {}
};
