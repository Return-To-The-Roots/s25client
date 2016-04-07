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

#ifndef AddonNumScoutsExploration_h__
#define AddonNumScoutsExploration_h__

#pragma once

#include "Addons.h"
#include "mygettext/src/mygettext.h"

///////////////////////////////////////////////////////////////////////////////
/**
 *  Set the number of scouts required for an exploration expedition
 */
class AddonNumScoutsExploration: public AddonList
{
    public:
        AddonNumScoutsExploration() :
            AddonList(AddonId::NUM_SCOUTS_EXPLORATION,
                      ADDONGROUP_ECONOMY,
                      _("Number of scouts required for exploration expedition"),
                      _("Change the required number of scouts for an exploration via ship\n"
                        "Note: Setting this to low might make some maps imbalanced!"),
                      2)
        {
            addOption(_("Minimal"));
            addOption(_("Fewer"));
            addOption(_("Normal"));
            addOption(_("More"));
            addOption(_("Maximal"));
        }
};

#endif // AddonNumScoutsExploration_h__

