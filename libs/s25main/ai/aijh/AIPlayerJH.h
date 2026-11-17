// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

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
class AIPlayerJH final : public AIPlayer
{
public:
    AIPlayerJH(unsigned char playerId, const GameWorldBase& gwb, AI::Level level);
    ~AIPlayerJH() override;

    AIInterface& getInterface() { return aii; }
    const AIInterface& getInterface() const { return aii; }
    const GameWorldBase& getWorld() const { return gwb; }
    // Required by the AIJobs:
    AIConstruction& getConstruction() { return *construction; }
    const BuildingPlanner& getBldPlanner() const { return *bldPlanner; }
    const AIJob* getCurrentJob() const { return currentJob.get(); }
    unsigned getNumJobs() const;

    void runGF(unsigned gf, bool gfisnwf) override;
    void onChatMessage(unsigned sendPlayerId, ChatDestination, const std::string& msg) override;

    /// Test whether the player should resign or not
    bool testDefeat();
    /// calculates the values the ai should pick for harbor flag & 1 bar buildings between 50% and 100%
    /// return value is whatever has to be added to 4(=50%) for harbor and if anything is left that has to be added to 1
    /// bar setting
    unsigned calcMilSettings();
    /// military & tool production settings
    void adjustSettings();
    /// return number of seaIds with at least 2 harbor spots
    unsigned getNumAIRelevantSeaIds() const;

    bool isInvalidShipyardPosition(MapPoint pt);

    int getResMapValue(MapPoint pt, AIResource res) const;
    const AIResourceMap& getResMap(AIResource res) const;

    Node& getAINode(const MapPoint pt) { return aiMap[pt]; }
    const Node& getAINode(const MapPoint pt) const { return aiMap[pt]; }

    unsigned getNumPlannedConnectedInlandMilitaryBlds()
    {
        return std::max<unsigned>(6u, aii.getMilitaryBuildings().size() / 5u);
    }
    /// returns the percentage*100 of possible normal building places
    unsigned bQsurroundcheck(MapPoint pt, unsigned range, bool includeexisting, unsigned limit = 0);
    /// returns the building the AI uses for troop upgrades
    const nobMilitary* updateUpgradeBuilding();
    /// returns amount of good/people stored in warehouses right now
    unsigned amountInStorage(GoodType good) const;
    unsigned amountInStorage(Job job) const;

    void planNewBuildings(unsigned gf);

    void sendAIEvent(std::unique_ptr<AIEvent::Base> ev);

