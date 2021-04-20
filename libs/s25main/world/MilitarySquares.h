// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

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
