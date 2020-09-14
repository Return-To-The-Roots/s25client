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

#include "gameTypes/GameSettingTypes.h"
#include "gameTypes/MapCoordinates.h"
#include "gameData/DescIdx.h"
#include <vector>

class World;
class GameWorldBase;
class glArchivItem_Map;
struct TerrainDesc;

class MapLoader
{
    World& world_;
    std::vector<MapPoint> hqPositions_;

    DescIdx<TerrainDesc> getTerrainFromS2(uint8_t s2Id) const;
    /// Initialize the nodes according to the map data
    bool InitNodes(const glArchivItem_Map& map, Exploration exploration);
    /// Place all objects on the nodes according to the map data.
    void PlaceObjects(const glArchivItem_Map& map);
    void PlaceAnimals(const glArchivItem_Map& map);

    /// Vermisst ein neues Weltmeer von einem Punkt aus, indem es alle mit diesem Punkt verbundenen
    /// Wasserpunkte mit der gleichen seaId belegt und die Anzahl zurückgibt
    static unsigned MeasureSea(World& world, MapPoint start, unsigned short seaId);
    static void CalcHarborPosNeighbors(World& world);

public:
    /// Construct a loader for the given world.
    explicit MapLoader(World& world);
    /// Load the map from the given archive, resetting previous state. Return false on error
    bool Load(const glArchivItem_Map& map, Exploration exploration);
    /// Place the HQs on a loaded map (must be loaded first as hqPositions etc. are used)
    bool PlaceHQs(GameWorldBase& world, bool randomStartPos);

    /// Return the position of the players HQ (only valid after successful load)
    MapPoint GetHQPos(unsigned player) const { return hqPositions_[player]; }

    static void InitShadows(World& world);
    static void SetMapExplored(World& world);
    static bool InitSeasAndHarbors(World& world, const std::vector<MapPoint>& additionalHarbors = std::vector<MapPoint>());
    static bool PlaceHQs(GameWorldBase& world, std::vector<MapPoint> hqPositions, bool randomStartPos);
};
