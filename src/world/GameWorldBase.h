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

#ifndef GameWorldBase_h__
#define GameWorldBase_h__

#include "TerrainRenderer.h"
#include "buildings/nobBaseMilitary.h"
#include "world/World.h"
#include "helpers/Deleter.h"
#include <boost/interprocess/smart_ptr/unique_ptr.hpp>

class RoadPathFinder;
class FreePathFinder;
class GameInterface;
class CatapultStone;
class noBuildingSite;
class GameClientPlayerList;
class GameClientPlayer;
class nobBaseMilitary;
class noNothing;
class fowNothing;
class nofPassiveSoldier;
class nobHarborBuilding;
class GameWorldBase;
class noFlag;
struct lua_State;

/// Grundlegende Klasse, die die Gamewelt darstellt, enthält nur deren Daten
class GameWorldBase: public World
{
    boost::interprocess::unique_ptr<RoadPathFinder, Deleter<RoadPathFinder> > roadPathFinder;
    boost::interprocess::unique_ptr<FreePathFinder, Deleter<FreePathFinder> > freePathFinder;
protected:

    TerrainRenderer tr;

    /// Interface zum GUI
    GameInterface* gi;

    /// Baustellen von Häfen, die vom Schiff aus errichtet wurden
    std::list<noBuildingSite*> harbor_building_sites_from_sea;

    GameClientPlayerList* players;

public:
    GameWorldBase();
    virtual ~GameWorldBase();

    // Grundlegende Initialisierungen
    void Init() override;

    /// Setzt GameInterface
    void SetGameInterface(GameInterface* const gi) { this->gi = gi; }

    /// Gibt Dynamische Objekte, die von einem bestimmten Punkt aus laufen oder dort stehen sowie andere Objekte,
    /// die sich dort befinden, zurück
    std::vector<noBase*> GetDynamicObjectsFrom(const MapPoint pt) const;

    /// Kann a node be used for a road (no flag/bld, no other road, no danger...)
    /// Should only be used for the points between the 2 flags of a road
    bool RoadAvailable(const bool boat_road, const MapPoint pt, const bool visual = true) const;
    /// Prüft ob exakt die gleiche Straße schon gebaut wurde
    bool RoadAlreadyBuilt(const bool boat_road, const MapPoint start, const std::vector<unsigned char>& route);
    /// Bauqualitäten berechnen, bei flagonly gibt er nur 1 zurück, wenn eine Flagge möglich ist
    BuildingQuality CalcBQ(const MapPoint pt, const unsigned char player, const bool flagonly = false, const bool visual = true, const bool ignore_player = false) const;
    /// Setzt die errechnete BQ gleich mit
    void CalcAndSetBQ(const MapPoint pt, const unsigned char player, const bool flagonly = false, const bool visual = true)
    { GetNode(pt).bq = CalcBQ(pt, player, flagonly, visual); }

    /// Prüft, ob der Pkut zu dem Spieler gehört (wenn er der Besitzer ist und es false zurückliefert, ist es Grenzgebiet)
    bool IsPlayerTerritory(const MapPoint pt) const;
    /// Berechnet BQ bei einer gebauten Straße
    void CalcRoad(const MapPoint pt, const unsigned char player);
    /// Ist eine Flagge irgendwo um x,y ?
    bool FlagNear(const MapPoint pt) const;
    /// Prüft, ob sich in unmittelbarer Nähe (im Radius von 4) Militärgebäude befinden
    bool IsMilitaryBuildingNearNode(const MapPoint nPt, const unsigned char player) const;

    /// Test, ob auf dem besagten Punkt ein Militärgebäude steht
    bool IsMilitaryBuilding(const MapPoint pt) const;

    /// Erstellt eine Liste mit allen Militärgebäuden in der Umgebung, radius bestimmt wie viele Kästchen nach einer Richtung im Umkreis
    sortedMilitaryBlds LookForMilitaryBuildings(const MapPoint pt, unsigned short radius) const;

    /// Prüft, ob von einem bestimmten Punkt aus der Untergrund für Figuren zugänglich ist (kein Wasser,Lava,Sumpf)
    bool IsNodeToNodeForFigure(const MapPoint pt, const unsigned dir) const;

