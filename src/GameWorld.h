// $Id: GameWorld.h 9578 2015-01-23 08:28:58Z marcus $
//
// Copyright (c) 2005 - 2011 Settlers Freaks (sf-team at siedler25.org)
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

#ifndef OBJMANAGER_H_
#define OBJMANAGER_H_


#include "gameData/GameConsts.h"
#include "gameTypes/MapTypes.h"
#include "nodeObjs/noNothing.h"
#include "RoadSegment.h"
#include "gameData/MilitaryConsts.h"
#include "EventManager.h"
#include "TerrainRenderer.h"
#include "defines.h"
#include "Point.h"
#include "GamePlayerList.h"
#include "TradeGraph.h"
#include "buildings/nobBaseMilitary.h"
#include <vector>
#include <list>
#include <set>
#include <cstddef>

class noEnvObject;
class noGranite;
class noTree;
class noFlag;
class noFigure;
class nobHQ;
class nobFarm;
class nobCatapult;
class ncFlag;
class noRoadNode;
class noCoordBase;
class SerializedGameData;
class CatapultStone;
class nofAttacker;
class MouseCoords;
class noBuilding;
class GameInterface;
class GameWorldBase;
class glArchivItem_Map;
class noShip;
class nofActiveSoldier;
class TradeGraph;

struct RoadsBuilding;
class FOWObject;

struct lua_State;

typedef bool (*FP_Node_OK_Callback)(const GameWorldBase& gwb, const MapPoint pt, const unsigned char dir, const void* param);

/// Eigenschaften von einem Punkt auf der Map
struct MapNode
{
    /// Straßen
    unsigned char roads[3];
    bool roads_real[3];
    /// Höhe
    unsigned char altitude;
    /// Schattierung
    unsigned char shadow;
    /// Terrain
    unsigned char t1, t2;
    /// Ressourcen
    unsigned char resources;
    /// Reservierungen
    bool reserved;
    /// Eigentümer (Spieler)
    unsigned char owner;
    /// Grenzsteine (der Punkt, und dann jeweils nach rechts, unten-links und unten-rechts die Zwischensteine)
    unsigned char boundary_stones[4];
    /// Bauqualität
    BuildingQuality bq;
    /// Visuelle Sachen für alle Spieler, die in Zusammenhang mit dem FoW stehen
    struct
    {
        /// Zeit (GF-Zeitpunkt), zu der, der Punkt zuletzt aktualisiert wurde
        unsigned last_update_time;
        /// Sichtbarkeit des Punktes
        Visibility visibility;
        /// FOW-Objekt
        FOWObject* object;
        /// Straßen im FoW
        unsigned char roads[3];
        /// Grenzsteine (der Punkt, und dann jeweils nach rechts, unten-links und unten-rechts die Zwischensteine)
        unsigned char owner;
        /// Grenzsteine (der Punkt, und dann jeweils nach rechts, unten-links und unten-rechts die Zwischensteine)
        unsigned char boundary_stones[4];
    } fow[MAX_PLAYERS];

    /// Meeres-ID, d.h. zu welchem Meer gehört dieser Punkt (0 = kein Meer)
    unsigned short sea_id;
    /// Hafenpunkt-ID (0 = kein Hafenpunkt)
    unsigned harbor_id;

    /// Objekt, welches sich dort befindet
    noBase* obj;
    /// Figuren, Kämpfe, die sich dort befinden
    std::list<noBase*> figures;
};

enum LandscapeType
{
    LT_GREENLAND = 0,
    LT_WASTELAND,
    LT_WINTERWORLD
};

/// Grundlegende Klasse, die die Gamewelt darstellt, enthält nur deren Daten
class GameWorldBase
{
    protected:

        /// Interface zum GUI
        GameInterface* gi;

        /// Breite und Höhe der Karte in Kontenpunkten
        unsigned short width, height;
        /// Landschafts-Typ
        LandscapeType lt;

        std::vector<unsigned short> handled_nodes;  /// Array von abgeklapperten Knoten und deren Punktzahlen

        /// Eigenschaften von einem Punkt auf der Map
        std::vector<MapNode> nodes;

        /// Rendert das Terrain
        TerrainRenderer tr;

        /// Informationen über die Weltmeere
        struct Sea
        {
            /// Anzahl der Knoten, welches sich in diesem Meer befinden
            unsigned nodes_count;

            Sea() : nodes_count(0) {}
            Sea(const unsigned nodes_count) : nodes_count(nodes_count) {}
        };
        std::vector<Sea> seas;

        /// Alle Hafenpositionen
        struct HarborPos
        {
            MapPoint pos;

            struct CoastalPoint
            {
                unsigned short sea_id;
            } cps[6];

            struct Neighbor
            {
                unsigned id;
                unsigned distance;

