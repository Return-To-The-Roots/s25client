// Copyright (c) 2005 - 2018 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "AddonBool.h"

class AddonFrontierDistanceReachable : public AddonBool
{
public:
    AddonFrontierDistanceReachable()
        : AddonBool(AddonId::FRONTIER_DISTANCE_REACHABLE, AddonGroup::GamePlay | AddonGroup::Military,
                    _("Frontier Distance checks reachability"),
                    _("Military building counts as interior if an attack is permanently impossible. (Path blocked by "
                      "terrain like sea, "
                      "lava, etc.)"))
    {}
};
