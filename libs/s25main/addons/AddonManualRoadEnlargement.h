// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "AddonBool.h"
#include "mygettext/mygettext.h"

/**
 *  Addon for allowing to have unlimited resources.
 */
class AddonManualRoadEnlargement : public AddonBool
{
public:
    AddonManualRoadEnlargement()
        : AddonBool(AddonId::MANUAL_ROAD_ENLARGEMENT, AddonGroup::Economy, _("Manual road enlargement"),
                    _("Manually upgrade your roads and directly build donkey roads."))
    {}
};
