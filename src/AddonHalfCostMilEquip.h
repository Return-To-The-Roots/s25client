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
#ifndef ADDONHALFCOSTMILEQUIP_H_INCLUDED
#define ADDONHALFCOSTMILEQUIP_H_INCLUDED

#pragma once

#include "Addons.h"

///////////////////////////////////////////////////////////////////////////////
/**
 *  Addon allows a smith to create 1 shield + 1 sword for 1 iron + 1 coal instead of 2 iron + 2 coal
 *
 *  @author PoC
 */
class AddonHalfCostMilEquip : public AddonBool
{
    public:
        AddonHalfCostMilEquip() : AddonBool(ADDON_HALF_COST_MIL_EQUIP,
                                                  ADDONGROUP_ECONOMY,
                                                  gettext_noop("Half cost recruits"),
                                                  gettext_noop("Allows a smith to create 1 shield & 1 sword\n\n"
                                                          "for 1 iron + 1 coal instead of 2 iron + 2 coal"),
                                                  0
                                                 )
        {
        }
};

#endif // !ADDONHALFCOSTMILEQUIP_H_INCLUDED
