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

#ifndef GameWorldViewer_h__
#define GameWorldViewer_h__

#include "TerrainRenderer.h"
#include "notifications/Subscribtion.h"
#include "gameTypes/BuildingQuality.h"
#include "gameTypes/Direction.h"
#include "gameTypes/MapTypes.h"

class GameClientPlayer;
class FOWObject;
class GameWorldBase;
struct MapNode;
struct NodeNote;
class noShip;
class TerrainRenderer;
struct RoadNote;

/// This is a players View(er) on the GameWorld
class GameWorldViewer
{
public:

    GameWorldViewer(unsigned playerId, GameWorldBase& gwb);

    /// Return the world itself
    const GameWorldBase& GetWorld() const { return gwb; }
    /// Return non-const world (TODO: Remove, this is a view only!)
    GameWorldBase& GetWorldNonConst() { return gwb; }
    const TerrainRenderer& GetTerrainRenderer() const { return tr; }
    /// Get the player instance for this view
    const GameClientPlayer& GetPlayer() const;
    /// Get the ID of the views player
    unsigned GetPlayerID() const { return playerId_; }

    /// Get number of soldiers that can attack that point
    unsigned GetAvailableSoldiersForAttack(const MapPoint pt) const;
    /// Get number of soldiers for attacking a point via sea
    unsigned GetAvailableSoldiersForSeaAttackCount(const MapPoint pt) const;

    /// Get BQ for this player
    BuildingQuality GetBQ(const MapPoint& pt) const;
    /// Recalculates the visual BQ when a road part is build at that point
    void RecalcBQForRoad(const MapPoint& pt);
    /// Ermittelt Sichtbarkeit eines Punktes für den lokalen Spieler, berücksichtigt ggf. Teamkameraden
    Visibility GetVisibility(const MapPoint pt) const;
    /// Returns true, if we own this point (but may not be our territory if this is a border point)
    bool IsOwner(const MapPoint& pt) const;
    const MapNode& GetNode(const MapPoint& pt) const;
    MapPoint GetNeighbour(const MapPoint pt, const Direction dir) const;

    /// liefert sichtbare Strasse, im Nebel entsprechend die FoW-Strasse
    unsigned char GetVisibleRoad(const MapPoint pt, unsigned char roadDir, const Visibility visibility) const;
    unsigned char GetVisibleRoad(const MapPoint pt, unsigned char roadDir) const;
    /// Get road, including virtual ones
    unsigned char GetVisiblePointRoad(const MapPoint pt, Direction dir) const;
    void SetVisiblePointRoad(const MapPoint& pt, Direction dir, unsigned char type);
    bool IsOnRoad(const MapPoint& pt) const;
    /// Remove a visual (not yet built) road
    void RemoveVisualRoad(const MapPoint& start, const std::vector<unsigned char>& route);
    /// Checks if the road can be build in the world and additonally if there is no virtual road at that point
    bool IsRoadAvailable(bool isWaterRoad, const MapPoint& pt) const;

    /// Get the "youngest" FOWObject of all players who share the view with the local player
    const FOWObject* GetYoungestFOWObject(const MapPoint pos) const;

    /// Gets the youngest fow node of all visible objects of all players who are connected
    /// with the local player via team view
    unsigned char GetYoungestFOWNodePlayer(const MapPoint pos) const;

    /// Get first found ship of this player at that point or NULL of none
    noShip* GetShip(const MapPoint pt) const;

    /// Schattierungen (vor allem FoW) neu berechnen
    void RecalcAllColors();

    /// Makes this a viewer for another player
    void ChangePlayer(unsigned player);

private:
    /// Visual node status (might be different than world if GameCommand is just sent) to hide network latency
    struct VisualMapNode{
        boost::array<unsigned char, 3> roads; // If != 0 then this road value is used (road construction) else real road is used
        BuildingQuality bq;
    };
    unsigned playerId_;
    GameWorldBase& gwb;
    TerrainRenderer tr;
    Subscribtion evVisibilityChanged, evAltitudeChanged, evRoadConstruction, evBQChanged;
    std::vector<VisualMapNode> visualNodes;

    void InitVisualData();
    void InitTerrainRenderer();
    inline void VisibilityChanged(const MapPoint& pt, unsigned player);
    inline void RoadConstructionEnded(const RoadNote& note);
    void RecalcBQ(const MapPoint& pt);
};

#endif // GameWorldViewer_h__
