// Copyright (C) 2018 - 2021 Settlers Freaks <sf-team at siedler25.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "AddonBool.h"
#include "mygettext/mygettext.h"

/**
 *  Addon for disable coins by default
 */
class AddonNoCoinsDefault : public AddonBool
{
public:
    AddonNoCoinsDefault()
        : AddonBool(AddonId::NO_COINS_DEFAULT, AddonGroup::Military, _("Disable coins by default"),
                    _("Receiving coins is disabled for military buildings by default."))
    {}
};
