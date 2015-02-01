// $Id: GameClientPlayer.h 9597 2015-02-01 09:42:22Z marcus $
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

#ifndef GAMECLIENTPLAYER_H_
#define GAMECLIENTPLAYER_H_

#include "GamePlayerInfo.h"
#include "GameConsts.h"
#include <list>
#include <algorithm>
#include "MapConsts.h"
#include "PostMsg.h"
#include "TradeGraph.h"


class GameWorld;
class noFlag;
class RoadSegment;
class nobBaseWarehouse;
class noRoadNode;
class noBaseBuilding;
class noBuilding;
class noBuildingSite;
class Ware;
class nobMilitary;
class nobUsual;
class nofFlagWorker;
class nobBaseMilitary;
class SerializedGameData;
class nofCarrier;
class noShip;
class nobHarborBuilding;
class GameWorldGame;
class GameMessage_GameCommand;
class nofPassiveSoldier;
class noFigure;


/// Informationen über Gebäude-Anzahlen
struct BuildingCount
{
    unsigned building_counts[40];
    unsigned building_site_counts[40];
};


class GameClientPlayer : public GamePlayerInfo
{
    private:

        // Zugriff der Spieler auf die Spielwelt
        GameWorldGame* gwg;
        /// Liste der Warenhäuser des Spielers
        std::list<nobBaseWarehouse*> warehouses;
        /// Liste von Häfen
        std::list<nobHarborBuilding*> harbors;
        ///// Liste von unbesetzten StraÃen (ohne Träger) von dem Spieler
        //std::list<RoadSegment*> unoccupied_roads;
        /// Lister aller StraÃen von dem Spieler
        std::list<RoadSegment*> roads;

        struct JobNeeded
        {
            Job job;
            noRoadNode* workplace;
        };

        struct BuildingWhichWantWare
        {
            unsigned char count;
            unsigned char building;
        };

        /// Liste von Baustellen/Gebäuden, die bestimmten Beruf wollen
        std::list<JobNeeded> jobs_wanted;

        /// Listen der einzelnen Gebäudetypen (nur nobUsuals!)
        std::list<nobUsual*> buildings[30];
        /// Liste von sämtlichen Baustellen
        std::list<noBuildingSite*> building_sites;
        /// Liste von allen Militärgebäuden
        std::list<nobMilitary*> military_buildings;
        /// Liste von sämtlichen Waren, die herumgetragen werden und an Fahnen liegen
        std::list<Ware*> ware_list;
        /// Liste von Geologen und Spähern, die an eine Flagge gebunden sind
        std::list<nofFlagWorker*> flagworkers;
        /// Liste von Schiffen dieses Spielers
        std::vector<noShip*> ships;

        /// Liste mit Punkten, die schon von Schiffen entdeckt wurden
        std::vector< Point<MapCoord> > enemies_discovered_by_ships;

        /** Polygon(s) defining the area the player may have territory in.
         *  No elements means no restrictions.
         *  Multiple polygons may be specified, see
         *  - http://www.ecse.rpi.edu/Homepages/wrf/Research/Short_Notes/pnpoly.html
         *  The area is not allowed to shrink! This will lead to crashes!
         */
        std::vector< Point<MapCoord> > restricted_area;

        bool building_enabled[BUILDING_TYPES_COUNT];

        /// Liste, welchen nächsten 10 Angreifern Verteidiger entgegenlaufen sollen
        bool defenders[5];
        unsigned short defenders_pos;

        /// Inventur
        Goods global_inventory;

        /// Bündnisse mit anderen Spielern
        struct Pact
        {
            /// Bündnis schon akzeptiert oder nur vorgeschlagen?
            bool accepted;
            /// Dauer (in GF), 0 = kein Bündnise, 0xFFFFFFFF = Bündnis auf Ewigkeit
            unsigned duration;
            /// Startzeitpunkt (in GF)
            unsigned start;
            /// Will dieser Spieler (also der this-Pointer) diesen Vertrag auflösen?
            bool want_cancel;

