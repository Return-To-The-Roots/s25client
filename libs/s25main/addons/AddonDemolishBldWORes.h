// Copyright (c) 2005 - 2018 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "AddonBool.h"

class AddonDemolishBldWORes : public AddonBool
{
public:
    AddonDemolishBldWORes()
        : AddonBool(AddonId::DEMOLISH_BLD_WO_RES, AddonGroup::GamePlay | AddonGroup::Economy,
                    _("Demolish building when out of resources"),
                    _("Automatically demolish a resource gathering building, like a mine, if it runs permanently out "
                      "of resources."))
    {}
};
