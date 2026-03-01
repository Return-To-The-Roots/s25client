// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

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

/// Properties of a point/node on a map
struct MapNode
{
    /// Roads from this point: E, SE, SW
    helpers::EnumArray<PointRoad, RoadDir> roads;
    /// Height
    unsigned char altitude;
    /// Shading
    unsigned char shadow;
    /// Terrain (t1 is the triangle with the edge at the top exactly below this pt, t2 with the edge at the bottom on
    /// the right lower side of the pt)
    DescIdx<TerrainDesc> t1, t2;
    /// Collectible resources
    Resource resources;
    /// Reserved by worker
    bool reserved;
    /// Owner (playerIdx - 1)
    unsigned char owner;
    BoundaryStones boundary_stones;
    BuildingQuality bq;
    /// How the players see the point in FoW
    std::array<FoWNode, MAX_PLAYERS> fow;

    /// To which sea this belongs to
    SeaId seaId;
    /// Which harbor is here
    HarborId harborId;

    /// Object built here
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