            Pact() : accepted(false), duration(0), start(0), want_cancel(false) {}
            Pact(SerializedGameData* ser);
            void Serialize(SerializedGameData* ser);
        };
        /// Bündnisse dieses Spielers mit anderen Spielern
        Pact pacts[MAX_PLAYERS][PACTS_COUNT];

    public:

        /// Laggt der Spieler?
        bool is_lagging;
        /// Empfangene GC für diesen Spieler
        std::list<GameMessage_GameCommand> gc_queue;

        /// Koordinaten des HQs des Spielers
        unsigned short hqx, hqy;

        // Informationen über die Verteilung
        struct
        {
            unsigned char percent_buildings[BUILDING_TYPES_COUNT];
            std::list<BuildingType> client_buildings; // alle Gebäude, die diese Ware bekommen, zusammengefasst
            std::vector<unsigned char> goals;
            unsigned selected_goal;
        } distribution[WARE_TYPES_COUNT];

        /// Art der Reihenfolge (0 = nach Auftraggebung, ansonsten nach build_order)
        unsigned char order_type;
        /// Baureihenfolge
        std::vector <unsigned char> build_order;
        /// Prioritäten der Waren im Transport
        unsigned char transport[WARE_TYPES_COUNT];
        /// Militäreinstellungen (die vom Militärmenü)
        std::vector <unsigned char> military_settings;
        /// Werkzeugeinstellungen (in der Reihenfolge wie im Fenster!)
        std::vector <unsigned char> tools_settings;
        // qx:tools
        unsigned char tools_ordered[TOOL_COUNT];
        signed char tools_ordered_delta[TOOL_COUNT];

        void EnableBuilding(BuildingType type) {building_enabled[type] = true;}
        void DisableBuilding(BuildingType type) {building_enabled[type] = false;}
        bool IsBuildingEnabled(BuildingType type) {return(building_enabled[type]);}
        std::vector< Point<MapCoord> > &GetRestrictedArea() {return(restricted_area);}

    private:

        // Sucht Weg für Job zu entsprechenden noRoadNode
        bool FindWarehouseForJob(const Job job, noRoadNode* goal);
        /// Prüft, ob der Spieler besiegt wurde
        void TestDefeat();
        /// Bündnis (real, d.h. spielentscheidend) abschlieÃen
        void MakePact(const PactType pt, const unsigned char other_player, const unsigned duration);

    public:
        /// Konstruktor von @p GameClientPlayer.
        GameClientPlayer(const unsigned playerid);

        /// Serialisieren
        void Serialize(SerializedGameData* sgd);
        // Deserialisieren
        void Deserialize(SerializedGameData* sgd);

        /// Tauscht Spieler
        void SwapPlayer(GameClientPlayer& two);

        /// Setzt GameWorld
        void SetGameWorldPointer(GameWorldGame* const gwg) { this->gwg = gwg; }

        /*/// liefert das aktuelle (komplette) inventar.
        void GetInventory(unsigned int *wares, unsigned int *figures);*/

        /// Sucht ein nächstgelegenes Warenhaus für den Punkt 'start', das die Bedingung der Ãbergebenen Funktion
        /// IsWarehouseGood erfüllt, als letzen Parameter erhält jene Funktion param
        /// - forbidden ist ein optionales StraÃenstück, das nicht betreten werden darf,
        /// - to_wh muss auf true gesetzt werden, wenn es zum Lagerhaus geht, ansonsten auf false, in length wird die Wegeslänge zurückgegeben
        nobBaseWarehouse* FindWarehouse(const noRoadNode* const start, bool (*IsWarehouseGood)(nobBaseWarehouse*, const void*), const RoadSegment* const forbidden, const bool to_wh, const void* param, const bool use_boat_roads, unsigned* const length = 0);
        /// Gibt dem Spieler bekannt, das eine neue StraÃe gebaut wurde
        void NewRoad(RoadSegment* const rs);
        /// Neue StraÃe hinzufügen
        void AddRoad(RoadSegment* const rs) { roads.push_back(rs); }
        /// Gibt dem Spieler brekannt, das eine StraÃe abgerissen wurde
        void RoadDestroyed();
        /// Sucht einen Träger für die StraÃe und ruft ggf den Träger aus dem jeweiligen nächsten Lagerhaus
        bool FindCarrierForRoad(RoadSegment* rs);
        /// Warenhaus zur Warenhausliste hinzufügen
        void AddWarehouse(nobBaseWarehouse* wh) { warehouses.push_back(wh); }
        /// Warenhaus aus Warenhausliste entfernen
        void RemoveWarehouse(nobBaseWarehouse* wh) { warehouses.remove(wh); TestDefeat(); }
        /// Hafen zur Warenhausliste hinzufügen
        void AddHarbor(nobHarborBuilding* hb);
        /// Hafen aus Warenhausliste entfernen
        void RemoveHarbor(nobHarborBuilding* hb) { harbors.remove(hb); }
        /// (Unbesetzte) StraÃe aus der Liste entfernen
        void DeleteRoad(RoadSegment* rs) { roads.remove(rs); }
        //bool TestRoads(RoadSegment * rs) { return roads.search(rs).valid(); }

