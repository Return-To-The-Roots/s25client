// Copyright (c) 2005 - 2015 Settlers Freaks (sf-team at siedler25.org)
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
#ifndef ADDONSEAATTACK_H_INCLUDED
#define ADDONSEAATTACK_H_INCLUDED

#pragma once

#include "Addons.h"
#include "mygettext/src/mygettext.h"

///////////////////////////////////////////////////////////////////////////////
/**
 *  settings for sea attacks
 *
 *  enemy harbors do not block attacks
 *
 *  enemy harbors block attacks
 *
 *  no sea attacks
 *
 *  @author poc
 */
class AddonSeaAttack : public AddonList
{
    public:
        AddonSeaAttack() : AddonList(AddonId::SEA_ATTACK,
                                         ADDONGROUP_MILITARY,
                                         _("Sea attack settings"),
                                         _("Set restriction level for sea attacks"),
                                         2
                                        )
        {
            addOption(_("Enemy harbors don't block"));
            addOption(_("Enemy harbors block"));
            addOption(_("No sea attacks"));
        }
};

#endif

