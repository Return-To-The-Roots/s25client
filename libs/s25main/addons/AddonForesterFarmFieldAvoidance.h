// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "AddonBool.h"
#include "mygettext/mygettext.h"

class AddonForesterFarmFieldAvoidance : public AddonBool
{
public:
    AddonForesterFarmFieldAvoidance()
        : AddonBool(AddonId::FORESTER_FARM_FIELD_AVOIDANCE, AddonGroup::GamePlay | AddonGroup::Economy,
                    _("Foresters avoid farm field spots"),
                    _("Prevents foresters from planting trees on spots that own farms could use for new fields."))
    {}
};
