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
#ifndef ADDONSHIPSPEED_H_INCLUDED
#define ADDONSHIPSPEED_H_INCLUDED

#pragma once

#include "Addons.h"
#include "mygettext/src/mygettext.h"

///////////////////////////////////////////////////////////////////////////////
/**
 *  set ship movement speed
 *
 *  @author Marcus
 */
class AddonShipSpeed : public AddonList
{
    public:
        AddonShipSpeed() : AddonList(AddonId::SHIP_SPEED,
                                         ADDONGROUP_ECONOMY,
                                         _("Set ship speed"),
                                         _("Changes the ship movement speed"),
                                         2
                                        )
        {
            addOption(_("Very slow"));
            addOption(_("Slow"));
            addOption(_("Normal"));
            addOption(_("Fast"));
            addOption(_("Very fast"));
        }
};

#endif

