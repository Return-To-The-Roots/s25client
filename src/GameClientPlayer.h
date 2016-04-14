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

#ifndef GAMECLIENTPLAYER_H_
#define GAMECLIENTPLAYER_H_

#include "GamePlayerInfo.h"
#include "gameTypes/MapTypes.h"
#include "gameTypes/StatisticTypes.h"
#include "gameTypes/Inventory.h"
#include "GameMessage_GameCommand.h"
#include "gameTypes/SettingsTypes.h"
#include "gameTypes/BuildingTypes.h"
#include "gameTypes/PactTypes.h"
#include "gameData/MilitaryConsts.h"
#include "helpers/containerUtils.h"
#include <boost/array.hpp>
#include <list>
#include <queue>

class noFlag;
class RoadSegment;
class nobBaseWarehouse;
class noRoadNode;
class noBaseBuilding;
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
class noFigure;


/// Informationen über Gebäude-Anzahlen
struct BuildingCount
{
    boost::array<unsigned, 40> building_counts;
    boost::array<unsigned, 40> building_site_counts;
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
        ///// Liste von unbesetzten Straßen (ohne Träger) von dem Spieler
        //std::list<RoadSegment*> unoccupied_roads;
        /// Lister aller Straßen von dem Spieler
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
        std::vector< MapPoint > enemies_discovered_by_ships;

        /** Polygon(s) defining the area the player may have territory in.
         *  No elements means no restrictions.
         *  Multiple polygons may be specified, see
         *  - http://www.ecse.rpi.edu/Homepages/wrf/Research/Short_Notes/pnpoly.html
         *  The area is not allowed to shrink! This will lead to crashes!
         */
        std::vector< MapPoint > restricted_area;

        boost::array<bool, BUILDING_TYPES_COUNT> building_enabled;

        /// Liste, welchen nächsten 10 Angreifern Verteidiger entgegenlaufen sollen
        boost::array<bool, 5> defenders;
        unsigned short defenders_pos;

        /// Inventur
        Inventory global_inventory;

        /// Bündnisse mit anderen Spielern
        struct Pact
        {
            /// Dauer (in GF), 0 = kein Bündnise, 0xFFFFFFFF = Bündnis auf Ewigkeit
            unsigned duration;
            /// Startzeitpunkt (in GF)
            unsigned start;
            /// Bündnis schon akzeptiert oder nur vorgeschlagen?
            bool accepted;
            /// Will dieser Spieler (also der this-Pointer) diesen Vertrag auflösen?
            bool want_cancel;

            Pact() : duration(0), start(0), accepted(false), want_cancel(false) {}
            Pact(SerializedGameData& sgd);
            void Serialize(SerializedGameData& sgd);
        };
        /// Bündnisse dieses Spielers mit anderen Spielern
        Pact pacts[MAX_PLAYERS][PACTS_COUNT];

    public:

        /// Laggt der Spieler?
        bool is_lagging;
        /// Empfangene GC für diesen Spieler
        std::queue<GameMessage_GameCommand> gc_queue;

        /// Koordinaten des HQs des Spielers
        MapPoint hqPos;

        // Informationen über die Verteilung
        struct Distribution
        {
            unsigned char percent_buildings[BUILDING_TYPES_COUNT];
            std::list<BuildingType> client_buildings; // alle Gebäude, die diese Ware bekommen, zusammengefasst
            std::vector<unsigned char> goals;
            unsigned selected_goal;
        };
        boost::array<Distribution, WARE_TYPES_COUNT> distribution;

        /// Art der Reihenfolge (0 = nach Auftraggebung, ansonsten nach build_order)
        unsigned char orderType_;
        /// Baureihenfolge
        BuildOrders build_order;
        /// Prioritäten der Waren im Transport
        boost::array<unsigned char, WARE_TYPES_COUNT> transport;
        /// Militäreinstellungen (die vom Militärmenü)
        boost::array<unsigned char, MILITARY_SETTINGS_COUNT> militarySettings_;
        /// Werkzeugeinstellungen (in der Reihenfolge wie im Fenster!)
        ToolSettings toolsSettings_;
        // qx:tools
        boost::array<unsigned char, TOOL_COUNT> tools_ordered;
        boost::array<signed char, TOOL_COUNT> tools_ordered_delta;

