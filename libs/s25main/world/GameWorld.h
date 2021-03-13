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

#include "helpers/OptionalEnum.h"
#include "world/GameWorldBase.h"
#include "gameTypes/MapCoordinates.h"
#include "gameTypes/RoadPathDirection.h"
#include <vector>

class GameInterface;
class MilitarySquares;
class noBaseBuilding;
class noBuildingSite;
class noRoadNode;
class nofActiveSoldier;
class nofAttacker;
struct PlayerInfo;
class RoadSegment;
class TerritoryRegion;

enum class TerritoryChangeReason
{
    Build,     /// Building was build (and occupied for the first time)
    Destroyed, /// Building destroyed
    Captured   /// Owner changed
};

/// "Interface-Klasse" für das Spiel
class GameWorld : public GameWorldBase
{
    /// Destroys player belongings if that pint does not belong to the player anymore
    void DestroyPlayerRests(MapPoint pt, unsigned char newOwner, const noBaseBuilding* exception);

    /// Return if there are deco-objects that can be removed when building roads
    bool HasRemovableObjForRoad(MapPoint pt) const;

    bool IsPointCompletelyVisible(const MapPoint& pt, unsigned char player, const noBaseBuilding* exception) const;
    /// Return if there is a scout (or an attacking soldier) of this player at that node with a visual range of at most
    /// the given distance. Excludes scouting ships!
    bool IsScoutingFigureOnNode(const MapPoint& pt, unsigned player, unsigned distance) const;
    /// Return true, if the point is explored by any ship of the player
    bool IsPointScoutedByShip(const MapPoint& pt, unsigned player) const;
    /// Berechnet die Sichtbarkeit eines Punktes neu für den angegebenen Spieler
    /// exception ist ein Gebäude (Spähturm, Militärgebäude), was nicht mit in die Berechnung einbezogen
    /// werden soll, z.b. weil es abgerissen wird
    void RecalcVisibility(MapPoint pt, unsigned char player, const noBaseBuilding* exception);
    /// Setzt Punkt auf jeden Fall auf sichtbar
    void MakeVisible(MapPoint pt, unsigned char player);

    /// Creates a region with territories marked around a building with the given radius
    TerritoryRegion CreateTerritoryRegion(const noBaseBuilding& building, unsigned radius,
                                          TerritoryChangeReason reason) const;
    /// Cleans the region (removes edges of terrain and applies the allied border push addon
    void CleanTerritoryRegion(TerritoryRegion& region, TerritoryChangeReason reason,
                              const noBaseBuilding& triggerBld) const;

public:
    GameWorld(const std::vector<PlayerInfo>& players, const GlobalGameSettings& gameSettings, EventManager& em);
    ~GameWorld() override;

    /// Stellt anderen Spielern/Spielobjekten das Game-GUI-Interface zur Verfüung
    GameInterface* GetGameInterface() const { return gi; }
    TradePathCache& GetTradePathCache();
    void setEconHandler(std::unique_ptr<EconomyModeHandler> handler);

    /// Kann dieser Punkt von auf Straßen laufenden Menschen betreten werden? (Kämpfe!)
    bool IsRoadNodeForFigures(MapPoint pt);
    /// Lässt alle Figuren, die auf diesen Punkt  auf Wegen zulaufen, anhalten auf dem Weg (wegen einem Kampf)
    void StopOnRoads(MapPoint pt, helpers::OptionalEnum<Direction> dir = boost::none);

    /// Sagt Bescheid, dass der Punkt wieder freigeworden ist und lässt ggf. Figuren drumherum wieder weiterlaufen
    void RoadNodeAvailable(MapPoint pt);

    /// Place a flag for the player specific
    void SetFlag(MapPoint pt, unsigned char player);
    /// Flagge soll zerstrört werden
    void DestroyFlag(MapPoint pt, unsigned char playerId);
    /// Baustelle setzen
    void SetBuildingSite(BuildingType type, MapPoint pt, unsigned char player);
    /// Gebäude bzw Baustelle abreißen
    void DestroyBuilding(MapPoint pt, unsigned char player);

    /// Find a path for people using roads.
    RoadPathDirection FindHumanPathOnRoads(const noRoadNode& start, const noRoadNode& goal, unsigned* length = nullptr,
                                           MapPoint* firstPt = nullptr, const RoadSegment* forbidden = nullptr);
    /// Find a path for wares using roads.
    RoadPathDirection FindPathForWareOnRoads(const noRoadNode& start, const noRoadNode& goal,
                                             unsigned* length = nullptr, MapPoint* firstPt = nullptr,
                                             unsigned max = std::numeric_limits<unsigned>::max());
    /// Prüft, ob eine Schiffsroute noch Gültigkeit hat
    bool CheckShipRoute(MapPoint start, const std::vector<Direction>& route, unsigned pos, MapPoint* dest);
    /// Find a route for trade caravanes
    helpers::OptionalEnum<Direction> FindTradePath(MapPoint start, MapPoint dest, unsigned char player,
                                                   unsigned max_route = 0xffffffff, bool random_route = false,
                                                   std::vector<Direction>* route = nullptr,
                                                   unsigned* length = nullptr) const;
    /// Check whether trade path (starting from point @param start and at index @param startRouteIdx) is still valid.
    /// Optionally returns destination pt
    bool CheckTradeRoute(MapPoint start, const std::vector<Direction>& route, unsigned pos, unsigned char player,
                         MapPoint* dest = nullptr) const;