    /* Wegfindung auf Straßen - Basisroutine
    bool FindPathOnRoads(const noRoadNode& start, const noRoadNode& goal,
        const bool ware_mode, unsigned* length, unsigned char* first_dir, MapPoint* next_harbor,
        const RoadSegment* const forbidden, const bool record = true, const unsigned max = 0xFFFFFFFF) const;*/
    /// Findet einen Weg für Figuren
    unsigned char FindHumanPath(const MapPoint start,
        const MapPoint dest, const unsigned max_route = 0xFFFFFFFF, const bool random_route = false, unsigned* length = NULL, const bool record = true) const;
    /// Wegfindung für Schiffe auf dem Wasser
    bool FindShipPath(const MapPoint start, const MapPoint dest, std::vector<unsigned char> * route, unsigned* length, const unsigned max_length = 200);
    RoadPathFinder& GetRoadPathFinder() const { return *roadPathFinder; }
    FreePathFinder& GetFreePathFinder() const { return *freePathFinder; }

    /// Baut eine (bisher noch visuell gebaute) Straße wieder zurück
    void RemoveVisualRoad(const MapPoint start, const std::vector<unsigned char>& route);

    /// x,y ist ein Punkt auf irgendeinem Wegstck, gibt die Flagge zurück
    noFlag* GetRoadFlag(MapPoint pt, unsigned char& dir, unsigned last_i = 255);

    /// Erzeugt eine GUI-ID für die Fenster von Map-Objekten
    unsigned CreateGUIID(const MapPoint pt) const { return 1000 + width_ * pt.y + pt.x; }
    /// Gibt Terrainkoordinaten zurück
    Point<float> GetNodePos(const MapPoint pt){ return tr.GetNodePos(pt); }

    /// Verändert die Höhe eines Punktes und die damit verbundenen Schatten
    void ChangeAltitude(const MapPoint pt, const unsigned char altitude);

    /// Ermittelt Sichtbarkeit eines Punktes auch unter Einbeziehung der Verbündeten des jeweiligen Spielers
    Visibility CalcWithAllyVisiblity(const MapPoint pt, const unsigned char player) const;

    /// Gibt die Anzahl an Hafenpunkten zurück
    unsigned GetHarborPointCount() const { return harbor_pos.size() - 1; }
    /// Ist es an dieser Stelle für einen Spieler möglich einen Hafen zu bauen
    bool IsHarborPointFree(const unsigned harbor_id, const unsigned char player,
        const unsigned short sea_id) const;
    /// Ermittelt, ob ein Punkt Küstenpunkt ist, d.h. Zugang zu einem schiffbaren Meer hat
    /// und gibt ggf. die Meeres-ID zurück, ansonsten 0
    unsigned short IsCoastalPoint(const MapPoint pt) const;
    /// Ermittelt, ob ein Punkt Küstenpunkt ist, d.h. Zugang zu einem schiffbaren Meer, an dem auch mindestens 1 Hafenplatz liegt, hat
    /// und gibt ggf. die Meeres-ID zurück, ansonsten 0
    unsigned short IsCoastalPointToSeaWithHarbor(const MapPoint pt) const;
    /// Grenzt der Hafen an ein bestimmtes Meer an?
    bool IsAtThisSea(const unsigned harbor_id, const unsigned short sea_id) const;
    /// Gibt den Punkt eines bestimmtes Meeres um den Hafen herum an, sodass Schiffe diesen anfahren können
    MapPoint GetCoastalPoint(const unsigned harbor_id, const unsigned short sea_id) const;
    /// Sucht freie Hafenpunkte, also wo noch ein Hafen gebaut werden kann
    unsigned GetNextFreeHarborPoint(const MapPoint pt, const unsigned origin_harbor_id, const unsigned char dir,
        const unsigned char player) const;
    /// Gibt die angrenzenden Sea-IDs eines Hafenpunktes zurück
    void GetSeaIDs(const unsigned harbor_id, unsigned short* sea_ids) const;
    /// Berechnet die Entfernung zwischen 2 Hafenpunkten
    unsigned CalcHarborDistance(const unsigned habor_id1, const unsigned harbor_id2) const;
    /// Bestimmt für einen beliebigen Punkt auf der Karte die Entfernung zum nächsten Hafenpunkt
    unsigned CalcDistanceToNearestHarbor(const MapPoint pos) const;
    /// returns true when a harborpoint is in SEAATTACK_DISTANCE for figures!
    bool IsAHarborInSeaAttackDistance(const MapPoint pos) const;

    void SetPlayers(GameClientPlayerList* pls) { players = pls; }
    /// Liefert einen Player zurück
    GameClientPlayer& GetPlayer(const unsigned int id) const;
    unsigned GetPlayerCt() const;

