// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "NodeMapBase.h"
#include "TerrainRenderer.h"
#include "helpers/EnumArray.h"
#include "notifications/Subscription.h"
#include "gameTypes/BuildingQuality.h"
#include "gameTypes/Direction.h"
#include "gameTypes/MapCoordinates.h"
#include "gameTypes/MapTypes.h"

class FOWObject;
class GamePlayer;
class GameWorldBase;
class noShip;
class SoundManager;
struct FoWNode;
struct MapNode;
struct RoadNote;

/// This is a players View(er) on the GameWorld
class GameWorldViewer
{
public:
    GameWorldViewer(unsigned playerId, GameWorldBase& gwb);

    /// Init the terrain renderer. Must be done before first call to GetTerrainRenderer!
    void initTerrainRenderer();

    /// Return the world itself
    const GameWorldBase& getWorld() const { return gwb; }
    /// Return non-const world (TODO: Remove, this is a view only!)
    GameWorldBase& getWorldNonConst() { return gwb; }
    const TerrainRenderer& getTerrainRenderer() const { return tr; }
    auto getMaxNodeAltitude() const { return maxNodeAltitude_; }
    SoundManager& getSoundMgr();
    /// Get the player instance for this view
    const GamePlayer& getPlayer() const;
    /// Get the ID of the views player
    unsigned getPlayerId() const { return playerId_; }
    unsigned getNumPlayers() const;

    /// Get number of soldiers that can attack bld at that point
    unsigned getNumSoldiersForAttack(MapPoint pt) const;
    /// Get number of soldiers for attacking a point via sea
    unsigned getNumSoldiersForSeaAttack(MapPoint pt) const;

    /// Get BQ for this player
    BuildingQuality GetBQ(const MapPoint& pt) const;
    /// Recalculates the visual BQ when a road part is build at that point
    void recalcBQForRoad(const MapPoint& pt);
    /// Ermittelt Sichtbarkeit eines Punktes für den lokalen Spieler, berücksichtigt ggf. Teamkameraden
    /// If checkAllVisible is false then `IsAllVisible` is assumed to return false, else it is called to check
    Visibility getVisibility(MapPoint pt, bool checkAllVisible = true) const;
    /// Check if the whole map should be visible
    bool isAllVisible() const;
    /// Returns true, if we own this point (but may not be our territory if this is a border point)
    bool isOwner(const MapPoint& pt) const;
    /// Return true if the point belongs to any player
    bool isPlayerTerritory(const MapPoint& pt) const;
    const MapNode& getNode(const MapPoint& pt) const;
    MapPoint getNeighbour(MapPoint pt, Direction dir) const;

    /// liefert sichtbare Strasse, im Nebel entsprechend die FoW-Strasse
    PointRoad getVisibleRoad(MapPoint pt, RoadDir roadDir, Visibility visibility) const;
    PointRoad getVisibleRoad(MapPoint pt, RoadDir roadDir) const;
    /// Get road, including virtual ones
    PointRoad getVisiblePointRoad(MapPoint pt, Direction dir) const;
    void setVisiblePointRoad(MapPoint pt, Direction dir, PointRoad type);
    bool isOnRoad(const MapPoint& pt) const;
    /// Remove a visual (not yet built) road
    void removeVisualRoad(const MapPoint& start, const std::vector<Direction>& route);
    /// Checks if the road can be build in the world and additonally if there is no virtual road at that point
    bool isRoadAvailable(bool isWaterRoad, const MapPoint& pt) const;

    /// Get the "youngest" FOWObject of all players who share the view with the local player
    const FOWObject* getYoungestFOWObject(MapPoint pos) const;

    /// Gets the youngest fow node of all visible objects of all players who are connected
    /// with the local player via team view
    const FoWNode& getYoungestFOWNode(MapPoint pos) const;

    /// Get first found ship of this player at that point or nullptr of none
    const noShip* getShip(MapPoint pt) const;

    /// Schattierungen (vor allem FoW) neu berechnen
    void recalcAllColors();

    /// Makes this a viewer for another player
    void changePlayer(unsigned player, bool updateVisualData = true);

    helpers::EnumArray<MapPoint, Direction> getNeighbours(MapPoint pt) const;

private:
    /// Visual node status (might be different than world if GameCommand is just sent) to hide network latency
    struct VisualMapNode
    {
        helpers::EnumArray<PointRoad, RoadDir>
          roads; // If != 0 then this road value is used (road construction) else real road is used
        BuildingQuality bq;
    };
    unsigned playerId_;
    GameWorldBase& gwb;
    TerrainRenderer tr;
    Subscription evVisibilityChanged, evAltitudeChanged, evRoadConstruction, evBQChanged;
    NodeMapBase<VisualMapNode> visualNodes;
    /// Max height of any node
    uint8_t maxNodeAltitude_ = 0;

    void initVisualData();
    inline void visibilityChanged(const MapPoint& pt, unsigned player);
    inline void roadConstructionEnded(const RoadNote& note);
    void recalcBQ(const MapPoint& pt);
};
