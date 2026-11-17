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
    bool initNodes(const libsiedler2::ArchivItem_Map& map, Exploration exploration);
    /// Place all objects on the nodes according to the map data.
    void placeObjects(const libsiedler2::ArchivItem_Map& map);
    void placeAnimals(const libsiedler2::ArchivItem_Map& map);

    /// Vermisst ein neues Weltmeer von einem Punkt aus, indem es alle mit diesem Punkt verbundenen
    /// Wasserpunkte mit der gleichen seaId belegt und die Anzahl zurückgibt
    static unsigned measureSea(World& world, MapPoint start, SeaId seaId);
    static void calcHarborPosNeighbors(World& world);

public:
    /// Construct a loader for the given world.
    explicit MapLoader(GameWorldBase& world);
    /// Load the map from the given archive, resetting previous state. Return false on error
    bool Load(const libsiedler2::ArchivItem_Map& map, Exploration exploration);
    /// Load the map from the given filepath
    bool Load(const boost::filesystem::path& mapFilePath);
    bool loadLuaScript(Game& game, ILocalGameState& localgameState, const boost::filesystem::path& luaFilePath);
    /// Place the HQs on a loaded map (must be loaded first as hqPositions etc. are used)
    /// Optionally add the starting wares to the HQs.
    /// Return false if there was an error (e.g. invalid start position)
    bool placeHQs(bool addStartWares = true);
    /// Setup resources like gold and water after loading a new map.
    /// TODO(Replay): Remove fixFish (always set to true)
    static void setupResources(GameWorldBase& world, bool fixFish = true);

    /// Return the (original/unshuffled) position of the players HQ (only valid after successful load)
    MapPoint getOriginalHQPos(unsigned player) const { return hqPositions_[player]; }

    static void initShadows(World& world);
    static void setMapExplored(World& world);
    static bool initSeasAndHarbors(World& world,
                                   const std::vector<MapPoint>& additionalHarbors = std::vector<MapPoint>());
    /// Place the HQs on a loaded map and add starting wares if desired.
    /// Return false if there was an error.
    static bool placeHQs(GameWorldBase& world, const std::vector<MapPoint>& hqPositions, bool addStartWares = true);

private:
    /// Converts map resources between types or deletes them. Used for games without gold.
    static void convertMineResourceTypes(GameWorldBase& world, ResourceType from, ResourceType to);
    /// Fills water depending on terrain and Addon setting.
    static void placeAndFixWater(GameWorldBase& world);
    /// Removes fish resources that cannot be reached by fisheries.
    static void removeUnusableFishResources(GameWorldBase& world);
};
