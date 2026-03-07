// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
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
class nobMilitary;
class nofCarrier;
class nofFlagWorker;
class PostMsg;
class RoadSegment;
class SerializedGameData;
struct VisualSettings;
class Ware;

/// Indicates whether a pact exists
enum class PactState
{
    None,       /// No pact agreed
    InProgress, /// Pact offered but not yet accepted
    Accepted    /// Pact is active
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
        // Index currently at the 'front' (right side in the statistics window)
        uint16_t currentIndex;
        // Counter: every fourth update, copy data to the long-term statistics
        uint16_t counter;
    };

    // Distribution information
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

    /// Serialize
    void Serialize(SerializedGameData& sgd) const;
    // Deserialize
    void Deserialize(SerializedGameData& sgd);

    GameWorld& GetGameWorld() { return world; }
    const GameWorld& GetGameWorld() const { return world; }

    const MapPoint& GetHQPos() const { return hqPos; }
    void AddBuilding(noBuilding* bld, BuildingType bldType);
    void RemoveBuilding(noBuilding* bld, BuildingType bldType);
    void AddBuildingSite(noBuildingSite* bldSite);
    void RemoveBuildingSite(noBuildingSite* bldSite);
    const BuildingRegister& GetBuildingRegister() const { return buildings; }

    /// Notify that a new road connection exists (not only an existing road splitted)
    void NewRoadConnection(RoadSegment* rs);
    /// Add new road
    void AddRoad(RoadSegment* rs);
    /// Notify the player that a road was removed
    void RoadDestroyed();
    /// Remove (unoccupied) road from the list
    void DeleteRoad(RoadSegment* rs);
    /// Find a carrier for the road and, if needed, request one from the nearest warehouse
    bool FindCarrierForRoad(RoadSegment* rs) const;
    /// Returns true if the given wh does still exist and hence the ptr is valid
    bool IsWarehouseValid(nobBaseWarehouse* wh) const;
    void RecalcToolSettings();
    /// Returns the first warehouse
    nobBaseWarehouse* GetFirstWH()
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
    /// Recalculate paths for all unoccupied roads
    void FindCarrierForAllRoads();
    /// Tries to find a worker for all workplaces
    void FindWarehouseForAllJobs();
    void FindWarehouseForAllJobs(Job job);

    /// Lets all construction sites order any still-missing building material
    void FindMaterialForBuildingSites();
    /// Adds a RoadNode that needs a specific job
    void AddJobWanted(Job job, noRoadNode* workplace);
    /// Removes it from the list again (if it is no longer needed)
    void JobNotWanted(noRoadNode* workplace, bool all = false);
    /// Removes a selected job from the list again (if it is no longer needed)
    void OneJobNotWanted(Job job, noRoadNode* workplace);
    /// Tries to find a warehouse for all lost wares without a destination
    void FindClientForLostWares();
    /// Orders a ware and returns it if one is found, otherwise 0
    Ware* OrderWare(GoodType ware, noBaseBuilding* goal);
    /// Tries to order a donkey, returns 0 if none was found
    nofCarrier* OrderDonkey(RoadSegment* road) const;
    /// Tries to find a road for a donkey; goal returns the target flag if a road was found,
    /// otherwise it is a warehouse or 0 if that was not found either
    RoadSegment* FindRoadForDonkey(noRoadNode* start, noRoadNode** goal);

    /// Finds a recipient for a (newly produced) ware (if none exists, a warehouse is searched;
    /// if no path exists there either, 0 is returned)
    noBaseBuilding* FindClientForWare(const Ware& ware);
    nobBaseWarehouse* FindWarehouseForWare(const Ware& ware) const;

    /// Finds a recipient (i.e. military building); if none is found, returns a warehouse or 0
    nobBaseMilitary* FindClientForCoin(const Ware& ware) const;

    /// Returns construction-site priority (decides ordering automatically, etc.)
    /// The smaller the return value, the higher the priority
    unsigned GetBuidingSitePriority(const noBuildingSite* building_site);

    /// Recalculate ware distribution across buildings
    void RecalcDistribution();
    /// Recalculate distribution of one specific ware
    void RecalcDistributionOfWare(GoodType ware);
    /// Converts wp_transport data into "our" priority format and applies it
    void ConvertTransportData(const TransportOrders& transport_data);

    /// Add/remove ware to/from the global ware list
    void RegisterWare(Ware& ware) { ware_list.push_back(&ware); }
    void RemoveWare(Ware& ware)
    {
        RTTR_Assert(IsWareRegistred(ware));
        ware_list.remove(&ware);
    }
    bool IsWareRegistred(const Ware& ware);
    bool IsWareDependent(const Ware& ware);

    /// Add wares to inventory
    void IncreaseInventoryWare(GoodType ware, unsigned count);
    void DecreaseInventoryWare(GoodType ware, unsigned count);
    void IncreaseInventoryJob(const Job job, unsigned count) { global_inventory.Add(job, count); }
    void DecreaseInventoryJob(const Job job, unsigned count) { global_inventory.Remove(job, count); }

    /// Returns inventory settings
    const Inventory& GetInventory() const { return global_inventory; }

    /// Apply new military settings
    void ChangeMilitarySettings(const MilitarySettings& military_settings);
    /// Apply new tool settings
    void ChangeToolsSettings(const ToolSettings& tools_settings, const helpers::EnumArray<int8_t, Tool>& orderChanges);
    /// Apply new distribution settings
    void ChangeDistribution(const Distributions& distribution_settings);
    /// Apply new build-order settings
    void ChangeBuildOrder(bool useCustomBuildOrder, const BuildOrders& order_data);

    /// Can this player and the other attack each other?
    bool IsAttackable(unsigned char playerId) const;
    /// Are these players allied? (-> Teamview, attack support, ...)
    bool IsAlly(unsigned char playerId) const;
    /// Order troops of each rank according to `counts` without exceeding `total_max` in total
    void OrderTroops(nobMilitary* goal, std::array<unsigned, NUM_SOLDIER_RANKS> counts, unsigned total_max) const;
    /// Checks staffing of all military buildings and regulates it accordingly (after military-setting changes)
    void RegulateAllTroops();
    /// Recalculate flags for all military buildings
    void RecalcMilitaryFlags();
    /// Find a new military building for soldiers; argument is a reference to the corresponding soldier count
    /// in the warehouse
    void NewSoldiersAvailable(const unsigned& soldier_count);
    /// Refresh defender list
    void RefreshDefenderList();
    /// Checks whether a defender should be sent against an attacking soldier
    bool ShouldSendDefender();

    /// Calls a flag worker (e.g. geologist)
    void CallFlagWorker(MapPoint pt, Job job);
    /// Registers a geologist/scout at a specific flag so they can be informed if the flag is demolished
    void RegisterFlagWorker(nofFlagWorker* flagworker) { flagworkers.push_back(flagworker); }
    void RemoveFlagWorker(nofFlagWorker* flagworker)
    {
        RTTR_Assert(IsFlagWorker(flagworker));
        flagworkers.remove(flagworker);
    }
    bool IsFlagWorker(const nofFlagWorker* flagworker);

    /// Called when a flag was demolished so flag workers can be notified
    void FlagDestroyed(noFlag* flag);

    /// Register a ship
    void RegisterShip(noShip& ship);
    /// Unregister a ship
    void RemoveShip(noShip* ship);
    /// Try to find a job for an idle ship
    void GetJobForShip(noShip& ship);
    /// Order a ship for a harbor. Returns true if a ship is on the way.
    bool OrderShip(nobHarborBuilding& hb);
    /// Returns a ship's ID
    unsigned GetShipID(const noShip* ship) const;
    /// Returns a ship by ID, or nullptr if no ship with that ID exists
    noShip* GetShipByID(unsigned ship_id) const;
    /// Returns total number of ships
    unsigned GetNumShips() const { return ships.size(); }
    /// Returns list of ships
    const std::vector<noShip*>& GetShips() const { return ships; }
    /// Returns all harbors of this player bordering a specific sea
    void GetHarborsAtSea(std::vector<nobHarborBuilding*>& harbor_buildings, unsigned short seaId) const;
    /// Returns number of ships currently heading to a specific harbor
    unsigned GetShipsToHarbor(const nobHarborBuilding& hb) const;
    /// Finds a nearby harbor where this ship can unload goods
    /// Returns true on success
    bool FindHarborForUnloading(noShip* ship, MapPoint start, unsigned* goal_harborId, std::vector<Direction>* route,
                                nobHarborBuilding* exception);
    /// A ship has discovered new hostile territory --> determines if this is new
    /// i.e. there is a sufficient distance to older locations
    /// Returns true if yes and false if not
    bool ShipDiscoveredHostileTerritory(MapPoint location);

    /// This player surrenders
    void Surrender();

    /// all allied players get a letter with the location
    void NotifyAlliesOfLocation(MapPoint pt);

    /// This player suggests a pact to target player
    void SuggestPact(unsigned char targetPlayerId, PactType pt, unsigned duration);
    /// Accepts a pact, that this player suggested target player
    void AcceptPact(unsigned id, PactType pt, unsigned char targetPlayer);
    /// Declares that this player wants to cancel the pact
    /// If this player proposed a pact, that proposal is withdrawn instead
    void CancelPact(PactType pt, unsigned char otherPlayerIdx);
    PactState GetPactState(PactType pt, unsigned char other_player) const;
    /// Returns remaining pact duration (DURATION_INFINITE = forever)
    unsigned GetRemainingPactTime(PactType pt, unsigned char other_player) const;
    /// Initialize starting pacts based on teams
    void MakeStartPacts();
    /// Tests pacts for expiration
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
    // Statistics

    void SetStatisticValue(StatisticType type, unsigned value);
    void ChangeStatisticValue(StatisticType type, int change);

    void IncreaseMerchandiseStatistic(GoodType type);

    /// Calculates current statistics
    void CalcStatistics();
    void StatisticStep();

    const Statistic& GetStatistic(StatisticTime time) const { return statistic[time]; };
    unsigned GetStatisticCurrentValue(StatisticType idx) const { return statisticCurrentData[idx]; }

    // Checks whether emergency program must be activated and does so if required
    void TestForEmergencyProgramm();
    bool hasEmergency() const { return emergency; }
    /// Checks whether the player may build more catapults
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

    /// List of all roads owned by this player
    std::list<RoadSegment*> roads;

    struct JobNeeded
    {
        Job job;
        noRoadNode* workplace;
    };

    /// List of construction sites/buildings that need a specific job
    std::list<JobNeeded> jobs_wanted;

    /// List of all wares being carried and lying at flags
    std::list<Ware*> ware_list;
    /// List of geologists/scouts assigned to a flag
    std::list<nofFlagWorker*> flagworkers;
    /// List of this player's ships
    std::vector<noShip*> ships;

    /// List of points already discovered by ships
    std::vector<MapPoint> enemies_discovered_by_ships;

    /// List which tells if a defender should be send to an attacker
    std::vector<bool> shouldSendDefenderList;

    /// Inventory
    Inventory global_inventory;

    /// Coordinates of the player's HQ
    MapPoint hqPos;

    helpers::EnumArray<Distribution, GoodType> distribution;

    /// Build-order mode (false = order by creation time, otherwise use build_order)
    bool useCustomBuildOrder_;
    /// Build order
    BuildOrders build_order;
    /// Transport priorities for wares
    TransportPriorities transportPrio;
    /// Military settings (from military menu)
    MilitarySettings militarySettings_;
    /// Tool settings (in the same order as in the UI window)
    ToolSettings toolsSettings_;
    // qx:tools
    helpers::EnumArray<uint8_t, Tool> tools_ordered;

    /// Pacts with other players
    struct Pact
    {
        /// Duration (in GF), 0 = no pact, DURATION_INFINITE = infinite pact
        unsigned duration;
        /// Start time (in GF)
        unsigned start;
        /// Pact accepted already or only proposed?
        bool accepted;
        /// Does this player (this-pointer) want to cancel this pact?
        bool want_cancel;

        Pact() : duration(0), start(0), accepted(false), want_cancel(false) {}
        explicit Pact(SerializedGameData& sgd);
        void Serialize(SerializedGameData& sgd) const;
    };
    /// This player's pacts with other players
    std::array<helpers::EnumArray<Pact, PactType>, MAX_PLAYERS> pacts;

    // Statistics data
    helpers::EnumArray<Statistic, StatisticTime> statistic;

    // Statistics values currently being measured
    helpers::EnumArray<uint32_t, StatisticType> statisticCurrentData;
    std::array<uint16_t, NUM_STAT_MERCHANDISE_TYPES> statisticCurrentMerchandiseData;

    // Emergency program enabled yes/no (resources are only sent to woodcutter/sawmill construction sites)
    bool emergency;

    void LoadStandardToolSettings();
    void LoadStandardMilitarySettings();
    void LoadStandardDistribution();
    void ApplyConfigBuildingDisables();
    /// Conclude pact (real, game-decisive)
    void MakePact(PactType pt, unsigned char other_player, unsigned duration);
    /// Called after a pact was changed(added/removed) in both players
    void PactChanged(PactType pt);
    // Finds path for job to the corresponding noRoadNode
    bool FindWarehouseForJob(Job job, noRoadNode* goal) const;
    /// Checks whether the player has been defeated
    void TestDefeat();

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
