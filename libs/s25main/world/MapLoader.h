// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "gameTypes/GameSettingTypes.h"
#include "gameTypes/MapCoordinates.h"
#include "gameTypes/MapTypes.h"
#include "gameTypes/Resource.h"
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
    static unsigned MeasureSea(World& world, MapPoint start, SeaId seaId);
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
    /// Optionally add the starting wares to the HQs.
    /// Return false if there was an error (e.g. invalid start position)
    bool PlaceHQs(bool addStartWares = true);
    /// Setup resources like gold and water after loading a new map.
    /// TODO(Replay): Remove fixFish (always set to true)
    static void SetupResources(GameWorldBase& world, bool fixFish = true);

    /// Return the (original/unshuffled) position of the players HQ (only valid after successful load)
    MapPoint GetOriginalHQPos(unsigned player) const { return hqPositions_[player]; }

    static void InitShadows(World& world);
    static void SetMapExplored(World& world);
    static bool InitSeasAndHarbors(World& world,
                                   const std::vector<MapPoint>& additionalHarbors = std::vector<MapPoint>());
    /// Place the HQs on a loaded map and add starting wares if desired.
    /// Return false if there was an error.
    static bool PlaceHQs(GameWorldBase& world, const std::vector<MapPoint>& hqPositions, bool addStartWares = true);

private:
    /// Converts map resources between types or deletes them. Used for games without gold.
    static void ConvertMineResourceTypes(GameWorldBase& world, ResourceType from, ResourceType to);
    /// Fills water depending on terrain and Addon setting.
    static void PlaceAndFixWater(GameWorldBase& world);
    /// Removes fish resources that cannot be reached by fisheries.
    static void RemoveUnusableFishResources(GameWorldBase& world);
};
