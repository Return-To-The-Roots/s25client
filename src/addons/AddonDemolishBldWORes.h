// Copyright (c) 2005 - 2018 Settlers Freaks (sf-team at siedler25.org)
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

#ifndef ADDONDEMOLISHBLDWORES_H_INCLUDED
#define ADDONDEMOLISHBLDWORES_H_INCLUDED

#pragma once

#include "AddonBool.h"

class AddonDemolishBldWORes : public AddonBool
{
public:
    AddonDemolishBldWORes()
        : AddonBool(AddonId::DEMOLISH_BLD_WO_RES, ADDONGROUP_GAMEPLAY | ADDONGROUP_ECONOMY,
            _("Demolish building when out of resources"),
            _("Automatically demolish the building if it runs permanently out of natural resources like mines."),
            0)
    {}
};

#endif
