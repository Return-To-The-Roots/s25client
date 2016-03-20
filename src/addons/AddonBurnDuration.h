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
#ifndef ADDONBURNDURATION_H_INCLUDED
#define ADDONBURNDURATION_H_INCLUDED

#pragma once

#include "Addons.h"

///////////////////////////////////////////////////////////////////////////////
/**
 *  changes the duration a fire will burn when a building is destoryed
 *
 *  @author PoC
 */
class AddonBurnDuration : public AddonList
{
    public:
        AddonBurnDuration() : AddonList(AddonId::BURN_DURATION,
                                                 ADDONGROUP_GAMEPLAY,
                                                 _("Set duration fires burn"),
                                                 _("Adjusts how long a building will burn for when it is destroyed"),
                                                 0
                                                )
        {
            addOption(_("Default"));
            addOption(_("-25%"));
            addOption(_("-50%"));
            addOption(_("-75%"));
            addOption(_("-90%"));
			addOption(_("+50%"));
			addOption(_("+100%"));
        }
};

#endif 