        void EnableBuilding(BuildingType type) {building_enabled[type] = true;}
        void DisableBuilding(BuildingType type) {building_enabled[type] = false;}
        bool IsBuildingEnabled(BuildingType type) const {return(building_enabled[type]);}
        std::vector< MapPoint >& GetRestrictedArea() {return(restricted_area);}

    private:

        // Sucht Weg für Job zu entsprechenden noRoadNode
        bool FindWarehouseForJob(const Job job, noRoadNode* goal);
        /// Prüft, ob der Spieler besiegt wurde
        void TestDefeat();
        /// Bündnis (real, d.h. spielentscheidend) abschließen
        void MakePact(const PactType pt, const unsigned char other_player, const unsigned duration);

    public:
        GameClientPlayer(const unsigned playerid);

        /// Serialisieren
        void Serialize(SerializedGameData& sgd);
        // Deserialisieren
        void Deserialize(SerializedGameData& sgd);

        /// Setzt GameWorld
        void SetGameWorldPointer(GameWorldGame* const gwg) { this->gwg = gwg; }

        /// Looks for the closes warehouse for the point 'start' (including it) that matches the conditions by the functor
        /// - isWarehouseGood must be a functor taking a "const nobBaseWarhouse&", that returns a bool whether this warehouse should be considered
        /// - to_wh true if path to wh is searched, false for path from wh
        /// - length is optional for the path length
        /// - forbidden optional roadSegment that must not be used
        template<class T_IsWarehouseGood>
        nobBaseWarehouse* FindWarehouse(const noRoadNode& start, const T_IsWarehouseGood& isWarehouseGood, const bool to_wh, const bool use_boat_roads,
            unsigned* const length = 0, const RoadSegment* const forbidden = NULL, bool record = true) const;
        /// Gibt dem Spieler bekannt, das eine neue Straße gebaut wurde
        void NewRoad(RoadSegment* const rs);
        /// Neue Straße hinzufügen
        void AddRoad(RoadSegment* const rs) { roads.push_back(rs); }
        /// Gibt dem Spieler brekannt, das eine Straße abgerissen wurde
        void RoadDestroyed();
        /// Sucht einen Träger für die Straße und ruft ggf den Träger aus dem jeweiligen nächsten Lagerhaus
        bool FindCarrierForRoad(RoadSegment* rs);
        /// Warenhaus zur Warenhausliste hinzufügen
        void AddWarehouse(nobBaseWarehouse* wh) { warehouses.push_back(wh); }
        /// Warenhaus aus Warenhausliste entfernen
        void RemoveWarehouse(nobBaseWarehouse* wh) { RTTR_Assert(helpers::contains(warehouses, wh)); warehouses.remove(wh); TestDefeat(); }
        /// Returns true if the given wh does still exist and hence the ptr is valid
        bool IsWarehouseValid(nobBaseWarehouse* wh) const{ return helpers::contains(warehouses, wh); }
        /// Hafen zur Warenhausliste hinzufügen
        void AddHarbor(nobHarborBuilding* hb);
        /// Hafen aus Warenhausliste entfernen
        void RemoveHarbor(nobHarborBuilding* hb) { RTTR_Assert(helpers::contains(harbors, hb)); harbors.remove(hb); }
        /// (Unbesetzte) Straße aus der Liste entfernen
        void DeleteRoad(RoadSegment* rs) { RTTR_Assert(helpers::contains(roads, rs)); roads.remove(rs); }

