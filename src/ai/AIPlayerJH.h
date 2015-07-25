// $Id: AIPlayerJH.h 9591 2015-02-01 09:39:07Z marcus $
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
#ifndef AIPLAYERJH_H_INCLUDED
#define AIPLAYERJH_H_INCLUDED

#pragma once

#include "AIBase.h"
#include "gameTypes/MapTypes.h"
#include "gameData/GameConsts.h"
#include "GameClientPlayer.h"
#include "AIJHHelper.h"
#include "GameWorld.h"
#include "AIEventManager.h"
#include "AIConstruction.h"

#include <queue>
#include <deque>
#include <list>

class noFlag;
class noBaseBuilding;
class noRoadNode;
class nobBaseMilitary;
class AIPlayerJH;
class nobMilitary;
class nobBaseMilitary;

enum PositionSearchState
{
    SEARCH_IN_PROGRESS,
    SEARCH_SUCCESSFUL,
    SEARCH_FAILED
};

struct PositionSearch
{
    // where did the search start?
    MapPoint start;

    // what do we want to find?
    AIJH::Resource res;

    // and how much of that at least?
    int minimum;

    // how much space do we need?
    BuildingQuality size;

    // how many nodes should we test each cycle?
    int nodesPerStep;

    // which nodes have already been tested or will be tested next (=already in queue)?
    std::vector<bool>* tested;

    // which nodes are currently queued to be tested next?
    std::queue<unsigned>* toTest;

    // results
    MapPoint result;
    int resultValue;

    // what to we want to build there?
    BuildingType bld;

    bool best;

    PositionSearch(const MapPoint pt, AIJH::Resource res, int minimum, BuildingQuality size, BuildingType bld, bool best = false)
        : start(pt), res(res), minimum(minimum), size(size), bld(bld), best(best) { }

    ~PositionSearch()
    {
        delete tested;
        delete toTest;
    }
};

/// Klasse für die besser JH-KI
class AIPlayerJH : public AIBase
{
        friend class AIJH::BuildJob;
        friend class AIJH::EventJob;
        friend class AIJH::ConnectJob;
        friend class AIJH::SearchJob;
        friend class iwAIDebug;
    public:
        AIPlayerJH(const unsigned char playerid, const GameWorldBase* const gwb, const GameClientPlayer* const player,
                   const GameClientPlayerList* const players, const GlobalGameSettings* const ggs,
                   const AI::Level level);
		int initgfcomplete;
        int GetResMapValue(const MapPoint pt, AIJH::Resource res);
        AIInterface* GetInterface() { return aii; }

        /// Test whether the player should resign or not
        bool TestDefeat();

		/// calculates the values the ai should pick for harbor flag & 1 bar buildings between 50% and 100%  
		/// return value is whatever has to be added to 4(=50%) for harbor and if anything is left that has to be added to 1 bar setting
		unsigned CalcMilSettings();

		/// military & tool production settings 
		void AdjustSettings();

        ///return number of sea_ids with at least 2 harbor spots
        unsigned GetCountofAIRelevantSeaIds();
		
        bool IsInvalidShipyardPosition(const MapPoint pt);

    protected:
        void RunGF(const unsigned gf,bool gfisnwf);

        void SendAIEvent(AIEvent::Base* ev);
		
        /// resigned yes/no
        bool defeated;

        /// Executes a job form the job queue
        void ExecuteAIJob();
        void AddBuildJob(AIJH::BuildJob* job, bool front = false) { construction.AddBuildJob(job, front); }
        void AddBuildJob(BuildingType type, const MapPoint pt, bool front = false);
		//adds buildjobs for a buildingtype around every warehouse or military building
		void AddBuildJobAroundEvery(BuildingType bt, bool warehouse);

        /// Checks the list of military buildingsites and puts the coordinates into the list of military buildings if building is finished
        void CheckNewMilitaryBuildings();

        /// blocks goods in each warehouse that has at least limit amount of that good - if all warehouses have enough they unblock
        void DistributeGoodsByBlocking(unsigned char goodnumber, unsigned limit);
		/// blocks people in each warehouse that has at least limit amount of that job - if all warehouses have enough they unblock
        void DistributePeopleByBlocking(unsigned char jobnumber, unsigned limit);

		/// blocks max rank soldiers in warehouse 1 (hq most often), then balances soldiers among frontier warehouses - if there are no frontier warehouses just pick anything but 1 if there is just 1 then dont block
        void DistributeMaxRankSoldiersByBlocking(unsigned limit,nobBaseWarehouse* upwh);

		/// returns true if at least 1 military building has a flag > 0
		bool HasFrontierBuildings();