                Neighbor() {}
                Neighbor(const unsigned id, const unsigned distance)
                    : id(id), distance(distance) {}

                bool operator<(const Neighbor& two) const
                { return (distance < two.distance) || (distance == two.distance && id < two.id); }
            };

            std::vector<Neighbor> neighbors[6];

            HarborPos(){}
            HarborPos(const MapPoint pt): pos(pt){}
        };

        std::vector< HarborPos > harbor_pos;

        /// Baustellen von Häfen, die vom Schiff aus errichtet wurden
        std::list<noBuildingSite*> harbor_building_sites_from_sea;

        GameClientPlayerList* players;

    public:
        unsigned int map_size;

        std::list<CatapultStone*> catapult_stones; /// Liste von allen umherfliegenden Katapultsteinen.

        noNothing nothing; // nur Platzhalter bei der Rckgabe von GetNO
        /// Liste von Militärgebäuden (auch HQ und Haufengebäude, daher normale Gebäude) pro "Militärquadrat"
        std::vector< std::list<nobBaseMilitary*> > military_squares;

    public:

        GameWorldBase();
        virtual ~GameWorldBase();

        // Grundlegende Initialisierungen
        void Init();
        /// Aufräumen
        void Unload();

        /// Setzt GameInterface
        inline void SetGameInterface(GameInterface* const gi) { this->gi = gi; }

        /// Größe der Map abfragen
        inline unsigned short GetWidth() const { return width; }
        inline unsigned short GetHeight() const { return height; }

        /// Landschaftstyp abfragen
        LandscapeType GetLandscapeType() const { return lt; }

        /// Gibt Punkt um diesen Punkt (X-Koordinate) direkt zurück in einer Richtung von 0-5
        MapCoord GetXA(const MapCoord x, const MapCoord y, unsigned dir) const;
        MapCoord GetXA(const MapPoint pt, unsigned dir) const { return GetNeighbour(pt, dir).x; }
        /// Gibt Punkt um diesen Punkt (Y-Koordinate)  direkt zurück in einer Richtung von 0-5
        MapCoord GetYA(const MapCoord x, const MapCoord y, unsigned dir) const;
        /// Returns neighbouring point in one direction (0-5)
        MapPoint GetNeighbour(const MapPoint, unsigned dir) const;
        /// Returns neighbouring point (2nd layer: dir 0-11)
        MapPoint GetNeighbour2(const MapPoint, unsigned dir) const;
        /// Berechnet die Differenz zweier Koordinaten von x1 zu x2, wenn man berücksichtigt, dass man über den
        /// Rand weitergehen kann
        MapCoord CalcDistanceAroundBorderX(const MapCoord x1, const MapCoord x2) const;
        MapCoord CalcDistanceAroundBorderY(const MapCoord y1, const MapCoord y2) const;

        /// Ermittelt Abstand zwischen 2 Punkten auf der Map unter Berücksichtigung der Kartengrenzüberquerung
        unsigned CalcDistance(int x1, int y1, int x2, int y2) const;
        inline unsigned CalcDistance(const MapPoint p1, const MapPoint p2) const
        { return CalcDistance(p1.x, p1.y, p2.x, p2.y); }

        // Erzeugt eindeutige ID aus gegebenen X und Y-Werten
        inline unsigned MakeCoordID(const MapPoint pt) const
        { return GetIdx(pt); }

        // Returns the linear index for a map point
        inline unsigned GetIdx(const MapPoint pt) const
        { return static_cast<unsigned>(pt.y) * static_cast<unsigned>(width) + static_cast<unsigned>(pt.x); }

        /// Gibt Map-Knotenpunkt zurück
        inline const MapNode& GetNode(const MapPoint pt) const { assert(pt.x < width && pt.y < height);  return nodes[GetIdx(pt)]; }
        MapNode& GetNode(const MapPoint pt) { assert(pt.x < width && pt.y < height); return nodes[GetIdx(pt)]; }
        /// Gibt MapKnotenpunkt darum zurück
        inline const MapNode& GetNodeAround(const MapPoint pt, const unsigned i) const
        { return GetNode(GetNeighbour(pt, i));  }
        inline MapNode& GetNodeAround(const MapPoint pt, const unsigned i)
        { return GetNode(GetNeighbour(pt, i));  }

        // Gibt ein NO zurück, falls keins existiert, wird ein "Nothing-Objekt" zurückgegeben
        noBase* GetNO(const MapPoint pt);
        // Gibt ein NO zurück, falls keins existiert, wird ein "Nothing-Objekt" zurückgegeben
        const noBase* GetNO(const MapPoint pt) const;
        /// Gibt ein FOW-Objekt zurück, falls keins existiert, wird ein "Nothing-Objekt" zurückgegeben
        const FOWObject* GetFOWObject(const MapPoint pt, const unsigned spectator_player) const;
        /// Gibt den GOT des an diesem Punkt befindlichen Objekts zurück bzw. GOT_NOTHING, wenn keins existiert
        GO_Type GetGOT(const MapPoint pt) const;

