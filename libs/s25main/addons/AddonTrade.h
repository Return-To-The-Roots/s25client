// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "AddonBool.h"
#include "mygettext/mygettext.h"

class AddonTrade : public AddonBool
{
public:
    AddonTrade()
        : AddonBool(AddonId::TRADE, AddonGroup::Economy, _("Trade"),
                    _("Allows to send wares/figures to allied warehouses"))
    {}
};
