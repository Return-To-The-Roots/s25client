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

#include "ai/AIEventManager.h"
#include "ai/AIPlayer.h"
#include "ai/aijh/AIMap.h"
#include "ai/aijh/AIResourceMap.h"
#include "helpers/OptionalEnum.h"
#include "gameTypes/MapCoordinates.h"
#include <boost/container/static_vector.hpp>
#include <list>
#include <memory>
#include <queue>

class noFlag;
class noShip;
class nobBaseWarehouse;
namespace AIEvent {
class Base;
}

namespace AIJH {
class BuildingPlanner;
class AIConstruction;
class AIJob;

/// Create a subscription which records all nodes for which the BQ (may) have changed
/// Requires arguments to have the same lifetime as the subscription
Subscription recordBQsToUpdate(const GameWorldBase& gw, std::vector<MapPoint>& bqsToUpdate);

/// Klasse für die besser JH-KI
class AIPlayerJH : public AIPlayer
{
public:
    AIPlayerJH(unsigned char playerId, const GameWorldBase& gwb, AI::Level level);
    ~AIPlayerJH() override;

    AIInterface& GetInterface() { return aii; }
    const AIInterface& GetInterface() const { return aii; }
    const GameWorldBase& GetWorld() const { return gwb; }
    // Required by the AIJobs:
    AIConstruction& GetConstruction() { return *construction; }
    const BuildingPlanner& GetBldPlanner() const { return *bldPlanner; }
    const AIJob* GetCurrentJob() const { return currentJob.get(); }
    unsigned GetNumJobs() const;

    void RunGF(unsigned gf, bool gfisnwf) override;

    /// Test whether the player should resign or not
    bool TestDefeat();
    /// calculates the values the ai should pick for harbor flag & 1 bar buildings between 50% and 100%
    /// return value is whatever has to be added to 4(=50%) for harbor and if anything is left that has to be added to 1
    /// bar setting
    unsigned CalcMilSettings();
    /// military & tool production settings
    void AdjustSettings();
    /// return number of seaIds with at least 2 harbor spots
    unsigned GetNumAIRelevantSeaIds() const;

    bool IsInvalidShipyardPosition(MapPoint pt);

    int GetResMapValue(MapPoint pt, AIResource res) const;
    const AIResourceMap& GetResMap(AIResource res) const;

    const Node& GetAINode(const MapPoint pt) const { return aiMap[pt]; }
    unsigned GetNumPlannedConnectedInlandMilitaryBlds()
    {
        return std::max<unsigned>(6u, aii.GetMilitaryBuildings().size() / 5u);
    }
    /// checks distance to all harborpositions
    bool HarborPosClose(MapPoint pt, unsigned range, bool onlyempty = false) const;
    /// returns the percentage*100 of possible normal building places
    unsigned BQsurroundcheck(MapPoint pt, unsigned range, bool includeexisting, unsigned limit = 0);
    /// returns list entry of the building the ai uses for troop upgrades
    int UpdateUpgradeBuilding();
    /// returns amount of good/people stored in warehouses right now
    unsigned AmountInStorage(GoodType good) const;
    unsigned AmountInStorage(Job job) const;

    void PlanNewBuildings(unsigned gf);

    void SendAIEvent(std::unique_ptr<AIEvent::Base> ev);