        /// Gibt Figuren, die sich auf einem bestimmten Punkt befinden, zurück
        /// nicht bei laufenden Figuren oder
        const std::list<noBase*>& GetFigures(const MapPoint pt) const { return GetNode(pt).figures; }
        /// Gibt Dynamische Objekte, die von einem bestimmten Punkt aus laufen oder dort stehen sowie andere Objekte,
        /// die sich dort befinden, zurück
        std::vector<noBase*> GetDynamicObjectsFrom(const MapPoint pt) const;

        // Gibt ein spezifisches Objekt zurück
        template<typename T> inline T* GetSpecObj(const MapPoint pt) { return dynamic_cast<T*>( GetNode(pt).obj ); }
        // Gibt ein spezifisches Objekt zurück
        template<typename T> inline const T* GetSpecObj(const MapPoint pt) const { return dynamic_cast<const T*>( GetNode(pt).obj ); }

        /// Gibt ein Terrain-Dreieck um einen Punkt herum zurück.
        unsigned char GetTerrainAround(const MapPoint pt, unsigned char dir) const;
        /// Gibt das Terrain zurück, über das ein Mensch/Tier laufen müsste, von X,Y in Richtung DIR (Vorwärts).
        unsigned char GetWalkingTerrain1(const MapPoint pt, unsigned char dir) const;
        /// Gibt das Terrain zurück, über das ein Mensch/Tier laufen müsste, von X,Y in Richtung DIR (Rückwärts).
        unsigned char GetWalkingTerrain2(const MapPoint pt, unsigned char dir) const;
        /// Gibt zurück, ob ein Punkt vollständig von Wasser umgeben ist
        bool IsSeaPoint(const MapPoint pt) const;

        /// liefert den Straßen-Wert an der Stelle X,Y
        unsigned char GetRoad(const MapPoint pt, unsigned char dir, bool all = false) const;
        /// liefert den Straßen-Wert um den Punkt X,Y.
        unsigned char GetPointRoad(const MapPoint pt, unsigned char dir, bool all = false) const;
        /// liefert FOW-Straßen-Wert um den punkt X,Y
        unsigned char GetPointFOWRoad(MapPoint pt, unsigned char dir, const unsigned char viewing_player) const;

        /// Kann dorthin eine Straße gebaut werden?
        bool RoadAvailable(const bool boat_road, const MapPoint pt, unsigned char to_dir, const bool visual = true) const;
        /// Prüft ob exakt die gleiche Straße schon gebaut wurde
        bool RoadAlreadyBuilt(const bool boat_road, const MapPoint start, const std::vector<unsigned char>& route);
        /// Bauqualitäten berechnen, bei flagonly gibt er nur 1 zurück, wenn eine Flagge möglich ist
        BuildingQuality CalcBQ(const MapPoint pt, const unsigned char player, const bool flagonly = false, const bool visual = true, const bool ignore_player = false) const;
        /// Setzt die errechnete BQ gleich mit
        inline void SetBQ(const MapPoint pt, const unsigned char player, const bool flagonly = false, const bool visual = true)
        { GetNode(pt).bq = CalcBQ(pt, player, flagonly, visual); }

        /// Prüft, ob der Pkut zu dem Spieler gehört (wenn er der Besitzer ist und es false zurückliefert, ist es Grenzgebiet)
        bool IsPlayerTerritory(const MapPoint pt) const;
        /// Berechnet BQ bei einer gebauten Straße
        void CalcRoad(const MapPoint pt, const unsigned char player);
        /// Ist eine Flagge irgendwo um x,y ?
        bool FlagNear(const MapPoint pt) const;
        /// Prüft, ob sich in unmittelbarer Nähe (im Radius von 4) Militärgebäude befinden
        bool IsMilitaryBuildingNearNode(const MapPoint nPt, const unsigned char player) const;

        /// setzt den virtuellen Straßen-Wert an der Stelle X,Y (berichtigt).
        void SetVirtualRoad(const MapPoint pt, unsigned char dir, unsigned char type);
        /// setzt den virtuellen Straßen-Wert um den Punkt X,Y.
        void SetPointVirtualRoad(const MapPoint pt, unsigned char dir, unsigned char type);

        /// Test, ob auf dem besagten Punkt ein Militärgebäude steht
        bool IsMilitaryBuilding(const MapPoint pt) const;

        /// Erstellt eine Liste mit allen Militärgebäuden in der Umgebung, radius bestimmt wie viele Kästchen nach einer Richtung im Umkreis
        nobBaseMilitarySet LookForMilitaryBuildings(const MapPoint pt, unsigned short radius) const;