        /// Für alle unbesetzen Straßen Weg neu berechnen
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
        /// Versucht für einen Esel eine Straße zu finden, in goal wird die Zielflagge zurückgegeben,
        /// sofern eine Straße gefunden wurde, ansonsten ist das ein Lagerhaus oder 0, falls auch das nich gefunden wurde
        RoadSegment* FindRoadForDonkey(noRoadNode* start, noRoadNode** goal);


        /// Sucht für eine (neuproduzierte) Ware einen Abnehmer (wenns keinen gibt, wird ein Lagerhaus gesucht, wenn
        /// es auch dorthin keinen Weg gibt, wird 0 zurückgegeben
        noBaseBuilding* FindClientForWare(Ware* ware);
        nobBaseWarehouse* FindWarehouseForWare(const Ware& ware) const;

        /// Sucht einen Abnehmer (sprich Militärgebäude), wenn es keinen findet, wird ein Warenhaus zurückgegeben bzw. 0
        nobBaseMilitary* FindClientForCoin(Ware* ware) const;

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
        /// je kleiner die Rückgabe, destro größer die Priorität!
        unsigned GetBuidingSitePriority(const noBuildingSite* building_site);

        /// Berechnet die Verteilung der Waren auf die einzelnen Gebäude neu
        void RecalcDistribution();
        /// Berechnet die Verteilung einer (bestimmten) Ware
        void RecalcDistributionOfWare(const GoodType ware);
        /// Konvertiert die Daten vom wp_transport in "unser" Prioritäten-Format und setzt es
        void ConvertTransportData(const TransportOrders& transport_data);

        /// Ware zur globalen Warenliste hinzufügen und entfernen
        void RegisterWare(Ware* ware) { ware_list.push_back(ware); }
        void RemoveWare(Ware* ware) { RTTR_Assert(IsWareRegistred(ware)); ware_list.remove(ware); }
        bool IsWareRegistred(Ware* ware) { return (helpers::contains(ware_list, ware)); }
        bool IsWareDependent(Ware* ware);

        /// Fügt Waren zur Inventur hinzu
        void IncreaseInventoryWare(const GoodType ware, const unsigned count);
        void DecreaseInventoryWare(const GoodType ware, const unsigned count);
        void IncreaseInventoryJob(const Job job, const unsigned count) { global_inventory.Add(job, count); }
        void DecreaseInventoryJob(const Job job, const unsigned count) { global_inventory.Remove(job, count); }

        /// Gibt Inventory-Settings zurück
        const Inventory& GetInventory() const { return global_inventory; }

        /// Setzt neue Militäreinstellungen
        void ChangeMilitarySettings(const boost::array<unsigned char, MILITARY_SETTINGS_COUNT>& military_settings);
        /// Setzt neue Werkzeugeinstellungen
        void ChangeToolsSettings(const ToolSettings& tools_settings);
        /// Setzt neue Verteilungseinstellungen
        void ChangeDistribution(const Distributions& distribution_settings);
        /// Setzt neue Baureihenfolge-Einstellungen
        void ChangeBuildOrder(const unsigned char order_type, const BuildOrders& oder_data);

        /// Darf der andere Spieler von mir angegriffen werden?
        bool IsPlayerAttackable(const unsigned char player) const;
        /// Ist ein anderer Spieler ein richtiger Verbündeter von uns, d.h. Teamsicht, Unterstützung durch aggressive Verteidiger usw.?
        bool IsAlly(const unsigned char player) const;
        /// Truppen bestellen
        void OrderTroops(nobMilitary* goal, unsigned count, bool ignoresettingsendweakfirst=false);
        /// Prüft die Besatzung von allen Militärgebäuden und reguliert entsprechend (bei Veränderung der Militäreinstellungen)
        void RegulateAllTroops();
        /// Prüft von allen Militärgebäuden die Fahnen neu
        void RecalcMilitaryFlags();
        /// Sucht für Soldaten ein neues Militärgebäude, als Argument wird Referenz auf die
        /// entsprechende Soldatenanzahl im Lagerhaus verlangt
        void NewSoldiersAvailable(const unsigned& soldier_count);
        /// Aktualisiert die Verteidiger-Liste
        void RefreshDefenderList();
        /// Prüft, ob für einen angreifenden Soldaten ein Verteidger geschickt werden soll
        bool ShouldSendDefender();

