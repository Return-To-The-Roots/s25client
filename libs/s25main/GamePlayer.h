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
#include "gameTypes/MapTypes.h"
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
    void serialize(SerializedGameData& sgd) const;
    // Deserialisieren
    void deserialize(SerializedGameData& sgd);

    GameWorld& getGameWorld() { return world; }
    const GameWorld& getGameWorld() const { return world; }

    const MapPoint& getHQPos() const { return hqPos; }
    const nobHQ* getHQ() const;
    nobHQ* getHQ();
    bool isHQTent() const;
    void setHQIsTent(bool isTent);

    void addBuilding(noBuilding* bld, BuildingType bldType);
    void removeBuilding(noBuilding* bld, BuildingType bldType);
    void addBuildingSite(noBuildingSite* bldSite);
    void removeBuildingSite(noBuildingSite* bldSite);
    const BuildingRegister& getBuildingRegister() const { return buildings; }

    /// Notify that a new road connection exists (not only an existing road splitted)
    void newRoadConnection(RoadSegment* rs);
    /// Neue Straße hinzufügen
    void addRoad(RoadSegment* rs);
    /// Gibt dem Spieler bekannt, das eine Straße abgerissen wurde
    void roadDestroyed();
    /// (Unbesetzte) Straße aus der Liste entfernen
    void deleteRoad(RoadSegment* rs);
    /// Sucht einen Träger für die Straße und ruft ggf den Träger aus dem jeweiligen nächsten Lagerhaus
    bool findCarrierForRoad(RoadSegment& rs) const;
    /// Returns true if the given wh does still exist and hence the ptr is valid
    bool isWarehouseValid(nobBaseWarehouse* wh) const;
    /// Gibt erstes Lagerhaus zurück
    nobBaseWarehouse* getFirstWH()
    {
        return buildings.getStorehouses().empty() ? nullptr : buildings.getStorehouses().front();
    }
    /// Looks for the closest warehouse for the point 'start' (including it) that matches the conditions by the functor
    /// - isWarehouseGood must be a functor taking a "const nobBaseWarhouse&", that returns a bool whether this
    /// warehouse should be considered - to_wh true if path to wh is searched, false for path from wh - length is
    /// optional for the path length - forbidden optional roadSegment that must not be used
    template<class T_IsWarehouseGood>
    nobBaseWarehouse* findWarehouse(const noRoadNode& start, const T_IsWarehouseGood& isWarehouseGood, bool to_wh,
                                    bool use_boat_roads, unsigned* length = nullptr,
                                    const RoadSegment* forbidden = nullptr) const;
    /// Für alle unbesetzen Straßen Weg neu berechnen
    void findCarrierForAllRoads();
    /// Versucht für alle Arbeitsplätze eine Arbeitskraft zu suchen
    void findWarehouseForAllJobs();
    void findWarehouseForAllJobs(Job job);

    /// Lässt alle Baustellen ggf. noch vorhandenes Baumaterial bestellen
    void findMaterialForBuildingSites();
    /// Fügt ein RoadNode hinzu, der einen bestimmten Job braucht
    void addJobWanted(Job job, noRoadNode* workplace);
    /// Entfernt ihn wieder aus der Liste (wenn er dann doch nich mehr gebraucht wird)
    void jobNotWanted(noRoadNode* workplace, bool all = false);
    /// Entfernt einen ausgesuchten Job wieder aus der Liste (wenn er dann doch nich mehr gebraucht wird)
    void oneJobNotWanted(Job job, noRoadNode* workplace);
    /// Versucht für alle verlorenen Waren ohne Ziel Lagerhaus zu finden
    void findClientForLostWares();
    /// Bestellt eine Ware und gibt sie zurück, falls es eine gibt, ansonsten 0
    Ware* orderWare(GoodType ware, noBaseBuilding& goal);
    /// Versucht einen Esel zu bestellen, gibt 0 zurück, falls keinen gefunden
    nofCarrier* orderDonkey(RoadSegment& road) const;
    /// Versucht für einen Esel eine Straße zu finden, in goal wird die Zielflagge zurückgegeben,
    /// sofern eine Straße gefunden wurde, ansonsten ist das ein Lagerhaus oder 0, falls auch das nich gefunden wurde
    RoadSegment* findRoadForDonkey(noRoadNode& start, noRoadNode** goal);

    /// Sucht für eine (neuproduzierte) Ware einen Abnehmer (wenns keinen gibt, wird ein Lagerhaus gesucht, wenn
    /// es auch dorthin keinen Weg gibt, wird 0 zurückgegeben
    noBaseBuilding* findClientForWare(const Ware& ware);
    nobBaseWarehouse* findWarehouseForWare(const Ware& ware) const;

    /// Sucht einen Abnehmer (sprich Militärgebäude), wenn es keinen findet, wird ein Warenhaus zurückgegeben bzw. 0
    nobBaseMilitary* findClientForCoin(const Ware& ware) const;
    nobBaseMilitary* findClientForArmor(const Ware& ware) const;

    /// Gibt Priorität der Baustelle zurück (entscheidet selbständig, welche Reihenfolge usw)
    /// je kleiner die Rückgabe, destro größer die Priorität!
    unsigned getBuidingSitePriority(const noBuildingSite* building_site);

    /// Berechnet die Verteilung der Waren auf die einzelnen Gebäude neu
    void recalcDistribution();
    /// Berechnet die Verteilung einer (bestimmten) Ware
    void recalcDistributionOfWare(GoodType ware);
    /// Konvertiert die Daten vom wp_transport in "unser" Prioritäten-Format und setzt es
    void convertTransportData(const TransportOrders& transport_data);

    /// Ware zur globalen Warenliste hinzufügen und entfernen
    void registerWare(Ware& ware) { ware_list.push_back(&ware); }
    void removeWare(Ware& ware)
    {
        RTTR_Assert(isWareRegistred(ware));
        ware_list.remove(&ware);
    }
    bool isWareRegistred(const Ware& ware);
    bool isWareDependent(const Ware& ware);

    /// Fügt Waren zur Inventur hinzu
    void increaseInventoryWare(GoodType ware, unsigned count);
    void decreaseInventoryWare(GoodType ware, unsigned count);
    void increaseInventoryJob(const Job job, unsigned count) { global_inventory.add(job, count); }
    void decreaseInventoryJob(const Job job, unsigned count) { global_inventory.remove(job, count); }
    void increaseInventoryJob(const ArmoredSoldier job, unsigned count) { global_inventory.add(job, count); }
    void decreaseInventoryJob(const ArmoredSoldier job, unsigned count) { global_inventory.remove(job, count); }

    /// Gibt Inventory-Settings zurück
    const Inventory& getInventory() const { return global_inventory; }

    /// Setzt neue Militäreinstellungen
    void changeMilitarySettings(const MilitarySettings& military_settings);
    /// Setzt neue Werkzeugeinstellungen
    void changeToolsSettings(const ToolSettings& tools_settings, const helpers::EnumArray<int8_t, Tool>& orderChanges);
    /// Setzt neue Verteilungseinstellungen
    void ChangeDistribution(const Distributions& distribution_settings);
    /// Setzt neue Baureihenfolge-Einstellungen
    void ChangeBuildOrder(bool useCustomBuildOrder, const BuildOrders& order_data);

    /// Can this player and the other attack each other?
    bool isAttackable(unsigned char playerId) const;
    /// Are these players allied? (-> Teamview, attack support, ...)
    bool isAlly(unsigned char playerId) const;
    /// Order troops of each rank according to `counts` without exceeding `total_max` in total
    void orderTroops(nobMilitary& goal, std::array<unsigned, NUM_SOLDIER_RANKS> counts, unsigned total_max) const;
    /// Prüft die Besatzung von allen Militärgebäuden und reguliert entsprechend (bei Veränderung der
    /// Militäreinstellungen)
    void regulateAllTroops();
    /// Prüft von allen Militärgebäuden die Fahnen neu
    void recalcMilitaryFlags();
    /// Sucht für Soldaten ein neues Militärgebäude, als Argument wird Referenz auf die
    /// entsprechende Soldatenanzahl im Lagerhaus verlangt
    void newSoldiersAvailable(const unsigned& soldierCount);
    /// Aktualisiert die Verteidiger-Liste
    void refreshDefenderList();
    /// Prüft, ob für einen angreifenden Soldaten ein Verteidger geschickt werden soll
    bool shouldSendDefender();

    /// Ruft einen Geologen
    void callFlagWorker(MapPoint pt, Job job);
    /// Registriert einen Geologen bzw. einen Späher an einer bestimmten Flagge, damit diese informiert werden,
    /// wenn die Flagge abgerissen wird
    void registerFlagWorker(nofFlagWorker* flagworker) { flagworkers.push_back(flagworker); }
    void removeFlagWorker(nofFlagWorker* flagworker)
    {
        RTTR_Assert(isFlagWorker(flagworker));
        flagworkers.remove(flagworker);
    }
    bool isFlagWorker(const nofFlagWorker* flagworker);

    /// Wird aufgerufen, wenn eine Flagge abgerissen wurde, damit das den Flaggen-Arbeitern gesagt werden kann
    void flagDestroyed(noFlag* flag);

    /// Registriert ein Schiff beim Einwohnermeldeamt
    void registerShip(noShip& ship);
    /// Meldet das Schiff wieder ab
    void removeShip(noShip* ship);
    /// Versucht, für ein untätiges Schiff eine Arbeit zu suchen
    void getJobForShip(noShip& ship);
    /// Schiff für Hafen bestellen. Wenn ein Schiff kommt, true.
    bool orderShip(nobHarborBuilding& hb);
    /// Gibt die ID eines Schiffes zurück
    unsigned getShipID(const noShip& ship) const;
    /// Gibt ein Schiff anhand der ID zurück bzw. nullptr, wenn keines mit der ID existiert
    noShip* getShipByID(unsigned ship_id) const;
    /// Gibt die Gesamtanzahl von Schiffen zurück
    unsigned getNumShips() const { return ships.size(); }
    /// Gibt liste der Schiffe zurück
    const std::vector<noShip*>& getShips() const { return ships; }
    /// Gibt die Anzahl der Schiffe, die einen bestimmten Hafen ansteuern, zurück
    unsigned getShipsToHarbor(const nobHarborBuilding& hb) const;
    /// Sucht einen Hafen in der Nähe, wo dieses Schiff seine Waren abladen kann
    /// gibt true zurück, falls erfolgreich
    /// If starting point equals the coastal point of target harbor, return true and an empty `route`.
    bool findHarborForUnloading(noShip* ship, MapPoint start, HarborId* goalHarborId, std::vector<Direction>* route,
                                nobHarborBuilding* exception);
    /// A ship has discovered new hostile territory --> determines if this is new
    /// i.e. there is a sufficient distance to older locations
    /// Returns true if yes and false if not
    bool shipDiscoveredHostileTerritory(MapPoint location);

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
    PactState getPactState(PactType pt, unsigned char other_player) const;
    /// Gibt die verbleibende Dauer zurück, die ein Bündnis noch laufen wird (DURATION_INFINITE = für immer)
    unsigned getRemainingPactTime(PactType pt, unsigned char other_player) const;
    /// Setzt die initialen Bündnisse anhand der Teams
    void makeStartPacts();
    /// Testet die Bündnisse, ob sie nicht schon abgelaufen sind
    void testPacts();

    /// Returns all warehouses that can trade with the given goal
    /// IMPORTANT: Warehouses can be destroyed. So check them first before using!
    std::vector<nobBaseWarehouse*> getWarehousesForTrading(const nobBaseWarehouse& goalWh) const;
    /// Send wares to warehouse wh
    void trade(nobBaseWarehouse* goalWh, const boost_variant2<GoodType, Job>& what, unsigned count) const;

    void enableBuilding(BuildingType type) { building_enabled[type] = true; }
    void disableBuilding(BuildingType type) { building_enabled[type] = false; }
    bool isBuildingEnabled(BuildingType type) const;
    /// Set the area the player may have territory in
    /// Nothing means all is allowed. See Lua description
    std::vector<MapPoint>& getRestrictedArea() { return restricted_area; }
    const std::vector<MapPoint>& getRestrictedArea() const { return restricted_area; }

    void sendPostMessage(std::unique_ptr<PostMsg> msg);

    /// Returns number of tools ordered for the given tool including visual orders (not yet committed)
    unsigned getToolsOrderedVisual(Tool tool) const;
    unsigned getToolsOrdered(Tool tool) const;
    /// Changes the current visual tool order by the given amount. Return true if anything was changed (tool order is
    /// clamped to [0,100])
    bool changeToolOrderVisual(Tool tool, int changeAmount) const;
    unsigned getToolPriority(Tool tool) const;
    /// Called when a ordered tool was finished
    void toolOrderProcessed(Tool tool);

    /// Get a military setting. TODO: Use named type instead of index
    unsigned char getMilitarySetting(unsigned type) const { return militarySettings_[type]; }
    unsigned char getTransportPriority(GoodType ware) const { return transportPrio[ware]; }

    //////////////////////////////////////////////////////////////////////////
    // Statistik-Sachen

    void setStatisticValue(StatisticType type, unsigned value);
    void changeStatisticValue(StatisticType type, int change);

    void increaseMerchandiseStatistic(GoodType type);

    /// Calculates current statistics
    void calcStatistics();
    void statisticStep();

    const Statistic& getStatistic(StatisticTime time) const { return statistic[time]; };
    unsigned getStatisticCurrentValue(StatisticType idx) const { return statisticCurrentData[idx]; }

    // Stop wares restricted in emergency mode that are waiting in warehouse to be transported already
    void cancelWaresForEmergencyProtocol();
    // Testet ob Notfallprogramm aktiviert werden muss und tut dies dann
    void testForEmergencyProgramm();
    bool hasEmergency() const { return emergency; }
    /// Testet ob der Spieler noch mehr Katapulte bauen darf
    bool canBuildCatapult() const;
    /// For debug only
    bool isDependentFigure(const noFigure& fig);

    void fillVisualSettings(VisualSettings& visualSettings) const;

    static BuildOrders getStandardBuildOrder();

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
        void serialize(SerializedGameData& sgd) const;
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

    void loadStandardToolSettings();
    void loadStandardMilitarySettings();
    void loadStandardDistribution();
    /// Bündnis (real, d.h. spielentscheidend) abschließen
    void makePact(PactType pt, unsigned char other_player, unsigned duration);
    /// Called after a pact was changed(added/removed) in both players
    void pactChanged(PactType pt);
    // Sucht Weg für Job zu entsprechenden noRoadNode
    bool findWarehouseForJob(Job job, noRoadNode& goal) const;

    /// Find best building for the ware according to the priority reduced by the distance.
    /// T_GetPriority must be a functor taking a "const noBaseBuilding&" and returning an unsigned priority.
    /// A zero priority means that the building is not suitable for this ware.
    /// Returns nullptr if no building was found
    template<class T_GetPriority, class T_Buildings>
    typename T_Buildings::value_type findClientImpl(const Ware& ware, T_GetPriority&& getPriority,
                                                    const T_Buildings& buildings) const;

    /// Prüft, ob der Spieler besiegt wurde
    void testDefeat();

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