    Node& GetAINode(const MapPoint pt) { return aiMap[pt]; }
    /// Executes a job form the job queue
    void ExecuteAIJob();
    /// Tries to build a bld of the given type at that point.
    /// If front is true, then the job is enqueued at the front, else the back
    /// If searchPosition is true, then the point is searched for a good position (around that pt) otherwise the point
    /// is taken
    void AddBuildJob(BuildingType type, MapPoint pt, bool front = false, bool searchPosition = true);
    /// Build a new military building at that position
    void AddMilitaryBuildJob(MapPoint pt);
    /// adds buildjobs for a buildingtype around every warehouse or military building
    void AddBuildJobAroundEveryWarehouse(BuildingType bt);
    void AddBuildJobAroundEveryMilBld(BuildingType bt);
    /// Checks the list of military buildingsites and puts the coordinates into the list of military buildings if
    /// building is finished
    void CheckNewMilitaryBuildings();
    /// blocks goods in each warehouse that has at least limit amount of that good - if all warehouses have enough they
    /// unblock
    void DistributeGoodsByBlocking(GoodType good, unsigned limit);
    /// blocks max rank soldiers in warehouse 1 (hq most often), then balances soldiers among frontier warehouses - if
    /// there are no frontier warehouses just pick anything but 1 if there is just 1 then dont block
    void DistributeMaxRankSoldiersByBlocking(unsigned limit, nobBaseWarehouse* upwh);
    /// returns true if at least 1 military building has a flag > 0
    bool HasFrontierBuildings();
    /// returns the warehouse closest to the upgradebuilding or if it cant find a way the first warehouse and if there
    /// is no warehouse left null
    nobBaseWarehouse* GetUpgradeBuildingWarehouse();
    /// activate gathering of swords,shields,beer,privates(if there is an upgrade building), helpers(if necessary)
    void SetGatheringForUpgradeWarehouse(nobBaseWarehouse* upgradewarehouse);
    /// Initializes the nodes on start of the game
    void InitNodes();
    /// Updates the nodes around a position
    void UpdateNodesAround(MapPoint pt, unsigned radius);
    /// Returns the resource on a specific point
    AINodeResource CalcResource(MapPoint pt);
    /// Initialize the resource maps
    void InitResourceMaps();
    /// Initialize the Store and Military building lists (only required when loading games but the AI doesnt know
    /// whether its a load game or new game so this runs when the ai starts in both cases)
    // now used to init farm space around farms ... lazy legacy
    void InitStoreAndMilitarylists();
    // set default start values for the ai for distribution
    void InitDistribution();
    // returns true if we can get to the startflag in <maxlen without turning back
    bool IsFlagPartofCircle(const noFlag& startFlag, unsigned maxlen, const noFlag& curFlag,
                            helpers::OptionalEnum<Direction> excludeDir, std::vector<const noFlag*> oldFlags);
    /// Finds a good position for a specific resource in an area using the resource maps,
    /// first position satisfying threshold is returned, returns false if no such position found
    MapPoint FindGoodPosition(const MapPoint& pt, AIResource res, int threshold, BuildingQuality size, int radius = -1,
                              bool inTerritory = true) const;
    /// Finds the best position for a specific resource in an area using the resource maps,
    /// satisfying the minimum value, returns false if no such position is found
    MapPoint FindBestPosition(const MapPoint& pt, AIResource res, BuildingQuality size, int minimum, int radius = -1,
                              bool inTerritory = true);
    MapPoint FindBestPosition(const MapPoint& pt, AIResource res, BuildingQuality size, int radius = -1,
                              bool inTerritory = true)
    {
        return FindBestPosition(pt, res, size, 1, radius, inTerritory);
    }
    /// finds the best position for a resource that cannot increase (fish,iron,coal,gold,granite,stones)
    MapPoint FindBestPositionDiminishingResource(const MapPoint& pt, AIResource res, BuildingQuality size, int minimum,
                                                 int radius = -1, bool inTerritory = true);
    /// Finds a position for the desired building size
    MapPoint SimpleFindPosition(const MapPoint& pt, BuildingQuality size, int radius = -1) const;
    /// Find a position for a specific building around a given point
    MapPoint FindPositionForBuildingAround(BuildingType type, const MapPoint& around);
    /// Density in percent (0-100)
    unsigned GetDensity(MapPoint pt, AIResource res, int radius);
    /// Recalculate the Buildingquality around a certain point
    void RecalcBQAround(MapPoint pt);
    /// Does some actions after a new military building is occupied
    void HandleNewMilitaryBuildingOccupied(MapPoint pt);
    /// Does some actions after a military building is lost
    void HandleMilitaryBuilingLost(MapPoint pt);
    /// Does some actions after a building is destroyed
    void HandleBuilingDestroyed(MapPoint pt, BuildingType bld);
    // Handle event "no more resources"
    void HandleNoMoreResourcesReachable(MapPoint pt, BuildingType bld);
    // A new ship has been built -> handle it
    void HandleShipBuilt(MapPoint pt);
    // A new road has been built -> handle it
    void HandleRoadConstructionComplete(MapPoint pt, Direction dir);
    // A road construction has failed -> handle it
    void HandleRoadConstructionFailed(MapPoint pt, Direction dir);
    // Handle border event
    void HandleBorderChanged(MapPoint pt);
    // Handle usual building finished
    void HandleBuildingFinished(MapPoint pt, BuildingType bld);

