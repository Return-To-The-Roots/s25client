// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "AddonBool.h"
#include "mygettext/mygettext.h"

/**
 *  Addon for new race specific catapult graphics.
 *
 *  Those graphics were made by Parasit.
 */
class AddonCatapultGraphics : public AddonBool
{
public:
    AddonCatapultGraphics()
        : AddonBool(AddonId::CATAPULT_GRAPHICS, AddonGroup::GamePlay, _("Race specific catapult graphics"),
                    _("Adds new race-specific graphics for catapults to the game."))
    {}
};
