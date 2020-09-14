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
 */
class AddonAdjustMilitaryStrength : public AddonList
{
public:
    AddonAdjustMilitaryStrength()
        : AddonList(AddonId::ADJUST_MILITARY_STRENGTH, AddonGroup::Military, _("Adjust military strength"),
                    _("Modify the strength increase of military ranks"),
                    {
                      _("Maximum strength"),
                      _("Medium strength"),
                      _("Minimum strength"),
                    },
                    1)
    {}
};
