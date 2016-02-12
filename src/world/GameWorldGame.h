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

#ifndef GameWorldGame_h__
#define GameWorldGame_h__

#include "world/GameWorldBase.h"
#include "gameTypes/MapTypes.h"
#include "world/TradeRoute.h"
#include <vector>

class TradeGraph;
class nobBaseWarehouse;
class TerritoryRegion;

/// "Interface-Klasse" für das Spiel
class GameWorldGame : public virtual GameWorldBase
{
    /// Zerstört Spielerteile auf einem Punkt, wenn der Punkt dem Spieler nun nich mehr gehört
    void DestroyPlayerRests(const MapPoint pt, const unsigned char new_player, const noBaseBuilding* exception, bool allowdestructionofmilbuildings=true);

    /// Prüft, ob auf diesem Punkt Deko-Objekte liegen, die für den Wegbau entfernt werden können
    bool IsObjectionableForRoad(const MapPoint pt);


    /// Punkt vollständig sichtbar?
    bool IsPointCompletelyVisible(const MapPoint pt, const unsigned char player, const noBaseBuilding* const exception) const;
    /// Prüft, ob auf angegebenen Punkt sich ein Späher von player befindet
    bool IsScoutingFigureOnNode(const MapPoint pt, const unsigned player, const unsigned distance) const;
    /// Berechnet die Sichtbarkeit eines Punktes neu für den angegebenen Spieler
    /// exception ist ein Gebäude (Spähturm, Militärgebäude), was nicht mit in die Berechnugn einbezogen
    /// werden soll, z.b. weil es abgerissen wird
    void RecalcVisibility(const MapPoint pt, const unsigned char player, const noBaseBuilding* const exception);
    /// Setzt Punkt auf jeden Fall auf sichtbar
    void MakeVisible(const MapPoint pt,  const unsigned char player);

    /// Creates a region with territories marked around a building with the given radius
    TerritoryRegion CreateTerritoryRegion(const noBaseBuilding& building, const unsigned short radius, const bool destroyed) const;
    /// Recalculates where border stones should be after a change in the given region
    void RecalcBorderStones(const TerritoryRegion& region);

protected:

    /// Create Trade graphs
    void CreateTradeGraphs();

public:

    GameWorldGame();
    virtual ~GameWorldGame();

    /// Stellt anderen Spielern/Spielobjekten das Game-GUI-Interface zur Verfüung
    inline GameInterface* GetGameInterface() const { return gi; }

    /// Berechnet Bauqualitäten an Punkt x;y und den ersten Kreis darum neu
    void RecalcBQAroundPoint(const MapPoint pt);
    /// Berechnet Bauqualitäten wie bei letzterer Funktion, bloß noch den 2. Kreis um x;y herum
    void RecalcBQAroundPointBig(const MapPoint pt);

    /// Prüft, ob dieser Punkt von Menschen betreten werden kann
    bool IsNodeForFigures(const MapPoint pt) const;
    /// Kann dieser Punkt von auf Straßen laufenden Menschen betreten werden? (Kämpfe!)
    bool IsRoadNodeForFigures(const MapPoint pt, const unsigned char dir);
    /// Lässt alle Figuren, die auf diesen Punkt  auf Wegen zulaufen, anhalten auf dem Weg (wegen einem Kampf)
    void StopOnRoads(const MapPoint pt, const unsigned char dir = 0xff);

    /// Sagt Bescheid, dass der Punkt wieder freigeworden ist und lässt ggf. Figuren drumherum wieder weiterlaufen
    void RoadNodeAvailable(const MapPoint pt);

    /// Flagge an x,y setzen, dis_dir ist der aus welche Richtung der Weg kommt, wenn man einen Weg mit Flagge baut
    /// kann ansonsten auf 255 gesetzt werden
    void SetFlag(const MapPoint pt, const unsigned char player, const unsigned char dis_dir = 255);
    /// Flagge soll zerstrört werden
    void DestroyFlag(const MapPoint pt);
    /// Baustelle setzen
    void SetBuildingSite(const BuildingType type, const MapPoint pt, const unsigned char player);
    /// Gebäude bzw Baustelle abreißen
    void DestroyBuilding(const MapPoint pt, const unsigned char playe);

    /// Wegfindung für Menschen im Straßennetz
    unsigned char FindHumanPathOnRoads(const noRoadNode& start, const noRoadNode& goal, unsigned* length = NULL, MapPoint* firstPt = NULL, const RoadSegment* const forbidden = NULL);
    /// Wegfindung für Waren im Straßennetz
    unsigned char FindPathForWareOnRoads(const noRoadNode& start, const noRoadNode& goal, unsigned* length = NULL, MapPoint* firstPt = NULL, unsigned max = std::numeric_limits<unsigned>::max());
    /// Prüft, ob eine Schiffsroute noch Gültigkeit hat
    bool CheckShipRoute(const MapPoint start, const std::vector<unsigned char>& route, const unsigned pos,
        MapPoint* dest);
    /// Find a route for trade caravanes
    unsigned char FindTradePath(const MapPoint start,
        const MapPoint dest, const unsigned char player, const unsigned max_route = 0xffffffff, const bool random_route = false,
        std::vector<unsigned char> * route = NULL, unsigned* length = NULL,
        const bool record = false) const;
    /// Check whether trade path is still valid
    bool CheckTradeRoute(const MapPoint start, const std::vector<unsigned char>& route, const unsigned pos, const unsigned char player,
        MapPoint * dest = NULL) const;


    /// setzt den Straßen-Wert an der Stelle X,Y (berichtigt).
    void SetRoad(const MapPoint pt, unsigned char dir, unsigned char type);

