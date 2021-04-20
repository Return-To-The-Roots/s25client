// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "AddonBool.h"
#include "mygettext/mygettext.h"

/**
 *  Addon for refunding materials soon as a building gets destroyed and
 *  the emergency program is active
 */
class AddonRefundOnEmergency : public AddonBool
{
public:
    AddonRefundOnEmergency()
        : AddonBool(AddonId::REFUND_ON_EMERGENCY, AddonGroup::Economy, _("Refund materials in emergency program"),
                    _("Get building materials back when a building is destroyed "
                      "and your emergency program is active."))
    {}
};