        /// Für alle unbesetzen StraÃen Weg neu berechnen
        void FindWarehouseForAllRoads();
        /// Lässt alle Baustellen ggf. noch vorhandenes Baumaterial bestellen
        void FindMaterialForBuildingSites();
        /// Fügt ein RoadNode hinzu, der einen bestimmten Job braucht
        void AddJobWanted(const Job job, noRoadNode* workplace);
        /// Entfernt ihn wieder aus der Liste (wenn er dann doch nich mehr gebraucht wird)
        void JobNotWanted(noRoadNode* workplace,bool all=false);
        /// Entfernt einen ausgesuchten Job wieder aus der Liste (wenn er dann doch nich mehr gebraucht wird)
        void OneJobNotWanted(const Job job, noRoadNode* workplace);
        /// Versucht für alle Arbeitsplätze eine Arbeitskraft zu suchen
        void FindWarehouseForAllJobs(const Job job);
        /// Versucht für alle verlorenen Waren ohne Ziel Lagerhaus zu finden
        void FindClientForLostWares();
        /// Bestellt eine Ware und gibt sie zurück, falls es eine gibt, ansonsten 0
        Ware* OrderWare(const GoodType ware, noBaseBuilding* goal);
        /// Versucht einen Esel zu bestellen, gibt 0 zurück, falls keinen gefunden
        nofCarrier* OrderDonkey(RoadSegment* road);
        /// Versucht für einen Esel eine StraÃe zu finden, in goal wird die Zielflagge zurückgegeben,
        /// sofern eine StraÃe gefunden wurde, ansonsten ist das ein Lagerhaus oder 0, falls auch das nich gefunden wurde
        RoadSegment* FindRoadForDonkey(noRoadNode* start, noRoadNode** goal);


        /// Sucht für eine (neuproduzierte) Ware einen Abnehmer (wenns keinen gibt, wird ein Lagerhaus gesucht, wenn
        /// es auch dorthin keinen Weg gibt, wird 0 zurückgegeben
        noBaseBuilding* FindClientForWare(Ware* ware);
        /// Sucht einen Abnehmer (sprich Militärgebäude), wenn es keinen findet, wird ein Warenhaus zurückgegeben bzw. 0
        nobBaseMilitary* FindClientForCoin(Ware* ware);

        /// Speichert Baustellen Gebäude etc, erklärt sich von selbst
        void AddBuildingSite(noBuildingSite* building_site);
        void RemoveBuildingSite(noBuildingSite* building_site);
        /// Speichert normale Gebäude
        void AddUsualBuilding(nobUsual* building);
        void RemoveUsualBuilding(nobUsual* building);
        /// Speichert Militärgebäude
        void AddMilitaryBuilding(nobMilitary* building);
        void RemoveMilitaryBuilding(nobMilitary* building);

        const std::list<noBuildingSite*>& GetBuildingSites() const { return building_sites; }

        const std::list<nobMilitary*>& GetMilitaryBuildings() const { return military_buildings; }

