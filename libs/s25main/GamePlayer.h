// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "BuildingRegister.h"
#include "GamePlayerInfo.h"
#include "helpers/EnumArray.h"
#include "helpers/MultiArray.h"
#include "variant.h"
#include "gameTypes/BuildingType.h"
#include "gameTypes/Inventory.h"
#include "gameTypes/MapCoordinates.h"
#include "gameTypes/PactTypes.h"
#include "gameTypes/SettingsTypes.h"
#include "gameTypes/StatisticTypes.h"
#include "gameData/MaxPlayers.h"
#include <array>
#include <list>
#include <memory>

enum class Direction : uint8_t;
class GameWorld;
class noBaseBuilding;
class noBuildingSite;
class noFigure;
class noFlag;
class noRoadNode;
class noShip;
class nobBaseMilitary;
class nobBaseWarehouse;
class nobHarborBuilding;
class nobHQ;
class nobMilitary;
class nofCarrier;
class nofFlagWorker;
class PostMsg;
class RoadSegment;
class SerializedGameData;
struct VisualSettings;
class Ware;

/// Zeigt an, ob ein Pakt besteht
enum class PactState
{
    None,       /// Kein Pakt geschlossen
    InProgress, /// Pakt angeboten, aber noch nicht akzeptiert
    Accepted    /// Bündnis in Kraft
};
constexpr auto maxEnumValue(PactState)
{
    return PactState::Accepted;
}

/// Player in the game (belongs to world)
class GamePlayer : public GamePlayerInfo
{
public:
    struct Statistic
    {
        helpers::EnumArray<std::array<uint32_t, NUM_STAT_STEPS>, StatisticType> data;
        helpers::MultiArray<uint16_t, NUM_STAT_MERCHANDISE_TYPES, NUM_STAT_STEPS> merchandiseData;
        // Index, der gerade 'vorne' (rechts im Statistikfenster) ist
        uint16_t currentIndex;
        // Counter, bei jedem vierten Update jeweils Daten zu den längerfristigen Statistiken kopieren
        uint16_t counter;
    };

    // Informationen über die Verteilung
    struct Distribution
    {
        /// Mapping of Building to percentage of ware the building gets
        helpers::EnumArray<uint8_t, BuildingType> percent_buildings;
        /// Buildings that get this ware
        std::vector<BuildingType> client_buildings;
        /// Possible preferred buildings (each building is n times in here with n=percentage)
        std::vector<BuildingType> goals;
        /// Index into goals: Preferred building
        unsigned selected_goal;
    };

    GamePlayer(unsigned playerId, const PlayerInfo& playerInfo, GameWorld& world);
    ~GamePlayer();

    /// Serialisieren
    void Serialize(SerializedGameData& sgd) const;
    // Deserialisieren
    void Deserialize(SerializedGameData& sgd);

    GameWorld& GetGameWorld() { return world; }
    const GameWorld& GetGameWorld() const { return world; }

    const MapPoint& GetHQPos() const { return hqPos; }
    bool IsHQTent() const;
    void SetHQIsTent(bool isTent);

    void AddBuilding(noBuilding* bld, BuildingType bldType);
    void RemoveBuilding(noBuilding* bld, BuildingType bldType);
    void AddBuildingSite(noBuildingSite* bldSite);
    void RemoveBuildingSite(noBuildingSite* bldSite);
    const BuildingRegister& GetBuildingRegister() const { return buildings; }

