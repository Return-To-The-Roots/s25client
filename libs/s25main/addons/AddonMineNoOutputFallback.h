// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "AddonList.h"
#include "const_addons.h"
#include "mygettext/mygettext.h"
#include "gameTypes/MineNoOutputFallback.h"

class AddonMineNoOutputFallback : public AddonList
{
public:
    AddonMineNoOutputFallback()
        : AddonList(AddonId::MINE_NO_OUTPUT_FALLBACK, AddonGroup::Economy, _("Mine No-Output Fallback"),
                    _("Configures what mines produce when S4-like exhaustion would produce nothing."),
                    {_("Produce nothing"), _("Produce granite 25%"), _("Produce granite 50%"),
                     _("Produce granite 100%"), _("Produce lower grade resource")},
                    static_cast<unsigned>(MineNoOutputFallback::ProduceNothing))
    {}
};
