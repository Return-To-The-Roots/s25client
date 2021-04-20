// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "AddonList.h"
#include "mygettext/mygettext.h"

/**
 *  Addon for changing the behavior of the military setting
 *  defender
 */
class AddonDefenderBehavior : public AddonList
{
public:
    AddonDefenderBehavior()
        : AddonList(AddonId::DEFENDER_BEHAVIOR, AddonGroup::Military, _("Change defender behavior"),
                    _("Change the military setting 'defender'.\n\n"
                      "You can choose to disallow any changes to that setting "
                      "or you can limit the amount of reoccupying troops "
                      "(during an attack) according to the defender setting."),
                    {
                      _("No change"),
                      _("Disallow change"),
                      _("Reduce reoccupying troops accordingly"),
                    })
    {}
};