    /// Notify that a new road connection exists (not only an existing road splitted)
    void NewRoadConnection(RoadSegment* rs);
    /// Neue Straße hinzufügen
    void AddRoad(RoadSegment* rs);
    /// Gibt dem Spieler bekannt, das eine Straße abgerissen wurde
    void RoadDestroyed();
    /// (Unbesetzte) Straße aus der Liste entfernen
    void DeleteRoad(RoadSegment* rs);
    /// Sucht einen Träger für die Straße und ruft ggf den Träger aus dem jeweiligen nächsten Lagerhaus
    bool FindCarrierForRoad(RoadSegment& rs) const;
    /// Returns true if the given wh does still exist and hence the ptr is valid
    bool IsWarehouseValid(nobBaseWarehouse* wh) const;
    /// Gibt erstes Lagerhaus zurück
    nobBaseWarehouse* GetFirstWH() const
    {
        return buildings.GetStorehouses().empty() ? nullptr : buildings.GetStorehouses().front();
    }
    /// Looks for the closest warehouse for the point 'start' (including it) that matches the conditions by the functor
    /// - isWarehouseGood must be a functor taking a "const nobBaseWarhouse&", that returns a bool whether this
    /// warehouse should be considered - to_wh true if path to wh is searched, false for path from wh - length is
    /// optional for the path length - forbidden optional roadSegment that must not be used
    template<class T_IsWarehouseGood>
    nobBaseWarehouse* FindWarehouse(const noRoadNode& start, const T_IsWarehouseGood& isWarehouseGood, bool to_wh,
                                    bool use_boat_roads, unsigned* length = nullptr,
                                    const RoadSegment* forbidden = nullptr) const;
    /// Für alle unbesetzen Straßen Weg neu berechnen
    void FindCarrierForAllRoads();
    /// Versucht für alle Arbeitsplätze eine Arbeitskraft zu suchen
    void FindWarehouseForAllJobs();
    void FindWarehouseForAllJobs(Job job);

    /// Lässt alle Baustellen ggf. noch vorhandenes Baumaterial bestellen
    void FindMaterialForBuildingSites();
    /// Fügt ein RoadNode hinzu, der einen bestimmten Job braucht
    void AddJobWanted(Job job, noRoadNode* workplace);
    /// Entfernt ihn wieder aus der Liste (wenn er dann doch nich mehr gebraucht wird)
    void JobNotWanted(noRoadNode* workplace, bool all = false);
    /// Entfernt einen ausgesuchten Job wieder aus der Liste (wenn er dann doch nich mehr gebraucht wird)
    void OneJobNotWanted(Job job, noRoadNode* workplace);
    /// Versucht für alle verlorenen Waren ohne Ziel Lagerhaus zu finden
    void FindClientForLostWares();
    /// Bestellt eine Ware und gibt sie zurück, falls es eine gibt, ansonsten 0
    Ware* OrderWare(GoodType ware, noBaseBuilding& goal);
    /// Versucht einen Esel zu bestellen, gibt 0 zurück, falls keinen gefunden
    nofCarrier* OrderDonkey(RoadSegment& road) const;
    /// Versucht für einen Esel eine Straße zu finden, in goal wird die Zielflagge zurückgegeben,
    /// sofern eine Straße gefunden wurde, ansonsten ist das ein Lagerhaus oder 0, falls auch das nich gefunden wurde
    RoadSegment* FindRoadForDonkey(noRoadNode& start, noRoadNode** goal);

    /// Sucht für eine (neuproduzierte) Ware einen Abnehmer (wenns keinen gibt, wird ein Lagerhaus gesucht, wenn
    /// es auch dorthin keinen Weg gibt, wird 0 zurückgegeben
    noBaseBuilding* FindClientForWare(const Ware& ware);
    nobBaseWarehouse* FindWarehouseForWare(const Ware& ware) const;

    /// Sucht einen Abnehmer (sprich Militärgebäude), wenn es keinen findet, wird ein Warenhaus zurückgegeben bzw. 0
    nobBaseMilitary* FindClientForCoin(const Ware& ware) const;

    /// Gibt Priorität der Baustelle zurück (entscheidet selbständig, welche Reihenfolge usw)
    /// je kleiner die Rückgabe, destro größer die Priorität!
    unsigned GetBuidingSitePriority(const noBuildingSite* building_site);

    /// Berechnet die Verteilung der Waren auf die einzelnen Gebäude neu
    void RecalcDistribution();
    /// Berechnet die Verteilung einer (bestimmten) Ware
    void RecalcDistributionOfWare(GoodType ware);
    /// Konvertiert die Daten vom wp_transport in "unser" Prioritäten-Format und setzt es
    void ConvertTransportData(const TransportOrders& transport_data);

    /// Ware zur globalen Warenliste hinzufügen und entfernen
    void RegisterWare(Ware& ware) { ware_list.push_back(&ware); }
    void RemoveWare(Ware& ware)
    {
        RTTR_Assert(IsWareRegistred(ware));
        ware_list.remove(&ware);
    }
    bool IsWareRegistred(const Ware& ware);
    bool IsWareDependent(const Ware& ware);

