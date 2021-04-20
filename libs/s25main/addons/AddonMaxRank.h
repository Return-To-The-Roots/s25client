// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "AddonList.h"
#include "mygettext/mygettext.h"

/**
 *  limit max rank of soldiers
 *
 *  rank 4 General Default
 *
 *  rank 3 Officer
 *
 *  rank 2 Sergeant
 *
 *  rank 1 Privatefirstclass
 *
 *  rank 0 Private
 */
class AddonMaxRank : public AddonList
{
public:
    AddonMaxRank()
        : AddonList(AddonId::MAX_RANK, AddonGroup::Military, _("Set max rank"), _("Limit the rank for soldiers"),
                    {
                      _("General (4)"),
                      _("Officer (3)"),
                      _("Sergeant (2)"),
                      _("Privatefc (1)"),
                      _("Private (0)"),
                    })
    {}
};
