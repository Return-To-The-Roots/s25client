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

/// This is a players Viewer on the GameWorld
class GameWorldViewer
{
    unsigned player_;
    GameWorldBase& gwb;
    TerrainRenderer tr;
    Subscribtion evVisibilityChanged, evAltitudeChanged;
public:

    GameWorldViewer(unsigned player, GameWorldBase& gwb);

    GameWorldBase& GetWorld() { return gwb; }
    const GameWorldBase& GetWorld() const { return gwb; }
    TerrainRenderer& GetTerrainRenderer() { return tr; }
    GameClientPlayer& GetPlayer();
    const GameClientPlayer& GetPlayer() const;
    unsigned GetPlayerID() const { return player_; }

    /// Get number of soldiers that can attack that point
    unsigned GetAvailableSoldiersForAttack(const MapPoint pt);
    /// Get number of soldiers for attacking a point via sea
    unsigned GetAvailableSoldiersForSeaAttackCount(const MapPoint pt) const;

    /// Get BQ for this player
    BuildingQuality GetBQ(const MapPoint& pt) const;
    /// Ermittelt Sichtbarkeit eines Punktes für den lokalen Spieler, berücksichtigt ggf. Teamkameraden
    Visibility GetVisibility(const MapPoint pt) const;
    /// Returns true, if we own this point (but may not be our territory if this is a border point)
    bool IsOwner(const MapPoint& pt) const;
    const MapNode& GetNode(const MapPoint& pt) const;
    MapPoint GetNeighbour(const MapPoint pt, const Direction dir) const;

    /// liefert sichtbare Strasse, im Nebel entsprechend die FoW-Strasse
    unsigned char GetVisibleRoad(const MapPoint pt, unsigned char dir, const Visibility visibility) const;

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
    inline void VisibilityChanged(const MapPoint& pt, unsigned player);
    void SubscribeToEvents();
};

#endif // GameWorldViewer_h__