    /// Executes a job form the job queue
    void executeAIJob();
    /// Tries to build a bld of the given type at that point.
    /// If front is true, then the job is enqueued at the front, else the back
    /// If searchPosition is true, then the point is searched for a good position (around that pt) otherwise the point
    /// is taken
    void addBuildJob(BuildingType type, MapPoint pt, bool front = false, bool searchPosition = true);
    /// Build a new military building at that position
    void addMilitaryBuildJob(MapPoint pt);
    /// adds buildjobs for a buildingtype around every warehouse or military building
    void addBuildJobAroundEveryWarehouse(BuildingType bt);
    void addBuildJobAroundEveryMilBld(BuildingType bt);
    /// blocks goods in each warehouse that has at least limit amount of that good - if all warehouses have enough they
    /// unblock
    void distributeGoodsByBlocking(GoodType good, unsigned limit);
    /// blocks max rank soldiers in warehouse 1 (hq most often), then balances soldiers among frontier warehouses - if
    /// there are no frontier warehouses just pick anything but 1 if there is just 1 then dont block
    void distributeMaxRankSoldiersByBlocking(unsigned limit, nobBaseWarehouse* upwh);
    /// returns true if at least 1 military building has a flag > 0
    bool hasFrontierBuildings();
    /// returns the warehouse closest to the upgradebuilding or if it cant find a way the first warehouse and if there
    /// is no warehouse left null
    nobBaseWarehouse* getUpgradeBuildingWarehouse();
    /// activate gathering of swords,shields,beer,privates(if there is an upgrade building), helpers(if necessary)
    void setGatheringForUpgradeWarehouse(nobBaseWarehouse* upgradewarehouse);
    /// Initializes the nodes on start of the game
    void initNodes();
    /// Updates the nodes around a position
    void updateNodesAround(MapPoint pt, unsigned radius);
    /// Returns the resource on a specific point
    AINodeResource calcResource(MapPoint pt);
    /// Initialize the resource maps
    void initResourceMaps();
    /// Initialize the Store and Military building lists (only required when loading games but the AI doesnt know
    /// whether its a load game or new game so this runs when the ai starts in both cases)
    // now used to init farm space around farms ... lazy legacy
    void initStoreAndMilitarylists();
    // set default start values for the ai for distribution
    void initDistribution();
    // returns true if we can get to the startflag in <maxlen without turning back
    bool isFlagPartofCircle(const noFlag& startFlag, unsigned maxlen, const noFlag& curFlag,
                            helpers::OptionalEnum<Direction> excludeDir, std::vector<const noFlag*> oldFlags);
    /// Finds the best position for a specific resource in an area using the resource maps,
    /// satisfying the minimum value, returns false if no such position is found
    MapPoint findBestPosition(const MapPoint& pt, AIResource res, BuildingQuality size, unsigned radius,
                              int minimum = 1);
    /// Finds a position for the desired building size
    MapPoint simpleFindPosition(const MapPoint& pt, BuildingQuality size, unsigned radius) const;
    /// Find a position for a specific building around a given point
    MapPoint findPositionForBuildingAround(BuildingType type, const MapPoint& around);
    /// Density in percent (0-100)
    unsigned getDensity(MapPoint pt, AIResource res, int radius);
    /// Does some actions after a new military building is occupied
    void handleNewMilitaryBuildingOccupied(MapPoint pt);
    /// Does some actions after a military building is lost
    void handleMilitaryBuilingLost(MapPoint pt);
    /// Does some actions after a building is destroyed
    void handleBuilingDestroyed(MapPoint pt, BuildingType bld);
    // Handle event "no more resources"
    void handleNoMoreResourcesReachable(MapPoint pt, BuildingType bld);
    // A new ship has been built -> handle it
    void handleShipBuilt(MapPoint pt);
    // A new road has been built -> handle it
    void handleRoadConstructionComplete(MapPoint pt, Direction dir);
    // A road construction has failed -> handle it
    void handleRoadConstructionFailed(MapPoint pt, Direction dir);
    // Handle border event
    void handleBorderChanged(MapPoint pt);
    // Handle usual building finished
    void handleBuildingFinished(MapPoint pt, BuildingType bld);

    void handleExpedition(MapPoint pt);
    void handleExpedition(const noShip& ship);
    // Handle chopped tree, test for new space
    void handleTreeChopped(MapPoint pt);
    // Handle new colony
    void handleNewColonyFounded(MapPoint pt);
    /// Lost land to another player
    void handleLostLand(MapPoint pt);
    /// check expeditions (order new / cancel)
    void checkExpeditions();
    /// if we have 1 complete forester but less than 1 military building and less than 2 buildingsites stop production
    void checkForester();
    /// stop/resume granitemine production
    void checkGranitMine();
    /// Tries to attack the enemy
    void tryToAttack();
    /// sea attack
    void trySeaAttack();
    /// checks if there is at least 1 sea id connected to the harbor spot with at least 2 harbor spots! when
    /// onlyempty=true there has to be at least 1 other free harborId
    bool harborPosRelevant(HarborId harborId, bool onlyempty = false) const;
    /// Update BQ and farming ground around new building site + road
    void recalcGround(MapPoint buildingPos, std::vector<Direction>& route_road);

    void saveResourceMapsToFile();

    void initReachableNodes();
    void iterativeReachableNodeChecker(std::queue<MapPoint> toCheck);
    void updateReachableNodes(const std::vector<MapPoint>& pts);

    /// disconnects 'inland' military buildings from road system(and sends out soldiers), sets stop gold, uses the
    /// upgrade building (order new private, kick out general)
    void milUpgradeOptim();

    void setFarmedNodes(MapPoint pt, bool set);
    // removes a no longer used road(and its flags) returns true when there is a building at the flag that might need a
    // new connection
    bool removeUnusedRoad(const noFlag& startFlag, helpers::OptionalEnum<Direction> excludeDir, bool firstflag = true,
                          bool allowcircle = true, bool keepstartflag = false);
    // finds all unused flags and roads, removes flags or reconnects them as neccessary
    void removeAllUnusedRoads(MapPoint pt);
    void checkForUnconnectedBuildingSites();
    // check if there are free soldiers (in hq/storehouses)
    unsigned soldierAvailable(int rank = -1);

    bool huntablesInRange(MapPoint pt, unsigned min);

    bool validTreeinRange(MapPoint pt);

    bool validStoneinRange(MapPoint pt);

    bool validFishInRange(MapPoint pt);

    void executeLuaConstructionOrder(MapPoint pt, BuildingType bt, bool forced = false);

    bool noEnemyHarbor();

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
