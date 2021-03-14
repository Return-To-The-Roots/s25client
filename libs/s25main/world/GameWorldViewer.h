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
    void InitTerrainRenderer();

    /// Return the world itself
    const GameWorldBase& GetWorld() const { return gwb; }
    /// Return non-const world (TODO: Remove, this is a view only!)
    GameWorldBase& GetWorldNonConst() { return gwb; }
    const TerrainRenderer& GetTerrainRenderer() const { return tr; }
    SoundManager& GetSoundMgr();
    /// Get the player instance for this view
    const GamePlayer& GetPlayer() const;
    /// Get the ID of the views player
    unsigned GetPlayerId() const { return playerId_; }
    unsigned GetNumPlayers() const;

    /// Get number of soldiers that can attack bld at that point
    unsigned GetNumSoldiersForAttack(MapPoint pt) const;
    /// Get number of soldiers for attacking a point via sea
    unsigned GetNumSoldiersForSeaAttack(MapPoint pt) const;

    /// Get BQ for this player
    BuildingQuality GetBQ(const MapPoint& pt) const;
    /// Recalculates the visual BQ when a road part is build at that point
    void RecalcBQForRoad(const MapPoint& pt);
    /// Ermittelt Sichtbarkeit eines Punktes für den lokalen Spieler, berücksichtigt ggf. Teamkameraden
    Visibility GetVisibility(MapPoint pt) const;
    /// Returns true, if we own this point (but may not be our territory if this is a border point)
    bool IsOwner(const MapPoint& pt) const;
    /// Return true if the point belongs to any player
    bool IsPlayerTerritory(const MapPoint& pt) const;
    const MapNode& GetNode(const MapPoint& pt) const;
    MapPoint GetNeighbour(MapPoint pt, Direction dir) const;

    /// liefert sichtbare Strasse, im Nebel entsprechend die FoW-Strasse
    PointRoad GetVisibleRoad(MapPoint pt, RoadDir roadDir, Visibility visibility) const;
    PointRoad GetVisibleRoad(MapPoint pt, RoadDir roadDir) const;
    /// Get road, including virtual ones
    PointRoad GetVisiblePointRoad(MapPoint pt, Direction dir) const;
    void SetVisiblePointRoad(MapPoint pt, Direction dir, PointRoad type);
    bool IsOnRoad(const MapPoint& pt) const;
    /// Remove a visual (not yet built) road
    void RemoveVisualRoad(const MapPoint& start, const std::vector<Direction>& route);
    /// Checks if the road can be build in the world and additonally if there is no virtual road at that point
    bool IsRoadAvailable(bool isWaterRoad, const MapPoint& pt) const;

    /// Get the "youngest" FOWObject of all players who share the view with the local player
    const FOWObject* GetYoungestFOWObject(MapPoint pos) const;

    /// Gets the youngest fow node of all visible objects of all players who are connected
    /// with the local player via team view
    const FoWNode& GetYoungestFOWNode(MapPoint pos) const;

    /// Get first found ship of this player at that point or nullptr of none
    const noShip* GetShip(MapPoint pt) const;

    /// Schattierungen (vor allem FoW) neu berechnen
    void RecalcAllColors();

    /// Makes this a viewer for another player
    void ChangePlayer(unsigned player, bool updateVisualData = true);

    helpers::EnumArray<MapPoint, Direction> GetNeighbours(MapPoint pt) const;

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

    void InitVisualData();
    inline void VisibilityChanged(const MapPoint& pt, unsigned player);
    inline void RoadConstructionEnded(const RoadNote& note);
    void RecalcBQ(const MapPoint& pt);
};