    void HandleExpedition(MapPoint pt);
    void HandleExpedition(const noShip* ship);
    // Handle chopped tree, test for new space
    void HandleTreeChopped(MapPoint pt);
    // Handle new colony
    void HandleNewColonyFounded(MapPoint pt);
    /// Lost land to another player
    void HandleLostLand(MapPoint pt);
    /// Sends a chat messsage to all players
    void Chat(const std::string& message);
    /// check expeditions (order new / cancel)
    void CheckExpeditions();
    /// if we have 1 complete forester but less than 1 military building and less than 2 buildingsites stop production
    void CheckForester();
    /// stop/resume granitemine production
    void CheckGranitMine();
    /// Tries to attack the enemy
    void TryToAttack();
    /// sea attack
    void TrySeaAttack();
    /// checks if there is at least 1 sea id connected to the harbor spot with at least 2 harbor spots! when
    /// onlyempty=true there has to be at least 1 other free harborid
    bool HarborPosRelevant(unsigned harborid, bool onlyempty = false) const;
    /// returns true when a building of the given type is closer to the given position than min (ONLY NOBUSUAL (=no
    /// warehouse/military))
    bool BuildingNearby(MapPoint pt, BuildingType bldType, unsigned min);
    /// Update BQ and farming ground around new building site + road
    void RecalcGround(MapPoint buildingPos, std::vector<Direction>& route_road);

    void SaveResourceMapsToFile();

    void InitReachableNodes();
    void IterativeReachableNodeChecker(std::queue<MapPoint> toCheck);
    void UpdateReachableNodes(const std::vector<MapPoint>& pts);

    /// disconnects 'inland' military buildings from road system(and sends out soldiers), sets stop gold, uses the
    /// upgrade building (order new private, kick out general)
    void MilUpgradeOptim();

    void SetFarmedNodes(MapPoint pt, bool set);
    // removes a no longer used road(and its flags) returns true when there is a building at the flag that might need a
    // new connection
    bool RemoveUnusedRoad(const noFlag& startFlag, helpers::OptionalEnum<Direction> excludeDir, bool firstflag = true,
                          bool allowcircle = true, bool keepstartflag = false);
    // finds all unused flags and roads, removes flags or reconnects them as neccessary
    void RemoveAllUnusedRoads(MapPoint pt);
    void CheckForUnconnectedBuildingSites();
    // check if there are free soldiers (in hq/storehouses)
    unsigned SoldierAvailable(int rank = -1);

    bool HuntablesinRange(MapPoint pt, unsigned min);

    bool ValidTreeinRange(MapPoint pt);

    bool ValidStoneinRange(MapPoint pt);

    bool ValidFishInRange(MapPoint pt);

    void ExecuteLuaConstructionOrder(MapPoint pt, BuildingType bt, bool forced = false);

    bool NoEnemyHarbor();

    void SetResourceMapValue(AIResource res, MapPoint pt, int newvalue)
    {
        resourceMaps[res][pt] = newvalue;
    }

    MapPoint UpgradeBldPos;

private:
    /// The current job the AI is working on
    std::unique_ptr<AIJob> currentJob;
    /// List of coordinates at which military buildings should be
    std::list<MapPoint> milBuildings;
    /// List of coordinates at which military buildingsites should be
    std::list<MapPoint> milBuildingSites;
    /// Nodes containing some information about every map node
    AIMap aiMap;
    /// Resource maps, containing a rating for every map point concerning a resource
    helpers::EnumArray<AIResourceMap, AIResource> resourceMaps;

    unsigned attack_interval;
    unsigned build_interval;
    int isInitGfCompleted;
    /// resigned yes/no
    bool defeated;
    AIEventManager eventManager;
    std::unique_ptr<BuildingPlanner> bldPlanner;
    std::unique_ptr<AIConstruction> construction;

    Subscription subBuilding, subExpedition, subResource, subRoad, subShip, subBQ;
    std::vector<MapPoint> nodesWithOutdatedBQ;
};

} // namespace AIJH
