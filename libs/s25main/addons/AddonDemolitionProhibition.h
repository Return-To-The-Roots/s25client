// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "AddonList.h"
#include "mygettext/mygettext.h"

/**
 *  Addon for changing the maximum length of waterways.
 */
class AddonDemolitionProhibition : public AddonList
{
public:
    AddonDemolitionProhibition()
        : AddonList(AddonId::DEMOLITION_PROHIBITION, AddonGroup::Military,
                    _("Disable Demolition of military buildings"),
                    _("Disable the demolition of military buildings under attack or near frontiers."),
                    {
                      _("Off"),
                      _("Active if attacked"),
                      _("Active near frontiers"),
                    })
    {}
};