    /// Fügt Waren zur Inventur hinzu
    void IncreaseInventoryWare(GoodType ware, unsigned count);
    void DecreaseInventoryWare(GoodType ware, unsigned count);
    void IncreaseInventoryJob(const Job job, unsigned count) { global_inventory.Add(job, count); }
    void DecreaseInventoryJob(const Job job, unsigned count) { global_inventory.Remove(job, count); }

    /// Gibt Inventory-Settings zurück
    const Inventory& GetInventory() const { return global_inventory; }

    /// Setzt neue Militäreinstellungen
    void ChangeMilitarySettings(const MilitarySettings& military_settings);
    /// Setzt neue Werkzeugeinstellungen
    void ChangeToolsSettings(const ToolSettings& tools_settings, const helpers::EnumArray<int8_t, Tool>& orderChanges);
    /// Setzt neue Verteilungseinstellungen
    void ChangeDistribution(const Distributions& distribution_settings);
    /// Setzt neue Baureihenfolge-Einstellungen
    void ChangeBuildOrder(bool useCustomBuildOrder, const BuildOrders& order_data);

    /// Can this player and the other attack each other?
    bool IsAttackable(unsigned char playerId) const;
    /// Are these players allied? (-> Teamview, attack support, ...)
    bool IsAlly(unsigned char playerId) const;
    /// Order troops of each rank according to `counts` without exceeding `total_max` in total
    void OrderTroops(nobMilitary& goal, std::array<unsigned, NUM_SOLDIER_RANKS> counts, unsigned total_max) const;
    /// Prüft die Besatzung von allen Militärgebäuden und reguliert entsprechend (bei Veränderung der
    /// Militäreinstellungen)
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
    void CallFlagWorker(MapPoint pt, Job job);
    /// Registriert einen Geologen bzw. einen Späher an einer bestimmten Flagge, damit diese informiert werden,
    /// wenn die Flagge abgerissen wird
    void RegisterFlagWorker(nofFlagWorker* flagworker) { flagworkers.push_back(flagworker); }
    void RemoveFlagWorker(nofFlagWorker* flagworker)
    {
        RTTR_Assert(IsFlagWorker(flagworker));
        flagworkers.remove(flagworker);
    }
    bool IsFlagWorker(const nofFlagWorker* flagworker);

    /// Wird aufgerufen, wenn eine Flagge abgerissen wurde, damit das den Flaggen-Arbeitern gesagt werden kann
    void FlagDestroyed(noFlag* flag);

    /// Registriert ein Schiff beim Einwohnermeldeamt
    void RegisterShip(noShip& ship);
    /// Meldet das Schiff wieder ab
    void RemoveShip(noShip* ship);
    /// Versucht, für ein untätiges Schiff eine Arbeit zu suchen
    void GetJobForShip(noShip& ship);
    /// Schiff für Hafen bestellen. Wenn ein Schiff kommt, true.
    bool OrderShip(nobHarborBuilding& hb);
    /// Gibt die ID eines Schiffes zurück
    unsigned GetShipID(const noShip* ship) const;
    /// Gibt ein Schiff anhand der ID zurück bzw. nullptr, wenn keines mit der ID existiert
    noShip* GetShipByID(unsigned ship_id) const;
    /// Gibt die Gesamtanzahl von Schiffen zurück
    unsigned GetNumShips() const { return ships.size(); }
    /// Gibt liste der Schiffe zurück
    const std::vector<noShip*>& GetShips() const { return ships; }
    /// Gibt eine Liste mit allen Häfen dieses Spieler zurück, die an ein bestimmtes Meer angrenzen
    void GetHarborsAtSea(std::vector<nobHarborBuilding*>& harbor_buildings, unsigned short seaId) const;
    /// Gibt die Anzahl der Schiffe, die einen bestimmten Hafen ansteuern, zurück
    unsigned GetShipsToHarbor(const nobHarborBuilding& hb) const;
    /// Sucht einen Hafen in der Nähe, wo dieses Schiff seine Waren abladen kann
    /// gibt true zurück, falls erfolgreich
    bool FindHarborForUnloading(noShip* ship, MapPoint start, unsigned* goal_harborId, std::vector<Direction>* route,
                                nobHarborBuilding* exception);
    /// A ship has discovered new hostile territory --> determines if this is new
    /// i.e. there is a sufficient distance to older locations
    /// Returns true if yes and false if not
    bool ShipDiscoveredHostileTerritory(MapPoint location);