        /// Prüft, ob von einem bestimmten Punkt aus der Untergrund für Figuren zugänglich ist (kein Wasser,Lava,Sumpf)
        bool IsNodeToNodeForFigure(const MapPoint pt, const unsigned dir) const;

        /// Informationen, ob Grenzen überquert wurden
        struct CrossBorders
        {
            CrossBorders(void) : left(false), top(false), right(false), bottom(false) { }
            bool left, top, right, bottom;
        };


        /// Wegfindung in freiem Terrain - Basisroutine
        bool FindFreePath(const MapPoint start,
                          const MapPoint dest, const bool random_route,
                          const unsigned max_route, std::vector<unsigned char> * route, unsigned* length, unsigned char* first_dir,
                          FP_Node_OK_Callback IsNodeOK, FP_Node_OK_Callback IsNodeToDestOk, const void* param, const bool record) const;
		bool FindFreePathAlternatingConditions(const MapPoint start,
                          const MapPoint dest, const bool random_route,
                          const unsigned max_route, std::vector<unsigned char> * route, unsigned* length, unsigned char* first_dir,
                          FP_Node_OK_Callback IsNodeOK, FP_Node_OK_Callback IsNodeOKAlternate, FP_Node_OK_Callback IsNodeToDestOk, const void* param, const bool record) const;
        /// Ermittelt, ob eine freie Route noch passierbar ist und gibt den Endpunkt der Route zurück
        bool CheckFreeRoute(const MapPoint start, const std::vector<unsigned char>& route,
                            const unsigned pos, FP_Node_OK_Callback IsNodeOK, FP_Node_OK_Callback IsNodeToDestOk,
                            MapPoint* dest, const void* const param = NULL) const;
        /// Wegfindung auf Straßen - Basisroutine
        bool FindPathOnRoads(const noRoadNode* const start, const noRoadNode* const goal,
                             const bool ware_mode, unsigned* length, unsigned char* first_dir, MapPoint * next_harbor,
                             const RoadSegment* const forbidden, const bool record = true, unsigned max = 0xFFFFFFFF) const;
        /// Findet einen Weg für Figuren
        unsigned char FindHumanPath(const MapPoint start,
                                    const MapPoint dest, const unsigned max_route = 0xFFFFFFFF, const bool random_route = false, unsigned* length = NULL, const bool record = true) const;
        /// Wegfindung für Schiffe auf dem Wasser
        bool FindShipPath(const MapPoint start, const MapPoint dest, std::vector<unsigned char> * route, unsigned* length, const unsigned max_length = 200,
                          CrossBorders* cb = NULL);


        /// Baut eine (bisher noch visuell gebaute) Straße wieder zurück
        void RemoveVisualRoad(const MapPoint start, const std::vector<unsigned char>& route);

        /// x,y ist ein Punkt auf irgendeinem Wegstck, gibt die Flagge zurück
        noFlag* GetRoadFlag(MapPoint pt, unsigned char& dir, unsigned last_i = 255);

        /// Konvertiert die Koordinaten.
        MapPoint ConvertCoords(int x, int y) const;

        /// Erzeugt eine GUI-ID für die Fenster von Map-Objekten
        inline unsigned CreateGUIID(const MapPoint pt) const
        { return 1000 + width * pt.y + pt.x; }
        /// Gibt Terrainkoordinaten zurück
        inline float GetTerrainX(const MapPoint pt)
        { return tr.GetTerrainX(pt); }
        inline float GetTerrainY(const MapPoint pt)
        { return tr.GetTerrainY(pt); }

        /// Verändert die Höhe eines Punktes und die damit verbundenen Schatten
        void ChangeAltitude(const MapPoint pt, const unsigned char altitude);

        /// Ermittelt Sichtbarkeit eines Punktes auch unter Einbeziehung der Verbündeten des jeweiligen Spielers
        Visibility CalcWithAllyVisiblity(const MapPoint pt, const unsigned char player) const;

        /// Gibt die Anzahl an Hafenpunkten zurück
        inline unsigned GetHarborPointCount() const
        { return harbor_pos.size() - 1; }
        /// Ist es an dieser Stelle für einen Spieler möglich einen Hafen zu bauen
        bool IsHarborPointFree(const unsigned harbor_id, const unsigned char player,
                               const unsigned short sea_id) const;
        /// Gibt die Koordinaten eines bestimmten Hafenpunktes zurück
        MapPoint GetHarborPoint(const unsigned harbor_id) const;
        /// Gibt die ID eines Hafenpunktes zurück
        inline unsigned GetHarborPointID(const MapPoint pt) const
        { return GetNode(pt).harbor_id; }
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



        inline void SetPlayers(GameClientPlayerList* pls) { players = pls; }
        /// Liefert einen Player zurück
        inline GameClientPlayer* GetPlayer(const unsigned int id) const { return players->getElement(id); }