        const std::list<nobBaseWarehouse*>&GetStorehouses()const {return warehouses;}

        /// Gibt Liste von Gebäuden des Spieler zurück
        const std::list<nobUsual*>& GetBuildings(const BuildingType type) const;
        /// Liefert die Anzahl aller Gebäude einzeln
        void GetBuildingCount(BuildingCount& bc) const;
        /// Berechnet die durschnittlichen Produktivität eines jeden Gebäudetyps
        /// (erwartet als Argument ein 40-er Array!)
        void CalcProductivities(std::vector<unsigned short>& productivities);

        /// Berechnet die durschnittlichen Produktivität aller Gebäude
        unsigned short CalcAverageProductivitiy();


        /// Gibt Priorität der Baustelle zurück (entscheidet selbständig, welche Reihenfolge usw)
        /// je kleiner die Rückgabe, destro gröÃer die Priorität!
        unsigned GetBuidingSitePriority(const noBuildingSite* building_site);

        /// Berechnet die Verteilung der Waren auf die einzelnen Gebäude neu
        void RecalcDistribution();
        /// Berechnet die Verteilung einer (bestimmten) Ware
        void RecalcDistributionOfWare(const GoodType ware);
        /// Konvertiert die Daten vom wp_transport in "unser" Prioritäten-Format und setzt es
        void ConvertTransportData(const std::vector<unsigned char>& transport_data);

        /// Ware zur globalen Warenliste hinzufügen und entfernen
        void RegisterWare(Ware* ware) { ware_list.push_back(ware); }
        void RemoveWare(Ware* ware) { ware_list.remove(ware); }
        bool IsWareRegistred(Ware* ware) { return (std::find(ware_list.begin(), ware_list.end(), ware) != ware_list.end()); }
        bool IsWareDependent(Ware* ware);

        /// Fügt Waren zur Inventur hinzu
        void IncreaseInventoryWare(const GoodType ware, const unsigned count) { global_inventory.goods[ConvertShields(ware)] += count; }
        void DecreaseInventoryWare(const GoodType ware, const unsigned count) { assert(global_inventory.goods[ConvertShields(ware)] >= count); global_inventory.goods[ConvertShields(ware)] -= count; }
        void IncreaseInventoryJob(const Job job, const unsigned count) { global_inventory.people[job] += count; }
        void DecreaseInventoryJob(const Job job, const unsigned count) { assert(global_inventory.people[job] >= count); global_inventory.people[job] -= count; }

        /// Gibt Inventory-Settings zurück
        const Goods* GetInventory() const { return &global_inventory; }

        /// Setzt neue Militäreinstellungen
        void ChangeMilitarySettings(const std::vector<unsigned char>& military_settings);
        /// Setzt neue Werkzeugeinstellungen
        void ChangeToolsSettings(const std::vector<unsigned char>& tools_settings);
        /// Setzt neue Verteilungseinstellungen
        void ChangeDistribution(const std::vector<unsigned char>& distribution_settings);
        /// Setzt neue Baureihenfolge-Einstellungen
        void ChangeBuildOrder(const unsigned char order_type, const std::vector<unsigned char>& oder_data);

        /// Darf der andere Spieler von mir angegriffen werden?
        bool IsPlayerAttackable(const unsigned char player) const;
		/// Am I allowed to construct this building?
		bool IsBuildingEnabled(BuildingType type) const {return(building_enabled[type]);}
        /// Ist ein anderer Spieler ein richtiger Verbündeter von uns, d.h. Teamsicht, Unterstützung durch aggressive Verteidiger usw.?
        bool IsAlly(const unsigned char player) const;
        /// Truppen bestellen
        void OrderTroops(nobMilitary* goal, unsigned count, bool ignoresettingsendweakfirst=false);
        /// Prüft die Besatzung von allen Militärgebäuden und reguliert entsprechend (bei Veränderung der Militäreinstellungen)
        void RegulateAllTroops();
        /// Prüft von allen Militärgebäuden die Fahnen neu
        void RecalcMilitaryFlags();
        /// Sucht für EINEN Soldaten ein neues Militärgebäude, als Argument wird Referenz auf die
        /// entsprechende Soldatenanzahl im Lagerhaus verlangt
        void NewSoldierAvailable(const unsigned& soldier_count);
        /// Aktualisiert die Verteidiger-Liste
        void RefreshDefenderList();
        /// Prüft, ob für einen angreifenden Soldaten ein Verteidger geschickt werden soll
        bool ShouldSendDefender();