    /// setzt den Straßen-Wert um den Punkt X,Y.
    void SetPointRoad(MapPoint pt, Direction dir, PointRoad type);

    /// Baut eine Straße ( nicht nur visuell, sondern auch wirklich )
    void BuildRoad(unsigned char playerId, bool boat_road, MapPoint start, const std::vector<Direction>& route);

    /// Recalculates the ownership around a military building
    void RecalcTerritory(const noBaseBuilding& building, TerritoryChangeReason reason);

    /// Berechnet das Land in einem bestimmten Bereich um ein aktuelles Militärgebäude rum neu und gibt zurück ob sich
    /// etwas verändern würde (auf für ki wichtigem untergrund) wenn das Gebäude zerstört werden würde
    bool DoesDestructionChangeTerritory(const noBaseBuilding& building) const;

    /// Greift ein Militärgebäude auf x,y an (entsendet dafür die Soldaten etc.)
    void Attack(unsigned char player_attacker, MapPoint pt, unsigned short soldiers_count, bool strong_soldiers);
    /// Greift ein Militäregebäude mit Schiffen an
    void AttackViaSea(unsigned char player_attacker, MapPoint pt, unsigned short soldiers_count, bool strong_soldiers);

    MilitarySquares& GetMilitarySquares();

    /// Lässt alles spielerische abbrennen, indem es alle Flaggen der Spieler zerstört
    void Armageddon();

    /// Lässt alles spielerische eines Spielers abbrennen, indem es alle Flaggen eines Spieler zerstört
    void Armageddon(unsigned char player);

    /// Ist der Punkt ein geeigneter Platz zum Warten vor dem Militärgebäude
    bool ValidWaitingAroundBuildingPoint(MapPoint pt, MapPoint center);
    /// Is this point a valid point for the given soldier to fight?
    bool IsValidPointForFighting(MapPoint pt, const nofActiveSoldier& soldier, bool avoid_military_building_flags);

    /// Berechnet die Sichtbarkeiten neu um einen Punkt mit radius
    void RecalcVisibilitiesAroundPoint(MapPoint pt, MapCoord radius, unsigned char player,
                                       const noBaseBuilding* exception);
    /// Setzt die Sichtbarkeiten um einen Punkt auf sichtbar (aus Performancegründen Alternative zu oberem)
    void MakeVisibleAroundPoint(MapPoint pt, MapCoord radius, unsigned char player);
    /// Bestimmt bei der Bewegung eines spähenden Objekts die Sichtbarkeiten an den Rändern neu
    void RecalcMovingVisibilities(MapPoint pt, unsigned char player, MapCoord radius, Direction moving_dir,
                                  MapPoint* enemy_territory);

    /// Return whether this is a border node (node belongs to player, but not all others around)
    bool IsBorderNode(MapPoint pt, unsigned char owner) const;

    // Konvertiert Ressourcen zwischen Typen hin und her oder löscht sie.
    // Für Spiele ohne Gold.
    void ConvertMineResourceTypes(ResourceType from, ResourceType to);
    // Setup resources like gold and water after loading a new map
    void SetupResources();

    // Fills water depending on terrain and Addon setting
    void PlaceAndFixWater();

    /// Gründet vom Schiff aus eine neue Kolonie, gibt true zurück bei Erfolg
    bool FoundColony(unsigned harbor_point, unsigned char player, unsigned short seaId);
    /// Registriert eine Baustelle eines Hafens, die vom Schiff aus gesetzt worden ist
    void AddHarborBuildingSiteFromSea(noBuildingSite* building_site)
    {
        harbor_building_sites_from_sea.push_back(building_site);
    }
    /// Removes it. It is allowed to be called with a regular harbor building site (no-op in that case)
    void RemoveHarborBuildingSiteFromSea(noBuildingSite* building_site);
    /// Gibt zurück, ob eine bestimmte Baustellen eine Baustelle ist, die vom Schiff aus errichtet wurde
    bool IsHarborBuildingSiteFromSea(const noBuildingSite* building_site) const;
    /// Liefert eine Liste der Hafenpunkte, die von einem bestimmten Hafenpunkt erreichbar sind
    std::vector<unsigned> GetUnexploredHarborPoints(unsigned hbIdToSkip, unsigned seaId, unsigned playerId) const;

    /// Writeable access to node. Use only for initial map setup!
    MapNode& GetNodeWriteable(MapPoint pt);
    /// Recalculates where border stones should be done after a change in the given region
    void RecalcBorderStones(Position startPt, Extent areaSize);

    /// Create Trade graphs
    void CreateTradeGraphs() final;

protected:
    void VisibilityChanged(MapPoint pt, unsigned player, Visibility oldVis, Visibility newVis) override;
};