        struct PotentialSeaAttacker
        {
            /// Soldat, der als Angreifer in Frage kommt
            nofPassiveSoldier* soldier;
            /// Hafen, den der Soldat zuerst ansteuern soll
            nobHarborBuilding* harbor;
            /// Entfernung Hafen-Hafen (entscheidende)
            unsigned distance;

            /// Komperator zum Sortieren
            bool operator<(const PotentialSeaAttacker& pa) const;
        };


        /// Liefert Hafenpunkte im Umkreis von einem bestimmten Militärgebäude
        void GetHarborPointsAroundMilitaryBuilding(const MapPoint pt, std::vector<unsigned> * harbor_points) const;
        /// returns all sea_ids from which a given building can be attacked by sea
        void GetValidSeaIDsAroundMilitaryBuildingForAttack(const MapPoint pt, std::vector<bool> * use_seas, const unsigned char player_attacker, std::vector<unsigned>*harbor_points)const;
        /// returns all sea_ids found in the given vector from which a given building can be attacked by sea
        void GetValidSeaIDsAroundMilitaryBuildingForAttackCompare(const MapPoint pt, std::vector<unsigned short> * use_seas, const unsigned char player_attacker)const;
        /// Sucht verfügbare Soldaten, um dieses Militärgebäude mit einem Seeangriff anzugreifen
        void GetAvailableSoldiersForSeaAttack(const unsigned char player_attacker, const MapPoint pt, std::list<PotentialSeaAttacker> * attackers) const;
        /// Gibt Anzahl oder geschätzte Stärke(rang summe + anzahl) der verfügbaren Soldaten die zu einem Schiffsangriff starten können von einer bestimmten sea id aus
        unsigned int GetAvailableSoldiersForSeaAttackAtSea(const unsigned char player_attacker, unsigned short seaid, bool count = true) const;


    protected:

        /// Berechnet die Schattierung eines Punktes neu
        void RecalcShadow(const MapPoint pt);

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

class GameWorldView
{
        /// Selektierter Punkt
        MapPoint selPt;
        Point<int> selO;

        /// Koordinaten auf der Map anzeigen (zum Debuggen)?
        bool show_coordinates;

        bool show_bq;    ///< Bauqualitäten-Anzeigen ein oder aus
        bool show_names; ///< Gebäudenamen-Anzeigen ein oder aus
        bool show_productivity; ///< Produktivität-Anzeigen ein oder aus

        /// Scrolling-Zeug
        Point<int> offset;
        /// Letzte Scrollposition, an der man war, bevor man weggesprungen ist
        Point<int> lastOffset;
        /// Erster gezeichneter Map-Punkt
        Point<int> firstPt;
        /// Letzter gezeichneter Map-Punkt
        Point<int> lastPt;

        GameWorldViewer* gwv;

    protected:
        unsigned d_what;
        unsigned d_player;
        bool d_active;

        MapPoint pos;
        unsigned short width, height;

    public:
        bool terrain_rerender;
        unsigned int terrain_list;
        Point<int> terrainLastOffset;
        unsigned int terrain_last_global_animation;
        unsigned int terrain_last_water;

        std::list<MapTile> sorted_textures[16];
        std::list<BorderTile> sorted_borders[5];
        std::list<PreparedRoad> sorted_roads[4];

        GameWorldView(const MapPoint pt, unsigned short width, unsigned short height);
        ~GameWorldView();

        GameWorldViewer* GetGameWorldViewer() const {return(gwv);}
        void SetGameWorldViewer(GameWorldViewer* viewer);


        inline void SetPos(MapPoint newPos) { pos = newPos; }
        inline MapPoint GetPos() const {return pos;}

        /// Bauqualitäten anzeigen oder nicht
        inline void ShowBQ() { show_bq = !show_bq; }
        /// Gebäudenamen zeigen oder nicht
        inline void ShowNames() { show_names = !show_names; }
        /// Produktivität zeigen oder nicht
        inline void ShowProductivity() { show_productivity = !show_productivity; };
        /// Schaltet Produktivitäten/Namen komplett aus oder an
        void ShowNamesAndProductivity();

        void Draw(const unsigned char player, unsigned* water, const bool draw_selected, const MapPoint selected, const RoadsBuilding& rb);
        /*
            void PrepareRendering(const unsigned char player, const bool draw_selected, const MapCoord selected_x, const MapCoord selected_y,const RoadsBuilding& rb);
            void Render();
        */

        /// Bewegt sich zu einer bestimmten Position in Pixeln auf der Karte
        void MoveTo(int x, int y, bool absolute = false);
        /// Zentriert den Bildschirm auf ein bestimmtes Map-Object
        void MoveToMapObject(const MapPoint pt);
        /// Springt zur letzten Position, bevor man "weggesprungen" ist
        void MoveToLastPosition();