		/// returns the warehouse closest to the upgradebuilding or if it cant find a way the first warehouse and if there is no warehouse left null
		nobBaseWarehouse* GetUpgradeBuildingWarehouse();

		/// activate gathering of swords,shields,beer,privates(if there is an upgrade building), helpers(if necessary)
		void SetGatheringForUpgradeWarehouse(nobBaseWarehouse* upgradewarehouse);

        /// Initializes the nodes on start of the game
        void InitNodes();
        /// Updates all nodes (takes a while so dont call it too often)
        void UpdateNodes();

        /// Updates the nodes around a position
        void UpdateNodesAround(const MapPoint pt, unsigned radius);
        void UpdateNodesAroundNoBorder(const MapPoint pt, unsigned radius);

        /// Returns the resource on a specific point
        AIJH::Resource CalcResource(const MapPoint pt);

        /// Initialize the resource maps
        void InitResourceMaps();
        /// Initialize the Store and Military building lists (only required when loading games but the AI doesnt know whether its a load game or new game so this runs when the ai starts in both cases)

        //now used to init farm space around farms ... lazy legacy
        void InitStoreAndMilitarylists();
		//set default start values for the ai for distribution
		void InitDistribution();

        //returns true if we can get to the startflag in <maxlen without turning back
        bool IsFlagPartofCircle(const noFlag* startFlag, unsigned maxlen, const noFlag* curFlag, unsigned char excludeDir, bool init, std::vector<MapPoint> oldFlags);

        //globally update a layer of the resource map
        void RecalcResource(AIJH::Resource res);

        //get me the current addon settings?

        /// Changes a single resource map around point pt in radius; to every point around pt distanceFromCenter * value is added
        void ChangeResourceMap(const MapPoint pt, unsigned radius, std::vector<int> &resMap, int value);

        /// Finds a good position for a specific resource in an area using the resource maps,
        /// first position satisfying threshold is returned, returns false if no such position found
        bool FindGoodPosition(MapPoint& pt, AIJH::Resource res, int threshold, BuildingQuality size, int radius = -1, bool inTerritory = true);

        PositionSearch* CreatePositionSearch(MapPoint& pt, AIJH::Resource res, BuildingQuality size, int minimum, BuildingType bld, bool best = false);

        // Find position that satifies search->minimum or best (takes longer!)
        PositionSearchState FindGoodPosition(PositionSearch* search, bool best = false);

        /// Finds the best position for a specific resource in an area using the resource maps,
        /// satisfying the minimum value, returns false if no such position is found
        bool FindBestPosition(MapPoint& pt, AIJH::Resource res, BuildingQuality size, int minimum, int radius = -1, bool inTerritory = true);
        bool FindBestPosition(MapPoint& pt, AIJH::Resource res, BuildingQuality size, int radius = -1, bool inTerritory = true)
        { return FindBestPosition(pt, res, size, 1, radius, inTerritory); }
        ///finds the best position for a resource that cannot increase (fish,iron,coal,gold,granite,stones)
        bool FindBestPositionDiminishingResource(MapPoint& pt, AIJH::Resource res, BuildingQuality size, int minimum, int radius = -1, bool inTerritory = true);

        /// Finds a position for the desired building size
        bool SimpleFindPosition(MapPoint& pt, BuildingQuality size, int radius = -1);

        /// Density in percent (0-100)
        unsigned GetDensity(MapPoint pt, AIJH::Resource res, int radius);

        /// Recalculate the Buildingquality around a certain point
        void RecalcBQAround(const MapPoint pt);

        /// Does some actions after a new military building is occupied
        void HandleNewMilitaryBuilingOccupied(const MapPoint pt);

        /// Does some actions after a military building is lost
        void HandleMilitaryBuilingLost(const MapPoint pt);

        /// Does some actions after a building is destroyed
        void HandleBuilingDestroyed(MapPoint pt, BuildingType bld);

        // Handle event "no more resources"
        void HandleNoMoreResourcesReachable(const MapPoint pt, BuildingType bld);

        // A new ship has been built -> handle it
        void HandleShipBuilt(const MapPoint pt);

        // A new road has been built -> handle it
        void HandleRoadConstructionComplete(MapPoint pt, unsigned char dir);

        // A road construction has failed -> handle it
        void HandleRoadConstructionFailed(const MapPoint pt, unsigned char dir);

        // Handle border event
        void HandleBorderChanged(const MapPoint pt);

        // Handle usual building finished
        void HandleBuildingFinished(const MapPoint pt, BuildingType bld);

        void HandleExpedition(const MapPoint pt);
        void HandleExpedition(const noShip* ship);

