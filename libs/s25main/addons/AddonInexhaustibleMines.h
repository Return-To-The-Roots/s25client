// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "AddonBool.h"
#include "mygettext/mygettext.h"

/**
 *  Deprecated global mine setting.
 *
 *  Not registered anymore. The ID is still decoded when loading old settings/savegames and migrated to the per-mine
 *  resource behavior settings.
 *
 *  TODO(Replay) TODO(Savegame): Remove this compatibility class when legacy global mine settings no longer need
 *  migration.
 */
class AddonInexhaustibleMines : public AddonBool
{
public:
    AddonInexhaustibleMines()
        : AddonBool(AddonId::INEXHAUSTIBLE_MINES, AddonGroup::Economy, _("Inexhaustible Mines"),
                    _("Mines will never be depleted."))
    {}
};
