// Copyright (c) 2005 - 2013 Settlers Freaks (sf-team at siedler25.org)
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
#ifndef ADDONMILITARYAID_H_INCLUDED
#define ADDONMILITARYAID_H_INCLUDED

#pragma once

#include "Addons.h"
#include "mygettext/src/mygettext.h"

///////////////////////////////////////////////////////////////////////////////
/**
 *  Addon for indicating military building construction possibility.
 *
 *  Gfx by SpikeOne.
 *
 *  @author KaiN
 */
class AddonMilitaryAid : public AddonBool
{
    public:
        AddonMilitaryAid() : AddonBool(AddonId::MILITARY_AID,
                                           ADDONGROUP_GAMEPLAY | ADDONGROUP_MILITARY,
                                           _("Military Aid"),
                                           _("Adds military building indicators in construction aid mode."),
                                           0
                                          )
        {
        }
};

#endif // !ADDONMILITARYAID_H_INCLUDED