        inline void MoveToX(int x, bool absolute = false) { MoveTo( (absolute ? 0 : offset.x) + x, offset.y, true); }
        inline void MoveToY(int y, bool absolute = false) { MoveTo( offset.x, (absolute ? 0 : offset.y) + y, true); }

        void CalcFxLx();

        /// Koordinatenanzeige ein/aus
        inline void ShowCoordinates() { show_coordinates = !show_coordinates; }

        /// Gibt selektierten Punkt zurück
        inline MapCoord GetSelX() const { return selPt.x; }
        inline MapCoord GetSelY() const { return selPt.y; }
        inline MapPoint GetSel() const { return selPt; }

        inline Point<int> GetSelo() const { return selO; }

        /// Gibt Scrolling-Offset zurück
        inline int GetXOffset() const { return offset.x - pos.x; }
        inline int GetYOffset() const { return offset.y - pos.y; }
        /// Gibt ersten Punkt an, der beim Zeichnen angezeigt wird
        inline Point<int> GetFirstPt() const { return firstPt; }
        /// Gibt letzten Punkt an, der beim Zeichnen angezeigt wird
        inline Point<int> GetLastPt() const { return lastPt; }

        void DrawBoundaryStone(const int x, const int y, const MapPoint t, const int xpos, const int ypos, Visibility vis);

        void Resize(unsigned short width, unsigned short height);

        void SetAIDebug(unsigned what, unsigned player, bool active)
        {
            d_what = what; d_player = player; d_active = active;
        }
};

/// "Interface-Klasse" für GameWorldBase, die die Daten grafisch anzeigt
class GameWorldViewer : public virtual GameWorldBase
{
        /// Wird gerade gescrollt?
        bool scroll;
        int sx, sy;

        GameWorldView view;
    public:

        GameWorldViewer();

        // Wege und Grenzsteine zeichnen ( nur reingenommen, da die Position sowieso hier berechnet wird, da bietet es sich an )
        void DrawWays(const int x, const int y, const float xpos, const float ypos);

        void Draw(const unsigned char player, unsigned* water, const bool draw_selected, const MapPoint selected, const RoadsBuilding& rb)
        {
            view.Draw(player, water, draw_selected, selected, rb);
        }

        inline GameWorldView* GetView() {return(&view);}

        inline TerrainRenderer* GetTerrainRenderer() {return(&tr);}

        /// Bauqualitäten anzeigen oder nicht
        inline void ShowBQ() {view.ShowBQ();}
        /// Gebäudenamen zeigen oder nicht
        inline void ShowNames() {view.ShowNames();}
        /// Produktivität zeigen oder nicht
        inline void ShowProductivity() {view.ShowProductivity();};
        /// Schaltet Produktivitäten/Namen komplett aus oder an
        inline void ShowNamesAndProductivity() {view.ShowNamesAndProductivity();}

        /// Wegfinden ( A* ) --> Wegfindung auf allgemeinen Terrain ( ohne Straäcn ) ( fr Wegebau oder frei herumlaufende )
        bool FindRoadPath(const MapPoint start, const MapPoint dest, std::vector<unsigned char>& route, const bool boat_road);
        /// Sucht die Anzahl der verfügbaren Soldaten, um das Militärgebäude an diesem Punkt anzugreifen
        unsigned GetAvailableSoldiersForAttack(const unsigned char player_attacker, const MapPoint pt);

        /// Scrolling-Zeug
        void MouseMove(const MouseCoords& mc);
        void MouseDown(const MouseCoords& mc);
        void MouseUp();
        inline void DontScroll() { scroll = false; }

        /// Bewegt sich zu einer bestimmten Position in Pixeln auf der Karte
        inline void MoveTo(int x, int y, bool absolute = false) {view.MoveTo(x, y, absolute);};
        /// Zentriert den Bildschirm auf ein bestimmtes Map-Object
        inline void MoveToMapObject(const MapPoint pt) {view.MoveToMapObject(pt);};
        /// Springt zur letzten Position, bevor man "weggesprungen" ist
        inline void MoveToLastPosition() {view.MoveToLastPosition();};

        void MoveToX(int x, bool absolute = false) {view.MoveToX(x, absolute);}
        void MoveToY(int y, bool absolute = false) {view.MoveToY(y, absolute);}

        /// Koordinatenanzeige ein/aus
        inline void ShowCoordinates() { view.ShowCoordinates(); }

        /// Gibt selektierten Punkt zurück
        inline MapCoord GetSelX() const { return(view.GetSelX()); }
        inline MapCoord GetSelY() const { return(view.GetSelY()); }
        MapPoint GetSel() const { return view.GetSel(); }

        /// Gibt ersten Punkt an, der beim Zeichnen angezeigt wird
        inline Point<int> GetFirstPt() const { return(view.GetFirstPt()); }
        /// Gibt letzten Punkt an, der beim Zeichnen angezeigt wird
        inline Point<int> GetLastPt() const { return(view.GetLastPt()); }

