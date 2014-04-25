// Copyright (c) 2005 - 2011 Settlers Freaks (sf-team at siedler25.org)
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
#ifndef ADDONADJUSTMILITARYSTRENGTH_H_INCLUDED
#define ADDONADJUSTMILITARYSTRENGTH_H_INCLUDED

#pragma once

#include "Addons.h"

///////////////////////////////////////////////////////////////////////////////
/**
 *  Addon for changing military strength of higher ranks.
 *
 *  Default behaviour: (Maximum strength)
 *  - Dice rolling
 *  - Rank+3
 *
 *  Medium strength:
 *  - Dice rolling
 *  - Rank + 8
 *
 *  Minimum strength
 *  - Dice rolling
 *  - All ranks get same dice
 *
 *  @author CS2001
 */
class AddonAdjustMilitaryStrength : public AddonList
{
    public:
        AddonAdjustMilitaryStrength() : AddonList(ADDON_ADJUST_MILITARY_STRENGTH,
                    ADDONGROUP_MILITARY,
                    gettext_noop("Adjust military strength"),
                    gettext_noop("Allows you to modify the strength increase of military ranks\n\n"),
                    1
                                                     )
        {
            addOption(gettext_noop("Maximum strength"));
            addOption(gettext_noop("Medium strength"));
            addOption(gettext_noop("Minimum strength"));
        }
};

#endif // !ADDONADJUSTMILITARYSTRENGTH_H_INCLUDED