        /// Ruft einen Geologen
        void CallFlagWorker(const unsigned short x, const unsigned short y, const Job job);
        /// Registriert einen Geologen bzw. einen Späher an einer bestimmten Flagge, damit diese informiert werden,
        /// wenn die Flagge abgerissen wird
        void RegisterFlagWorker(nofFlagWorker* flagworker) { flagworkers.push_back(flagworker); }
        void RemoveFlagWorker(nofFlagWorker* flagworker) { flagworkers.remove(flagworker); }
        /// Wird aufgerufen, wenn eine Flagge abgerissen wurde, damit das den Flaggen-Arbeitern gesagt werden kann
        void FlagDestroyed(noFlag* flag);

        /// Gibt erstes Lagerhaus zurück
        nobBaseWarehouse* GetFirstWH() { return *warehouses.begin(); }

        /// Registriert ein Schiff beim Einwohnermeldeamt
        void RegisterShip(noShip* ship);
        /// Meldet das Schiff wieder ab
        void RemoveShip(noShip* ship);
        /// Versucht, für ein untätiges Schiff eine Arbeit zu suchen
        void GetJobForShip(noShip* ship);
        /// Schiff für Hafen bestellen. Wenn ein Schiff kommt, true.
        bool OrderShip(nobHarborBuilding* hb);
        /// Gibt die ID eines Schiffes zurück
        unsigned GetShipID(const noShip* const ship) const;
        /// Gibt ein Schiff anhand der ID zurück bzw. NULL, wenn keines mit der ID existiert
        noShip* GetShipByID(const unsigned ship_id) const;
        /// Gibt die Gesamtanzahl von Schiffen zurück
        unsigned GetShipCount() const { return ships.size(); }
        /// Gibt eine Liste mit allen Häfen dieses Spieler zurück, die an ein bestimmtes Meer angrenzen
        void GetHarborBuildings(std::vector<nobHarborBuilding*>& harbor_buildings, const unsigned short sea_id) const;
        /// Gibt die Anzahl der Schiffe, die einen bestimmten Hafen ansteuern, zurück
        unsigned GetShipsToHarbor(nobHarborBuilding* hb) const;
        /// Gibt der Wirtschaft Bescheid, dass ein Hafen zerstört wurde
        void HarborDestroyed(nobHarborBuilding* hb);
        /// Sucht einen Hafen in der Nähe, wo dieses Schiff seine Waren abladen kann
        /// gibt true zurück, falls erfolgreich
        bool FindHarborForUnloading(noShip* ship, const MapCoord start_x, const MapCoord start_y, unsigned* goal_harbor_id, std::vector<unsigned char> * route,
                                    nobHarborBuilding* exception);
        /// A ship has discovered new hostile territory --> determines if this is new
        /// i.e. there is a sufficient distance to older locations
        /// Returns true if yes and false if not
        bool ShipDiscoveredHostileTerritory(const Point<MapCoord> location);

        ///Gibt liste der Schiffe zurück
        const std::vector<noShip*>&GetShips() const {return ships;}

        /// Gibt eine Liste der verfügbaren Häfen zurück
        const std::list<nobHarborBuilding*>& GetHarbors() const { return harbors; }

        /// Er gibt auf
        void Surrender();

		///all allied players get a letter with the location
		void NotifyAlliesOfLocation(MapCoord x, MapCoord y, unsigned char allyplayerid);

