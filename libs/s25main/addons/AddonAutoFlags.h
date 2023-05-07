// Copyright (C) 2005 - 2023 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "AddonBool.h"
#include "mygettext/mygettext.h"

/**
 *  Automatically places flags on newly build roads
 *
 */
class AddonAutoFlags : public AddonBool
{
public:
    AddonAutoFlags()
        : AddonBool(AddonId::AUTOFLAGS, AddonGroup::Economy, _("Automatic flag placement"),
                    _("Automatically places flags on newly build roads"))
    {}
};