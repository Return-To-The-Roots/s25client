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
#include <vector>

class World;
class glArchivItem_Map;

class MapLoader
{
    World& world;
    /// Vermisst ein neues Weltmeer von einem Punkt aus, indem es alle mit diesem Punkt verbundenen
    /// Wasserpunkte mit der gleichen sea_id belegt und die Anzahl zurückgibt
    unsigned MeasureSea(const MapPoint pt, const unsigned short sea_id);

    void InitSeasAndHarbors();

    /// Inititalizes the nodes according to the map data
    void InitNodes(const glArchivItem_Map& map);
    /// Places all objects on the nodes according to the map data. Returns the positions of the HQs
    std::vector<MapPoint> PlaceObjects(const glArchivItem_Map& map);
    void PlaceHQs(std::vector<MapPoint> &headquarter_positions);
    void PlaceAnimals(const glArchivItem_Map& map);
    void CalcHarborPosNeighbors();
public:
    explicit MapLoader(World& world);
    void Load(const glArchivItem_Map& map);
};

#endif // MapLoader_h__