    struct PotentialSeaAttacker
    {
        /// Comparator that compares only the soldier pointer
        struct CmpSoldier
        {
            nofPassiveSoldier* const search;
            CmpSoldier(nofPassiveSoldier* const search): search(search){}
            bool operator()(const PotentialSeaAttacker& other)
            {
                return other.soldier == search;
            }
        };
        /// Soldat, der als Angreifer in Frage kommt
        nofPassiveSoldier* soldier;
        /// Hafen, den der Soldat zuerst ansteuern soll
        nobHarborBuilding* harbor;
        /// Entfernung Hafen-Hafen (entscheidende)
        unsigned distance;

        PotentialSeaAttacker(nofPassiveSoldier* soldier, nobHarborBuilding* harbor, unsigned distance): soldier(soldier), harbor(harbor), distance(distance){}

        /// Komperator zum Sortieren
        bool operator<(const PotentialSeaAttacker& pa) const;
    };


    /// Liefert Hafenpunkte im Umkreis von einem bestimmten Militärgebäude
    std::vector<unsigned> GetHarborPointsAroundMilitaryBuilding(const MapPoint pt) const;
    /// returns all sea_ids from which a given building can be attacked by sea
    std::vector<unsigned> GetValidSeaIDsAroundMilitaryBuildingForAttack(const MapPoint pt, std::vector<bool>& use_seas, const unsigned char player_attacker)const;
    /// returns all sea_ids found in the given vector from which a given building can be attacked by sea
    void GetValidSeaIDsAroundMilitaryBuildingForAttackCompare(const MapPoint pt, std::vector<unsigned short>& use_seas, const unsigned char player_attacker)const;
    /// Sucht verfügbare Soldaten, um dieses Militärgebäude mit einem Seeangriff anzugreifen
    std::vector<PotentialSeaAttacker> GetAvailableSoldiersForSeaAttack(const unsigned char player_attacker, const MapPoint pt) const;
    /// Gibt Anzahl oder geschätzte Stärke(rang summe + anzahl) der verfügbaren Soldaten die zu einem Schiffsangriff starten können von einer bestimmten sea id aus
    unsigned int GetAvailableSoldiersForSeaAttackAtSea(const unsigned char player_attacker, unsigned short seaid, bool count = true) const;

protected:

    /// Für abgeleitete Klasse, die dann das Terrain entsprechend neu generieren kann
    virtual void AltitudeChanged(const MapPoint pt) = 0;
    /// Für abgeleitete Klasse, die dann das Terrain entsprechend neu generieren kann
    virtual void VisibilityChanged(const MapPoint pt) = 0;

    /// Gibt nächsten Hafenpunkt in einer bestimmten Richtung zurück, bzw. 0, wenn es keinen gibt
    unsigned GetNextHarborPoint(const MapPoint pt, const unsigned origin_harbor_id, const unsigned char dir,
        const unsigned char player, bool (GameWorldBase::*IsPointOK)(const unsigned, const unsigned char, const unsigned short) const) const;

    lua_State* lua;

    static int LUA_DisableBuilding(lua_State* L);
    static int LUA_EnableBuilding(lua_State* L);
    static int LUA_SetRestrictedArea(lua_State* L);
    static int LUA_ClearResources(lua_State *L);
    static int LUA_AddWares(lua_State* L);
    static int LUA_AddPeople(lua_State* L);
    static int LUA_GetGF(lua_State *L);
    static int LUA_Log(lua_State *L);
    static int LUA_Chat(lua_State *L);
    static int LUA_MissionStatement(lua_State *L);
    static int LUA_PostMessage(lua_State *L);
    static int LUA_PostMessageWithLocation(lua_State *L);
    static int LUA_GetPlayerCount(lua_State *L);
    static int LUA_GetBuildingCount(lua_State *L);
    static int LUA_GetWareCount(lua_State *L);
    static int LUA_GetPeopleCount(lua_State *L);
    static int LUA_AddEnvObject(lua_State *L);
    static int LUA_AIConstructionOrder(lua_State *L);
    static int LUA_AddStaticObject(lua_State *L);
    static int LUA_PostNewBuildings(lua_State *L);

public:
    void LUA_EventExplored(unsigned player, const MapPoint pt);
    void LUA_EventOccupied(unsigned player, const MapPoint pt);
    void LUA_EventStart();
    void LUA_EventGF(unsigned number);
    void LUA_EventResourceFound(unsigned char player, const MapPoint pt, const unsigned char type, const unsigned char quantity);
};

#endif // GameWorldBase_h__