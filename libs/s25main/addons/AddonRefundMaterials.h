// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "AddonList.h"
#include "mygettext/mygettext.h"

/**
 *  Addon for refunding materials soon as a building gets destroyed.
 */
class AddonRefundMaterials : public AddonList
{
public:
    AddonRefundMaterials()
        : AddonList(AddonId::REFUND_MATERIALS, AddonGroup::Economy, _("Refund materials for destroyed buildings"),
                    _("Get building materials back when a building is destroyed."),
                    {
                      _("No refund"),
                      _("Refund 25%"),
                      _("Refund 50%"),
                      _("Refund 75%"),
                      _("Get all back"),
                    })
    {}
};
