// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "gameTypes/GameSettingTypes.h"
#include "gameTypes/MapCoordinates.h"
#include "gameData/DescIdx.h"
#include <boost/filesystem/path.hpp>
#include <vector>

class Game;
class GameWorldBase;
class ILocalGameState;
class World;
struct TerrainDesc;

namespace libsiedler2 {
class ArchivItem_Map;
}

class MapLoader
{
    GameWorldBase& world_;
    std::vector<MapPoint> hqPositions_;

    DescIdx<TerrainDesc> getTerrainFromS2(uint8_t s2Id) const;
    /// Initialize the nodes according to the map data
    bool InitNodes(const libsiedler2::ArchivItem_Map& map, Exploration exploration);
    /// Place all objects on the nodes according to the map data.
    void PlaceObjects(const libsiedler2::ArchivItem_Map& map);
    void PlaceAnimals(const libsiedler2::ArchivItem_Map& map);

    /// Vermisst ein neues Weltmeer von einem Punkt aus, indem es alle mit diesem Punkt verbundenen
    /// Wasserpunkte mit der gleichen seaId belegt und die Anzahl zurückgibt
    static unsigned MeasureSea(World& world, MapPoint start, unsigned short seaId);
    static void CalcHarborPosNeighbors(World& world);

public:
    /// Construct a loader for the given world.
    explicit MapLoader(GameWorldBase& world);
    /// Load the map from the given archive, resetting previous state. Return false on error
    bool Load(const libsiedler2::ArchivItem_Map& map, Exploration exploration);
    /// Load the map from the given filepath
    bool Load(const boost::filesystem::path& mapFilePath);
    bool LoadLuaScript(Game& game, ILocalGameState& localgameState, const boost::filesystem::path& luaFilePath);
    /// Place the HQs on a loaded map (must be loaded first as hqPositions etc. are used)
    bool PlaceHQs(bool randomStartPos);

    /// Return the position of the players HQ (only valid after successful load)
    MapPoint GetHQPos(unsigned player) const { return hqPositions_[player]; }

    static void InitShadows(World& world);
    static void SetMapExplored(World& world);
    static bool InitSeasAndHarbors(World& world,
                                   const std::vector<MapPoint>& additionalHarbors = std::vector<MapPoint>());
    static bool PlaceHQs(GameWorldBase& world, std::vector<MapPoint> hqPositions, bool randomStartPos);
};
