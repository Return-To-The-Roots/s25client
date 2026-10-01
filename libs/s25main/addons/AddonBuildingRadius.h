// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "AddonBool.h"
#include "mygettext/mygettext.h"

/**
 *  Show building radius information in tooltips and as map overlay
 */
class AddonBuildingRadius : public AddonBool
{
public:
    AddonBuildingRadius()
        : AddonBool(AddonId::BUILDING_RADIUS, AddonGroup::GamePlay, _("Show building radius"),
                    _("Shows the working radius of buildings in the build menu tooltip and as an overlay on the map "
                      "when hovering over a building icon or selecting a building."),
                    1) // Enabled by default
    {}
};
