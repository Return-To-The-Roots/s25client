// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "AddonBool.h"
#include "mygettext/mygettext.h"

/**
 *  Addon allowing boat carriers to transport figures (soldiers, workers, ...) over waterways.
 *  Like wares they wait at the flag until the boat carrier picks them up.
 */
class AddonBoatsTransportFigures : public AddonBool
{
public:
    AddonBoatsTransportFigures()
        : AddonBool(AddonId::BOATS_TRANSPORT_FIGURES, AddonGroup::GamePlay, _("Boats transport settlers"),
                    _("Boat carriers also carry settlers like soldiers or workers over waterways.\n"
                      "They wait at the flag until a boat picks them up, just like wares."))
    {}
};