        /// Ermittelt Sichtbarkeit eines Punktes für den lokalen Spieler, berücksichtigt ggf. Teamkameraden
        Visibility GetVisibility(const MapPoint pt) const;

        /// Höhe wurde verändert: TerrainRenderer Bescheid sagen, damit es entsprechend verändert werden kann
        void AltitudeChanged(const MapPoint pt);
        /// Sichtbarkeit wurde verändert: TerrainRenderer Bescheid sagen, damit es entsprechend verändert werden kann
        void VisibilityChanged(const MapPoint pt);

        /// liefert sichtbare Strasse, im Nebel entsprechend die FoW-Strasse
        unsigned char GetVisibleRoad(const MapPoint pt, unsigned char dir, const Visibility visibility) const;

        /// Get the "youngest" FOWObject of all players who share the view with the local player
        const FOWObject* GetYoungestFOWObject(const MapPoint pos) const;

        /// Gets the youngest fow node of all visible objects of all players who are connected
        /// with the local player via team view
        unsigned char GetYoungestFOWNodePlayer(const MapPoint pos) const;

        /// Schattierungen (vor allem FoW) neu berechnen
        void RecalcAllColors();

        /// Gibt das erste Schiff, was gefunden wird von diesem Spieler, zurück, ansonsten NULL, falls es nicht
        /// existiert
        noShip* GetShip(const MapPoint pt, const unsigned char player) const;

        /// Gibt die verfügbar Anzahl der Angreifer für einen Seeangriff zurück
        unsigned GetAvailableSoldiersForSeaAttackCount(const unsigned char player_attacker, const MapPoint pt) const;

        inline void Resize(unsigned short width, unsigned short height) {view.Resize(width, height);}

        inline void SetAIDebug(unsigned what, unsigned player, bool active) {view.SetAIDebug(what, player, active);}
};

/// "Interface-Klasse" für das Spiel
class GameWorldGame : public virtual GameWorldBase
{
    protected:
        /// rade graphs, one for each player
        std::vector<TradeGraph*> tgs;
    private:



        /// vergleicht 2 Punkte, ob sie von unterschiedlichen Spielern sind und setzt
        /// Punkt ggf. zu gar keinem Spieler, 2. Funktion wird für Punkte im 2er Abstand verwendet, da es dort ein bisschen anders läuft!
        void AdjustNodes(const MapCoord x1, const MapCoord y1, const MapCoord x2, const MapCoord y2);
        void AdjustNodes2(const MapCoord x1, const MapCoord y1, const MapCoord x2, const MapCoord y2);
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
        void SetVisibility(const MapPoint pt,  const unsigned char player);

        /// Prüfen, ob zu einem bestimmten Küsenpunkt ein Hafenpunkt gehört und wenn ja, wird dieser zurückgegeben
        unsigned short GetHarborPosID(const MapPoint pt);
        /// Bestimmt die Schifffahrtrichtung, in der ein Punkt relativ zu einem anderen liegt
        unsigned char GetShipDir(Point<int> pos1, Point<int> pos2);


    protected:

        /// Erzeugt FOW-Objekte, -Straßen und -Grensteine von aktuellen Punkt für einen bestimmten Spieler
        void SaveFOWNode(const MapPoint pt, const unsigned player);
        /// Berechnet für alle Hafenpunkt jeweils die Richtung und Entfernung zu allen anderen Hafenpunkten
        /// über die Kartenränder hinweg
        void CalcHarborPosNeighbors();
        /// Create Trade graphs
        void CreateTradeGraphs();

    public:

        virtual ~GameWorldGame();

        /// Set by the playerSwitch GC
        struct{
            unsigned char oldPlayer, newPlayer;
        } switchedPlayers;

        /// Stellt anderen Spielern/Spielobjekten das Game-GUI-Interface zur Verfüung
        inline GameInterface* GetGameInterface() const { return gi; }

        inline void SetNO(noBase* obj, const MapPoint pt) { GetNode(pt).obj = obj; }
        void AddFigure(noBase* fig, const MapPoint pt);
        void RemoveFigure(noBase* fig, const MapPoint pt);

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
        unsigned char FindHumanPathOnRoads(const noRoadNode* const start, const noRoadNode* const goal, unsigned* length = NULL, MapPoint * next_harbor = NULL, const RoadSegment* const forbidden = NULL);
        /// Wegfindung für Waren im Straßennetz
        unsigned char FindPathForWareOnRoads(const noRoadNode* const start, const noRoadNode* const goal, unsigned* length = NULL, MapPoint * next_harbor = NULL, unsigned max = 0xFFFFFFFF);
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
        void BuildRoad(const unsigned char playerid, const bool boat_road,
                       const MapPoint start, const std::vector<unsigned char>& route);
        /// Reißt eine Straße ab
        void DestroyRoad(const MapPoint pt, const unsigned char dir);
        /// baut eine Straße aus
        void UpgradeRoad(const MapPoint pt, const unsigned char dir);