    /// Er gibt auf
    void Surrender();

    /// all allied players get a letter with the location
    void NotifyAlliesOfLocation(MapPoint pt);

    /// This player suggests a pact to target player
    void SuggestPact(unsigned char targetPlayerId, PactType pt, unsigned duration);
    /// Accepts a pact, that this player suggested target player
    void AcceptPact(unsigned id, PactType pt, unsigned char targetPlayer);
    /// Gibt Einverständnis, dass dieser Spieler den Pakt auflösen will
    /// Falls dieser Spieler einen Bündnisvorschlag gemacht hat, wird dieser dagegen zurückgenommen
    void CancelPact(PactType pt, unsigned char otherPlayerIdx);
    PactState GetPactState(PactType pt, unsigned char other_player) const;
    /// Gibt die verbleibende Dauer zurück, die ein Bündnis noch laufen wird (DURATION_INFINITE = für immer)
    unsigned GetRemainingPactTime(PactType pt, unsigned char other_player) const;
    /// Setzt die initialen Bündnisse anhand der Teams
    void MakeStartPacts();
    /// Testet die Bündnisse, ob sie nicht schon abgelaufen sind
    void TestPacts();

    /// Returns all warehouses that can trade with the given goal
    /// IMPORTANT: Warehouses can be destroyed. So check them first before using!
    std::vector<nobBaseWarehouse*> GetWarehousesForTrading(const nobBaseWarehouse& goalWh) const;
    /// Send wares to warehouse wh
    void Trade(nobBaseWarehouse* goalWh, const boost_variant2<GoodType, Job>& what, unsigned count) const;

    void EnableBuilding(BuildingType type) { building_enabled[type] = true; }
    void DisableBuilding(BuildingType type) { building_enabled[type] = false; }
    bool IsBuildingEnabled(BuildingType type) const;
    /// Set the area the player may have territory in
    /// Nothing means all is allowed. See Lua description
    std::vector<MapPoint>& GetRestrictedArea() { return restricted_area; }
    const std::vector<MapPoint>& GetRestrictedArea() const { return restricted_area; }

    void SendPostMessage(std::unique_ptr<PostMsg> msg);

    /// Returns number of tools ordered for the given tool including visual orders (not yet committed)
    unsigned GetToolsOrderedVisual(Tool tool) const;
    unsigned GetToolsOrdered(Tool tool) const;
    /// Changes the current visual tool order by the given amount. Return true if anything was changed (tool order is
    /// clamped to [0,100])
    bool ChangeToolOrderVisual(Tool tool, int changeAmount) const;
    unsigned GetToolPriority(Tool tool) const;
    /// Called when a ordered tool was finished
    void ToolOrderProcessed(Tool tool);

    /// Get a military setting. TODO: Use named type instead of index
    unsigned char GetMilitarySetting(unsigned type) const { return militarySettings_[type]; }
    unsigned char GetTransportPriority(GoodType ware) const { return transportPrio[ware]; }

    //////////////////////////////////////////////////////////////////////////
    // Statistik-Sachen

    void SetStatisticValue(StatisticType type, unsigned value);
    void ChangeStatisticValue(StatisticType type, int change);

    void IncreaseMerchandiseStatistic(GoodType type);

    /// Calculates current statistics
    void CalcStatistics();
    void StatisticStep();

    const Statistic& GetStatistic(StatisticTime time) const { return statistic[time]; };
    unsigned GetStatisticCurrentValue(StatisticType idx) const { return statisticCurrentData[idx]; }

    // Testet ob Notfallprogramm aktiviert werden muss und tut dies dann
    void TestForEmergencyProgramm();
    bool hasEmergency() const { return emergency; }
    /// Testet ob der Spieler noch mehr Katapulte bauen darf
    bool CanBuildCatapult() const;
    /// For debug only
    bool IsDependentFigure(const noFigure& fig);

    void FillVisualSettings(VisualSettings& visualSettings) const;

    static BuildOrders GetStandardBuildOrder();

private:
    /// Access to the world
    GameWorld& world;
    /// List of all buildings
    BuildingRegister buildings; //-V730_NOINIT

    /// Lister aller Straßen von dem Spieler
    std::list<RoadSegment*> roads;

