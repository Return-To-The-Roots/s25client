// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "AddonList.h"
#include "mygettext/mygettext.h"

/**
 *  Addon for changing gold deposits to other resources or
 *  to remove them completely
 */
class AddonChangeGoldDeposits : public AddonList
{
public:
    AddonChangeGoldDeposits()
        : AddonList(AddonId::CHANGE_GOLD_DEPOSITS, AddonGroup::Military | AddonGroup::Economy,
                    _("Change gold deposits"),
                    _("You can remove gold resources completely or convert them into iron ore, coal or granite.\n\n"
                      "You'll probably want to convert gold to iron ore, as this (on most maps)\n"
                      "allows you to utilize the additional coal not needed for minting anymore."),
                    {
                      _("No change"),
                      _("Remove gold completely"),
                      _("Convert to iron ore"),
                      _("Convert to coal"),
                      _("Convert to granite"),
                    })
    {}
};
