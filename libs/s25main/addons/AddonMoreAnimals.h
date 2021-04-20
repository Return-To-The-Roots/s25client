// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "AddonList.h"
#include "mygettext/mygettext.h"

/**
 *  Addon allows users to adjust the percentage of trees that have the recurring spawn animal event
 */
class AddonMoreAnimals : public AddonList
{
public:
    AddonMoreAnimals()
        : AddonList(AddonId::MORE_ANIMALS, AddonGroup::Economy, _("More trees spawn animals"),
                    _("Adjust the fraction of trees that spawn animals."),
                    {
                      _("default"),
                      _("+50%"),
                      _("+100%"),
                      _("+200%"),
                      _("+500%"),
                      _("+1000%"),
                    })
    {}
};