        /// Berechnet das Land in einem bestimmten Bereich (um ein neues, abgerissenes oder eingenommenes
        /// Militärgebäude rum) neu, destroyed gibt an, ob building abgerissen wurde und somit nicht einberechnet werden soll
        void RecalcTerritory(const noBaseBuilding* const building, const unsigned short radius, const bool destroyed, const bool newBuilt);
        /// Berechnet das Land in einem bestimmten Bereich um ein aktuelles Militärgebäude rum neu und gibt zurück ob sich etwas verändern würde (auf für ki wichtigem untergrund) wenn das Gebäude zerstört werden würde
        bool TerritoryChange(const noBaseBuilding* const building, const unsigned short radius, const bool destroyed, const bool newBuilt);
        /// Greift ein Militärgebäude auf x,y an (entsendet dafür die Soldaten etc.)
        void Attack(const unsigned char player_attacker, const MapPoint pt, const unsigned short soldiers_count, const bool strong_soldiers);
        /// Greift ein Militäregebäude mit Schiffen an
        void AttackViaSea(const unsigned char player_attacker, const MapPoint pt, const unsigned short soldiers_count, const bool strong_soldiers);
        // Liefert das entsprechende Militärquadrat für einen bestimmten Punkt auf der Karte zurück (normale Koordinaten)
        std::list<nobBaseMilitary*>& GetMilitarySquare(const MapPoint pt)
        { return military_squares[(pt.y / MILITARY_SQUARE_SIZE) * (width / MILITARY_SQUARE_SIZE + 1) + pt.x / MILITARY_SQUARE_SIZE]; }

        /// Fügt einen Katapultstein der Welt hinzu, der gezeichnt werden will
        inline void AddCatapultStone(CatapultStone* cs) {catapult_stones.push_back(cs); }
        inline void RemoveCatapultStone(CatapultStone* cs) {catapult_stones.remove(cs); }

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
        /// Berechet die ganzen Sichtbarkeiten der Karte neu
        void RecalcAllVisibilities();
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
        void AddHarborBuildingSiteFromSea(noBuildingSite* building_site)
        { this->harbor_building_sites_from_sea.push_back(building_site); }
        /// Entfernt diese wieder
        void RemoveHarborBuildingSiteFromSea(noBuildingSite* building_site)
        { this->harbor_building_sites_from_sea.remove(building_site); }
        /// Gibt zurück, ob eine bestimmte Baustellen eine Baustelle ist, die vom Schiff aus errichtet wurde
        bool IsHarborBuildingSiteFromSea(const noBuildingSite* building_site) const;
        /// Liefert eine Liste der Hafenpunkte, die von einem bestimmten Hafenpunkt erreichbar sind
        void GetHarborPointsWithinReach(const unsigned hp, std::vector<unsigned>& hps) const;

        /// Creates a Trade Route from one point to another
        void CreateTradeRoute(const MapPoint start, MapPoint dest, const unsigned char player, TradeRoute** tr);
        /// Retrieves a trade graph
        TradeGraph* GetTradeGraph(const unsigned char player) const { return tgs[player]; }
};


class GameWorld : public GameWorldViewer, public GameWorldGame
{
    public:

        /// Lädt eine Karte
        bool LoadMap(const std::string& filename);

        /// Serialisiert den gesamten GameWorld
        void Serialize(SerializedGameData* sgd) const;
        void Deserialize(SerializedGameData* sgd);

        /// Sagt der GW Bescheid, dass ein Objekt von Bedeutung an x,y vernichtet wurde, damit dieser
        /// dass ggf. an den WindowManager weiterleiten kann, damit auch ein Fenster wieder geschlossen wird
        void ImportantObjectDestroyed(const MapPoint pt);
        /// Sagt, dass ein Militärgebäude eingenommen wurde und ggf. ein entsprechender "Fanfarensound" abgespielt werden sollte
        void MilitaryBuildingCaptured(const MapPoint pt, const unsigned char player);

    private:
        /// Vermisst ein neues Weltmeer von einem Punkt aus, indem es alle mit diesem Punkt verbundenen
        /// Wasserpunkte mit der gleichen sea_id belegt und die Anzahl zurückgibt
        unsigned MeasureSea(const MapPoint pt, const unsigned short sea_id);

        /// Erstellt Objekte anhand der ausgelesenen S2map
        void Scan(glArchivItem_Map* map);

        /// random function using RANDOM.Rand(...) for std::random_shuffle
        static ptrdiff_t myRandom(ptrdiff_t max);
};



#endif