        /// Ruft einen Geologen
        void CallFlagWorker(const MapPoint pt, const Job job);
        /// Registriert einen Geologen bzw. einen Späher an einer bestimmten Flagge, damit diese informiert werden,
        /// wenn die Flagge abgerissen wird
        void RegisterFlagWorker(nofFlagWorker* flagworker) { flagworkers.push_back(flagworker); }
        void RemoveFlagWorker(nofFlagWorker* flagworker) { RTTR_Assert(IsFlagWorker(flagworker)); flagworkers.remove(flagworker); }
        bool IsFlagWorker(nofFlagWorker* flagworker) { return helpers::contains(flagworkers, flagworker); }

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
        bool FindHarborForUnloading(noShip* ship, const MapPoint start, unsigned* goal_harbor_id, std::vector<unsigned char> * route,
                                    nobHarborBuilding* exception);
        /// A ship has discovered new hostile territory --> determines if this is new
        /// i.e. there is a sufficient distance to older locations
        /// Returns true if yes and false if not
        bool ShipDiscoveredHostileTerritory(const MapPoint location);

        ///Gibt liste der Schiffe zurück
        const std::vector<noShip*>&GetShips() const {return ships;}

        /// Gibt eine Liste der verfügbaren Häfen zurück
        const std::list<nobHarborBuilding*>& GetHarbors() const { return harbors; }

        /// Er gibt auf
        void Surrender();

		///all allied players get a letter with the location
		void NotifyAlliesOfLocation(const MapPoint pt);

        /// This player suggests a pact to target player
        void SuggestPact(const unsigned char targetPlayer, const PactType pt, const unsigned duration);
        /// Accepts a pact, that this player suggested target player
        void AcceptPact(const unsigned id, const PactType pt, const unsigned char targetPlayer);
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

        /// Returns all warehouses that can trade with the given goal
        /// IMPORTANT: Warehouses can be destroyed. So check them first before using!
        std::vector<nobBaseWarehouse*> GetWarehousesForTrading(nobBaseWarehouse& goalWh) const;
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
        const unsigned int GetStatisticCurrentValue(unsigned int idx)  { RTTR_Assert(idx < STAT_TYPE_COUNT); return(statisticCurrentData[idx]);}

        // Testet ob Notfallprogramm aktiviert werden muss und tut dies dann
        void TestForEmergencyProgramm();

    private:
        // Statistikdaten
        boost::array<Statistic, STAT_TIME_COUNT> statistic;

        // Die Statistikwerte die 'aktuell' gemessen werden
        int statisticCurrentData[STAT_TYPE_COUNT];
        int statisticCurrentMerchandiseData[STAT_MERCHANDISE_TYPE_COUNT];

        unsigned short incrStatIndex(unsigned short i) { return (i == STAT_STEP_COUNT - 1) ? 0 : ++i; }
        unsigned short decrStatIndex(unsigned short i) { return (i == 0) ? STAT_STEP_COUNT - 1 : --i; }
        unsigned short decrStatIndex(unsigned short i, unsigned short amount) { return (i < amount) ? STAT_STEP_COUNT - (amount - i) - 1 : i - amount; }

        // Notfall-Programm aktiviert ja/nein (Es gehen nur noch Res an Holzfäller- und Sägewerk-Baustellen raus)
        bool emergency;

        /// Called after a pact was changed(added/removed) in both players
        void PactChanged(const PactType pt);

    public:
        bool hasEmergency() const { return emergency; }

        /// Testet ob der Spieler noch mehr Katapulte bauen darf
        bool CanBuildCatapult() const;

        /// For debug only
        bool IsDependentFigure(noFigure* fig);
};

#endif