    /// setzt den Straßen-Wert um den Punkt X,Y.
    void SetPointRoad(const MapPoint pt, unsigned char dir, unsigned char type);

    /// Funktionen aus ehemaligen Game
    /// Baut eine Straße ( nicht nur visuell, sondern auch wirklich )
    void BuildRoad(const unsigned char playerid, const bool boat_road, const MapPoint start, const std::vector<unsigned char>& route);
    /// Reißt eine Straße ab
    void DestroyRoad(const MapPoint pt, const unsigned char dir);
    /// baut eine Straße aus
    void UpgradeRoad(const MapPoint pt, const unsigned char dir);

    /// Berechnet das Land in einem bestimmten Bereich (um ein neues, abgerissenes oder eingenommenes
    /// Militärgebäude rum) neu, destroyed gibt an, ob building abgerissen wurde und somit nicht einberechnet werden soll
    void RecalcTerritory(const noBaseBuilding& building, const bool destroyed, const bool newBuilt);

    /// Berechnet das Land in einem bestimmten Bereich um ein aktuelles Militärgebäude rum neu und gibt zurück ob sich etwas verändern würde (auf für ki wichtigem untergrund) wenn das Gebäude zerstört werden würde
    bool DoesTerritoryChange(const noBaseBuilding& building, const bool destroyed, const bool newBuilt) const;

    /// Greift ein Militärgebäude auf x,y an (entsendet dafür die Soldaten etc.)
    void Attack(const unsigned char player_attacker, const MapPoint pt, const unsigned short soldiers_count, const bool strong_soldiers);
    /// Greift ein Militäregebäude mit Schiffen an
    void AttackViaSea(const unsigned char player_attacker, const MapPoint pt, const unsigned short soldiers_count, const bool strong_soldiers);

    /// Fügt einen Katapultstein der Welt hinzu, der gezeichnt werden will
    void AddCatapultStone(CatapultStone* cs);
    void RemoveCatapultStone(CatapultStone* cs);
    MilitarySquares& GetMilitarySquares();

    /// Lässt alles spielerische abbrennen, indem es alle Flaggen der Spieler zerstört
    void Armageddon();

    /// Lässt alles spielerische eines Spielers abbrennen, indem es alle Flaggen eines Spieler zerstört
    void Armageddon(const unsigned char player);

    /// Sagt der GW Bescheid, dass ein Objekt von Bedeutung an x,y vernichtet wurde, damit dieser
    /// dass ggf. an den WindowManager weiterleiten kann, damit auch ein Fenster wieder geschlossen wird
    virtual void ImportantObjectDestroyed(const MapPoint pt) = 0;
    /// Sagt, dass ein Militärgebäude eingenommen wurde und ggf. ein entsprechender "Fanfarensound" abgespielt werden sollte
    virtual void MilitaryBuildingCaptured(const MapPoint pt, const unsigned char player) = 0;

    /// Ist der Punkt ein geeigneter Platz zum Warten vor dem Militärgebäude
    bool ValidWaitingAroundBuildingPoint(const MapPoint pt, nofAttacker* attacker, const MapPoint center);
    /// Geeigneter Punkt für Kämpfe?
    bool ValidPointForFighting(const MapPoint pt, const bool avoid_military_building_flags, nofActiveSoldier* exception = NULL);

    /// Berechnet die Sichtbarkeiten neu um einen Punkt mit radius
    void RecalcVisibilitiesAroundPoint(const MapPoint pt, const MapCoord radius, const unsigned char player, const noBaseBuilding* const exception);
    /// Setzt die Sichtbarkeiten um einen Punkt auf sichtbar (aus Performancegründen Alternative zu oberem)
    void SetVisibilitiesAroundPoint(const MapPoint pt, const MapCoord radius, const unsigned char player);
    /// Bestimmt bei der Bewegung eines spähenden Objekts die Sichtbarkeiten an
    /// den Rändern neu
    void RecalcMovingVisibilities(const MapPoint pt, const unsigned char player, const MapCoord radius,
        const unsigned char moving_dir, MapPoint * enemy_territory);

    /// Stellt fest, ob auf diesem Punkt ein Grenzstein steht (ob das Grenzgebiet ist)
    bool IsBorderNode(const MapPoint pt, const unsigned char player) const;

    // Konvertiert Ressourcen zwischen Typen hin und her oder löscht sie.
    // Für Spiele ohne Gold.
    void ConvertMineResourceTypes(unsigned char from, unsigned char to);

    /// Gründet vom Schiff aus eine neue Kolonie, gibt true zurück bei Erfolg
    bool FoundColony(const unsigned harbor_point, const unsigned char player, const unsigned short sea_id);
    /// Registriert eine Baustelle eines Hafens, die vom Schiff aus gesetzt worden ist
    void AddHarborBuildingSiteFromSea(noBuildingSite* building_site) { harbor_building_sites_from_sea.push_back(building_site); }
    /// Removes it. It is allowed to be called with a regular harbor building site (no-op in that case)
    void RemoveHarborBuildingSiteFromSea(noBuildingSite* building_site);
    /// Gibt zurück, ob eine bestimmte Baustellen eine Baustelle ist, die vom Schiff aus errichtet wurde
    bool IsHarborBuildingSiteFromSea(const noBuildingSite* building_site) const;
    /// Liefert eine Liste der Hafenpunkte, die von einem bestimmten Hafenpunkt erreichbar sind
    std::vector<unsigned> GetHarborPointsWithinReach(const unsigned hp) const;

    /// Returns true, if the given (map)-resource is available at that node
    bool IsResourcesOnNode(const MapPoint pt, const unsigned char type) const;
};

#endif // GameWorldGame_h__