        // Handle chopped tree, test for new space
        void HandleTreeChopped(const MapPoint pt);

        // Handle new colony
        void HandleNewColonyFounded(const MapPoint pt);

        /// Sends a chat messsage to all players
        void Chat(std::string message);

        /// Tries to attack the enemy
        void TryToAttack();
        /// sea attack
        void TrySeaAttack();

        /// checks if there is at least 1 sea id connected to the harbor spot with at least 2 harbor spots! when onlyempty=true there has to be at least 1 other free harborid
        bool HarborPosRelevant(unsigned harborid, bool onlyempty = false);

        /// returns true when a building of the given type is closer to the given position than min (ONLY NOBUSUAL (=no warehouse/military))
        bool BuildingNearby(const MapPoint pt, BuildingType bld, unsigned min);

        /// Update BQ and farming ground around new building site + road
        void RecalcGround(const MapPoint buildingPos, std::vector<unsigned char> &route_road);

        void SaveResourceMapsToFile();

        void InitReachableNodes();
        void UpdateReachableNodes(const MapPoint pt, unsigned radius);
        void IterativeReachableNodeChecker(std::queue<MapPoint>& toCheck);

		/// disconnects 'inland' military buildings from road system(and sends out soldiers), sets stop gold, uses the upgrade building (order new private, kick out general)
		void MilUpgradeOptim();

        void SetFarmedNodes(const MapPoint pt, bool set);

        //removes a no longer used road(and its flags) returns true when there is a building at the flag that might need a new connection
        bool RemoveUnusedRoad(const noFlag* startFlag, unsigned char excludeDir = 0xFF, bool firstflag = true, bool allowcircle = true,bool keepstartflag=false);
        //finds all unused flags and roads, removes flags or reconnects them as neccessary
        void RemoveAllUnusedRoads(const MapPoint pt);

        // check if there are free soldiers (in hq/storehouses)
        unsigned SoldierAvailable(int rank=-1);

        bool HuntablesinRange(const MapPoint pt, unsigned min);

        bool ValidTreeinRange(const MapPoint pt);

        bool ValidStoneinRange(const MapPoint pt);

        bool ValidFishInRange(const MapPoint pt);

		void ExecuteLuaConstructionOrder(const MapPoint pt, BuildingType bt, bool forced=false);

        bool NoEnemyHarbor();
		
        void SetResourceMap(AIJH::Resource res, unsigned nodenumber, int newvalue) {resourceMaps[res][nodenumber] = newvalue;}
		
		MapCoord UpgradeBldX,UpgradeBldY;
		

    protected:
        /// The current job the AI is working on
        AIJH::Job* currentJob;

        /// Contains the jobs the AI should try to execute, for example build jobs
        /// std::deque<AIJH::Job*> aiJobs;

        /// List of coordinates at which military buildings should be
        std::list<MapPoint> milBuildings;

        /// List of coordinates at which military buildingsites should be
        std::list<MapPoint> milBuildingSites;

        /// Nodes containing some information about every map node
        std::vector<AIJH::Node> nodes;

        /// Resource maps, containing a rating for every map point concerning a resource
        std::vector<std::vector<int> > resourceMaps;

		// Required by the AIJobs:
		

        const std::string& GetPlayerName() { return player->name; }
        unsigned char GetPlayerID() { return playerid; }
        AIConstruction* GetConstruction() { return &construction; }
        AIJH::Job* GetCurrentJob() { return currentJob; }
    public:
        inline AIJH::Node& GetAINode(const MapPoint pt) { return nodes[pt.x + gwb->GetWidth() * pt.y]; }
		inline unsigned GetJobNum() const { return eventManager.GetEventNum() + construction.GetBuildJobNum() + construction.GetConnectJobNum(); }	
		int UpgradeBldListNumber;
		unsigned PlannedConnectedInlandMilitary() {return aii->GetMilitaryBuildings().size()/5<6 ? 6:aii->GetMilitaryBuildings().size()/5;}
        /// checks distance to all harborpositions
        bool HarborPosClose(const MapPoint pt, unsigned range, bool onlyempty = false);
		/// returns the percentage*100 of possible normal building places
        unsigned BQsurroundcheck(const MapPoint pt, unsigned range, bool includeexisting,unsigned limit=0);
		/// returns list entry of the building the ai uses for troop upgrades
		int UpdateUpgradeBuilding();
		/// returns amount of good/people stored in warehouses right now
		unsigned AmountInStorage(unsigned char num,unsigned char page);
		

// Event...
    protected:
        AIEventManager eventManager;
        AIConstruction construction;


};


#endif //!AIPLAYERJH_H_INCLUDED
