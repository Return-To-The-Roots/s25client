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

#ifndef MapSettings_h__
#define MapSettings_h__

#include "mapGenerator/MapStyle.h"
#include "gameTypes/LandscapeType.h"
#include "gameTypes/MapCoordinates.h"

/**
 * Settings used for map generation.
 */
struct MapSettings
{
    MapSettings()
        : players(2), size(MapExtent::all(256)), ratioGold(9), ratioIron(36), ratioCoal(40), ratioGranite(15), minPlayerRadius(0.31),
          maxPlayerRadius(0.51), type(LT_GREENLAND), style(MapStyle::Random)
    {
    }

    /**
     * Number of players.
     */
    unsigned players;

    /**
     * Map size in vertices.
     */
    MapExtent size;

    /**
     * Ratio of gold distributed as resources on mountain terrain.
     */
    unsigned short ratioGold;

    /**
     * Ratio of iron distributed as resources on mountain terrain.
     */
    unsigned short ratioIron;

    /**
     * Ratio of coal distributed as resources on mountain terrain.
     */
    unsigned short ratioCoal;

    /**
     * Ratio of granite distributed as resources on mountain terrain.
     */
    unsigned short ratioGranite;

    /**
     * Minimum radius from the center of the map for player placement.
     */
    double minPlayerRadius;

    /**
     * Maximum radius from the center of the map for player placement.
     */
    double maxPlayerRadius;

    /**
     * Landscape type used for map generation.
     */
    LandscapeType type;

    /**
     * Style of the map.
     */
    MapStyle style;
};

#endif // MapSettings_h__
