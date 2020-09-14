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

#include "gameTypes/MapCoordinates.h"
#include <list>
#include <vector>

class nobBaseMilitary;
class sortedMilitaryBlds;

class MilitarySquares
{
    /// military buildings (including HQs and harbors) per military square
    std::vector<std::list<nobBaseMilitary*>> squares;
    MapExtent size_;
    // Liefert das entsprechende Militärquadrat für einen bestimmten Punkt auf der Karte zurück (normale Koordinaten)
    std::list<nobBaseMilitary*>& GetSquare(MapPoint pt);

public:
    MilitarySquares();
    void Init(const MapExtent& mapSize);
    void Clear();
    void Add(nobBaseMilitary* bld);
    void Remove(nobBaseMilitary* bld);
    sortedMilitaryBlds GetBuildingsInRange(MapPoint pt, unsigned short radius) const;
};
