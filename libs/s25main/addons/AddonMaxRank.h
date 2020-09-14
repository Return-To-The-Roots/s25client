// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// This file is part of Return To The Roots.
//
// Return To The Roots is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// Return To The Roots is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Return To The Roots. If not, see <http://www.gnu.org/licenses/>.

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