    struct JobNeeded
    {
        Job job;
        noRoadNode* workplace;
    };

    /// Liste von Baustellen/Gebäuden, die bestimmten Beruf wollen
    std::list<JobNeeded> jobs_wanted;

    /// Liste von sämtlichen Waren, die herumgetragen werden und an Fahnen liegen
    std::list<Ware*> ware_list;
    /// Liste von Geologen und Spähern, die an eine Flagge gebunden sind
    std::list<nofFlagWorker*> flagworkers;
    /// Liste von Schiffen dieses Spielers
    std::vector<noShip*> ships;

    /// Liste mit Punkten, die schon von Schiffen entdeckt wurden
    std::vector<MapPoint> enemies_discovered_by_ships;

    /// List which tells if a defender should be send to an attacker
    std::vector<bool> shouldSendDefenderList;

    /// Inventur
    Inventory global_inventory;

    /// Koordinaten des HQs des Spielers
    MapPoint hqPos;

    helpers::EnumArray<Distribution, GoodType> distribution;

    /// Art der Reihenfolge (false = nach Auftraggebung, ansonsten nach build_order)
    bool useCustomBuildOrder_;
    /// Baureihenfolge
    BuildOrders build_order;
    /// Prioritäten der Waren im Transport
    TransportPriorities transportPrio;
    /// Militäreinstellungen (die vom Militärmenü)
    MilitarySettings militarySettings_;
    /// Werkzeugeinstellungen (in der Reihenfolge wie im Fenster!)
    ToolSettings toolsSettings_;
    // qx:tools
    helpers::EnumArray<uint8_t, Tool> tools_ordered;

    /// Bündnisse mit anderen Spielern
    struct Pact
    {
        /// Dauer (in GF), 0 = kein Bündnise, DURATION_INFINITE = Bündnis auf Ewigkeit
        unsigned duration;
        /// Startzeitpunkt (in GF)
        unsigned start;
        /// Bündnis schon akzeptiert oder nur vorgeschlagen?
        bool accepted;
        /// Will dieser Spieler (also der this-Pointer) diesen Vertrag auflösen?
        bool want_cancel;

        Pact() : duration(0), start(0), accepted(false), want_cancel(false) {}
        explicit Pact(SerializedGameData& sgd);
        void Serialize(SerializedGameData& sgd) const;
    };
    /// Bündnisse dieses Spielers mit anderen Spielern
    std::array<helpers::EnumArray<Pact, PactType>, MAX_PLAYERS> pacts;

    // Statistikdaten
    helpers::EnumArray<Statistic, StatisticTime> statistic;

    // Die Statistikwerte die 'aktuell' gemessen werden
    helpers::EnumArray<uint32_t, StatisticType> statisticCurrentData;
    std::array<uint16_t, NUM_STAT_MERCHANDISE_TYPES> statisticCurrentMerchandiseData;

    // Notfall-Programm aktiviert ja/nein (Es gehen nur noch Res an Holzfäller- und Sägewerk-Baustellen raus)
    bool emergency;

    void LoadStandardToolSettings();
    void LoadStandardMilitarySettings();
    void LoadStandardDistribution();
    /// Bündnis (real, d.h. spielentscheidend) abschließen
    void MakePact(PactType pt, unsigned char other_player, unsigned duration);
    /// Called after a pact was changed(added/removed) in both players
    void PactChanged(PactType pt);
    // Sucht Weg für Job zu entsprechenden noRoadNode
    bool FindWarehouseForJob(Job job, noRoadNode& goal) const;
    /// Prüft, ob der Spieler besiegt wurde
    void TestDefeat();
    nobHQ* GetHQ() const;

    //////////////////////////////////////////////////////////////////////////
    /// Unsynchronized state (e.g. lua, gui...)
    //////////////////////////////////////////////////////////////////////////
    /** Polygon(s) defining the area the player may have territory in.
     *  No elements means no restrictions.
     *  Multiple polygons may be specified, see
     *  -http://www.ecse.rpi.edu/Homepages/wrf/Research/Short_Notes/pnpoly.html
     */
    std::vector<MapPoint> restricted_area;
    helpers::EnumArray<bool, BuildingType> building_enabled;

    // TODO: Move to viewer. Mutable as a work-around
    mutable helpers::EnumArray<int8_t, Tool> tools_ordered_delta;
};
