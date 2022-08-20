// Copyright (C) 2005 - 2022 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "AddonBool.h"
#include "mygettext/mygettext.h"

/**
 *  Addon for new nation-specific catapult graphics.
 *
 *  Those graphics were made by Parasit.
 */
class AddonCatapultGraphics : public AddonBool
{
public:
    AddonCatapultGraphics()
        : AddonBool(AddonId::CATAPULT_GRAPHICS, AddonGroup::GamePlay, _("Nation-specific catapult graphics"),
                    _("Adds new race-specific graphics for catapults to the game."))
    {}
};
