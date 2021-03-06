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

#include "Resource.h"
#include "helpers/EnumArray.h"
#include "gameTypes/BuildingQuality.h"
#include "gameTypes/FoWNode.h"
#include "gameTypes/MapTypes.h"
#include "gameData/DescIdx.h"
#include "gameData/MaxPlayers.h"
#include <array>
#include <list>
#include <memory>
#include <vector>

class noBase;
class SerializedGameData;
struct TerrainDesc;
struct WorldDescription;

/// Eigenschaften von einem Punkt auf der Map
struct MapNode
{
    /// Roads from this point: E, SE, SW
    helpers::EnumArray<PointRoad, RoadDir> roads;
    /// Height
    unsigned char altitude;
    /// Schattierung
    unsigned char shadow;
    /// Terrain (t1 is the triangle with the edge at the top exactly below this pt, t2 with the edge at the bottom on
    /// the right lower side of the pt)
    DescIdx<TerrainDesc> t1, t2;
    /// Ressourcen
    Resource resources;
    /// Reservierungen
    bool reserved;
    /// Owner (playerIdx - 1)
    unsigned char owner;
    BoundaryStones boundary_stones;
    BuildingQuality bq;
    /// How the players see the point in FoW
    std::array<FoWNode, MAX_PLAYERS> fow;

    /// To which sea this belongs to (0=None)
    unsigned short seaId;
    /// Hafenpunkt-ID (0 = kein Hafenpunkt)
    unsigned harborId;

    /// Objekt, welches sich dort befindet
    noBase* obj;
    /// Figures or fights on this node
    std::list<std::unique_ptr<noBase>> figures;

    MapNode();
    MapNode(const MapNode&) = delete;
    MapNode(MapNode&&) = default;
    MapNode& operator=(const MapNode&) = delete;
    MapNode& operator=(MapNode&&) = default;
    void Serialize(SerializedGameData& sgd, unsigned numPlayers, const WorldDescription& desc) const;
    void Deserialize(SerializedGameData& sgd, unsigned numPlayers, const WorldDescription& desc,
                     const std::vector<DescIdx<TerrainDesc>>& landscapeTerrains);
};
