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

#ifndef ADDONFRONTIERDISTANCEREACHABLE_H_INCLUDED
#define ADDONFRONTIERDISTANCEREACHABLE_H_INCLUDED

#pragma once

#include "AddonBool.h"

class AddonFrontierDistanceReachable : public AddonBool
{
public:
    AddonFrontierDistanceReachable()
        : AddonBool(AddonId::FRONTIER_DISTANCE_REACHABLE, ADDONGROUP_GAMEPLAY | ADDONGROUP_MILITARY,
                    _("Frontier Distance checks Reachability"),
                    _("Checks if the military building is reachable by triple distance of both military buildings (minimum distance is "
                      "10). If its not reachable within this distance a military building counts as far away."),
                    0)
    {}
};

#endif // !ADDONFRONTIERDISTANCEREACHABLE_H_INCLUDED