        /// Macht Bündnisvorschlag an diesen Spieler
        void SuggestPact(const unsigned char other_player, const PactType pt, const unsigned duration);
        /// Akzeptiert ein bestimmtes Bündnis, welches an diesen Spieler gemacht wurde
        void AcceptPact(const unsigned id, const PactType pt, const unsigned char other_player);
        /// Gibt Einverständnis, dass dieser Spieler den Pakt auflösen will
        /// Falls dieser Spieler einen Bündnisvorschlag gemacht hat, wird dieser dagegen zurückgenommen
        void CancelPact(const PactType pt, const unsigned char other_player);
        /// Zeigt an, ob ein Pakt besteht
        enum PactState
        {
            NO_PACT = 0, /// Kein Pakt geschlossen
            IN_PROGRESS, /// Pakt angeboten, aber noch nicht akzeptiert
            ACCEPTED /// Bündnis in Kraft
        };
        PactState GetPactState(const PactType pt, const unsigned char other_player) const;
        /// Gibt die verbleibende Dauer zurück, die ein Bündnis noch laufen wird (0xFFFFFFFF = für immer)
        unsigned GetRemainingPactTime(const PactType pt, const unsigned char other_player) const;
        /// Setzt die initialen Bündnisse ahand der Teams
        void MakeStartPacts();
        /// returns fixed team number for randomteam players
        Team GetFixedTeam(Team rawteam);
        /// Testet die Bündnisse, ob sie nicht schon abgelaufen sind
        void TestPacts();

        /// Get available wares/figures which can THIS player (usually ally of wh->player) send to warehouse wh
        unsigned GetAvailableWaresForTrading(nobBaseWarehouse* wh, const GoodType gt, const Job job) const;
        /// Send wares to warehouse wh
        void Trade(nobBaseWarehouse* wh, const GoodType gt, const Job job, unsigned count) const;

        // Statistik-Sachen

        void SetStatisticValue(StatisticType type, unsigned int value);
        void ChangeStatisticValue(StatisticType type, int change);

        void IncreaseMerchandiseStatistic(GoodType type);

        /// Calculates current statistics
        void CalcStatistics();


        void StatisticStep();

        struct Statistic
        {
            // 30 Datensätze pro Typ
            unsigned int data[STAT_TYPE_COUNT][STAT_STEP_COUNT];
            // und das gleiche für die Warenstatistik
            unsigned short merchandiseData[STAT_MERCHANDISE_TYPE_COUNT][STAT_STEP_COUNT];
            // Index, der gerade 'vorne' (rechts im Statistikfenster) ist
            unsigned short currentIndex;
            // Counter, bei jedem vierten Update jeweils Daten zu den längerfristigen Statistiken kopieren
            unsigned short counter;
        };

        const Statistic& GetStatistic(StatisticTime time) { return statistic[time]; };
        const unsigned int GetStatisticCurrentValue(unsigned int idx)  { assert(idx < STAT_TYPE_COUNT); return(statisticCurrentData[idx]);}

        // Testet ob Notfallprogramm aktiviert werden muss und tut dies dann
        void TestForEmergencyProgramm();

    private:
        // Statistikdaten
        Statistic statistic[STAT_TIME_COUNT];

        // Die Statistikwerte die 'aktuell' gemessen werden
        unsigned int statisticCurrentData[STAT_TYPE_COUNT];
        unsigned short statisticCurrentMerchandiseData[STAT_MERCHANDISE_TYPE_COUNT];

        unsigned short incrStatIndex(unsigned short i) { return (i == STAT_STEP_COUNT - 1) ? 0 : ++i; }
        unsigned short decrStatIndex(unsigned short i) { return (i == 0) ? STAT_STEP_COUNT - 1 : --i; }
        unsigned short decrStatIndex(unsigned short i, unsigned short amount) { return (i < amount) ? STAT_STEP_COUNT - (amount - i) - 1 : i - amount; }

        // Notfall-Programm aktiviert ja/nein (Es gehen nur noch Res an Holzfäller- und Sägewerk-Baustellen raus)
        bool emergency;

    public:
        bool hasEmergency() const { return emergency; }

        /// Testet ob der Spieler noch mehr Katapulte bauen darf
        bool CanBuildCatapult() const;

        /// For debug only
        bool CheckDependentFigure(noFigure* fig);

};

#endif


