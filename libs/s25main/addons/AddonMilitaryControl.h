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

#include "AddonBool.h"

/**
 *  Addon adds a 'order home' command button to military buildings. Pressing this button will send all soldiers of the
 *highest rank available to the next warehouse the command was first added so the ai wouldnt have to change the military
 *settings all the time to use the coins more efficiently - but with this addon players can use the command as well
 */
class AddonMilitaryControl : public AddonBool
{
public:
    AddonMilitaryControl()
        : AddonBool(AddonId::MILITARY_CONTROL, AddonGroup::GamePlay | AddonGroup::Military, _("Military Control"),
                    _("Adds the 'send home' button to military buildings.\n"
                      "Pressing this button will send all soldiers of the highest available rank to a warehouse (at "
                      "least 1 Soldier will "
                      "remain in the building)"))
    {}
};
