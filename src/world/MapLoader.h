// Copyright (c) 2005 - 2015 Settlers Freaks (sf-team at siedler25.org)
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

#ifndef MapLoader_h__
#define MapLoader_h__

#include "gameTypes/MapTypes.h"
#include "gameTypes/GameSettingTypes.h"
#include "gameData/NationConsts.h"
#include <vector>

class World;
class glArchivItem_Map;

class MapLoader
{
    World& world;
    const std::vector<Nation> playerNations;
    std::vector<MapPoint> hqPositions;

    /// Vermisst ein neues Weltmeer von einem Punkt aus, indem es alle mit diesem Punkt verbundenen
    /// Wasserpunkte mit der gleichen sea_id belegt und die Anzahl zurückgibt
    unsigned MeasureSea(const MapPoint pt, const unsigned short sea_id);

    void InitSeasAndHarbors();

    /// Inititalize the nodes according to the map data
    void InitNodes(const glArchivItem_Map& map, Exploration exploration);
    /// Place all objects on the nodes according to the map data.
    void PlaceObjects(const glArchivItem_Map& map);
    bool PlaceHQs(bool randomStartPos);
    void PlaceAnimals(const glArchivItem_Map& map);
    void CalcHarborPosNeighbors();
public:
    /// Construct a loader for the given world.
    /// Size of @playerNations must be the player count and unused player spots must be set to NAT_INVALID
    MapLoader(World& world, const std::vector<Nation>& playerNations);
    /// Load the map from the given archive, resetting previous state. Return false on error
    bool Load(const glArchivItem_Map& map, bool randomStartPos, Exploration exploration);
    /// Return the position of the players HQ (only valid after successful load)
    MapPoint GetHQPos(unsigned player) const { return hqPositions[player]; }
};

#endif // MapLoader_h__
