// $Id: AIInterface.h
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

#ifndef AIINTERFACE_H_
#define AIINTERFACE_H_

#include "defines.h"
#include "GameWorld.h"
#include "GameClientPlayer.h"
#include "GameCommands.h"
#include "AIJHHelper.h"
#include "buildings/nobMilitary.h"
#include "buildings/nobBaseWarehouse.h"
#include "buildings/nobHarborBuilding.h"
#include "nodeObjs/noShip.h"
#include "buildings/noBuilding.h"
#include "buildings/nobBaseMilitary.h"
#include "nodeObjs/noFlag.h"
#include "buildings/nobShipYard.h"
#include "GameCommands.h"

typedef unsigned char Direction;

class AIInterface
{
    public:
        AIInterface(const GameWorldBase* const gwb, const GameClientPlayer* const player,
                    const GameClientPlayerList* const players, std::vector<gc::GameCommand*> *gcs, const unsigned char playerID) :
            gwb(gwb), player(player), players(players), gcs(gcs), playerID(playerID) { assert(gcs); }

    private:
        /// Pointer to GameWorld, containing all information about the world
        const GameWorldBase* const gwb;
        /// Pointer to this player, containing all information about his economoy, buildings, etc.
        const GameClientPlayer* const player;
        /// Pointer to list with all other players, for alliances, etc
        const GameClientPlayerList* const players;
        /// Pointer to the game commands queue, to send commands to the game
        std::vector<gc::GameCommand*> * gcs;
        /// ID of AI player
        const unsigned char playerID;

    public:
        // "Get" commands, to retrieve information from the game

        /// Returns the width of the map
        unsigned short GetMapWidth() const { return gwb->GetWidth(); }

        /// Returns the height of the map
        unsigned short GetMapHeight() const { return gwb->GetHeight(); }

        unsigned GetIdx(MapPoint pt) const { return gwb->GetIdx(pt); }

        /// Returns x-coordinate of the neighbouring point in given direction (6 possible directions)
        inline MapCoord GetXA(const MapPoint pt, Direction direction) { return gwb->GetXA(pt, direction); }
        inline MapCoord GetXA(const MapCoord x, const MapCoord y, Direction direction) { return gwb->GetXA(x, y, direction); }

        /*/// Returns y-coordinate of the neighbouring point in given direction (6 possible directions)
        inline MapCoord GetYA(const MapPoint pt, Direction direction) { return gwb->GetYA(pt, direction); }

        /// Returns x-coordinate of the neighbouring point with radius 2  in given direction (12 possible directions)
        inline MapCoord GetXA2(const MapPoint pt, Direction direction) { return gwb->GetXA2(pt, direction); }

        /// Returns y-coordinate of the neighbouring point with radius 2 in given direction (12 possible directions)
        inline MapCoord GetYA2(const MapPoint pt, Direction direction) { return gwb->GetYA2(pt, direction); }*/

        /// Transforms coordinates of a point into a neighbour point in given direction
        inline MapPoint GetNeighbour(const MapPoint pt, Direction direction) const { return gwb->GetNeighbour(pt, direction); }

        /// Get Distance between to points (wraps around at end of world)
        unsigned GetDistance(MapPoint p1, MapPoint p2) const { return gwb->CalcDistance(p1, p2); }

        unsigned char GetPlayerID() const { return playerID; }

		bool IsDefeated() const {return player->isDefeated();}

        /// Returns a specific object from a position on the map (const version)
        template<typename T> const T* GetSpecObj(const MapPoint pt) const { return gwb->GetSpecObj<T>(pt); }

        /// Returns the resource buried on a given spot (gold, coal, ironore, granite (sub), fish, nothing)
        AIJH::Resource GetSubsurfaceResource(const MapPoint pt) const;

        /// Returns the resource on top on a given spot (wood, stones, nothing)
        AIJH::Resource GetSurfaceResource(const MapPoint pt) const;

        /// calculates the surface resource value on a given spot (wood/ stones/ farmland)
        /// when given a direction and lastvalue the calculation will be much faster O(n) vs O(n^2)
        int CalcResourceValue(const MapPoint pt, AIJH::Resource res, char direction = -1, int lastval = 0xffff) const;

        /// Tests whether a given point is part of the border or not
        bool IsBorder(const MapPoint pt) const  { return gwb->GetNode(pt).boundary_stones[0] == (playerID + 1); }

        /// Tests whether a given point is part of own territory
        bool IsOwnTerritory(const MapPoint pt) const { return gwb->GetNode(pt).owner == (playerID + 1); }

        /// Get a list of dynamic objects (like figures, ships) on a given spot // TODO: zu lowlevilig?
        std::vector<noBase*> GetDynamicObjects(const MapPoint pt) const { return gwb->GetDynamicObjectsFrom(pt); }

        /// Checks whether there is a road on a point or not
        bool IsRoadPoint(const MapPoint pt) const;

        bool GetPointRoad(const MapPoint pt, Direction dir) { return gwb->GetPointRoad(pt, dir) > 0; }

        /// Returns the terrain around a given point in a given direction
        unsigned char GetTerrainAround(const MapPoint pt, Direction direction) const { return gwb->GetTerrainAround(pt, direction); }

        /// Tests whether there is a object of a certain type on a spot
        bool IsObjectTypeOnNode(const MapPoint pt, NodalObjectType objectType) const { return gwb->GetNO(pt)->GetType() == objectType; }

        /// Tests whether there is specific building on a spot
        bool IsBuildingOnNode(const MapPoint pt, BuildingType bld) const { return (gwb->GetNO(pt)->GetType() == NOP_BUILDING || gwb->GetNO(pt)->GetType() == NOP_BUILDINGSITE) ? (gwb->GetSpecObj<noBaseBuilding>(pt)->GetBuildingType() == bld) : false; }

		/// test whether there is a military building on a position
		bool IsMilitaryBuildingOnNode(const MapPoint pt) const {return ((gwb->GetNO(pt)->GetType()==NOP_BUILDING || gwb->GetNO(pt)->GetType() == NOP_BUILDINGSITE) ? (gwb->GetSpecObj<noBaseBuilding>(pt)->GetBuildingType() >= BLD_BARRACKS && gwb->GetSpecObj<noBaseBuilding>(pt)->GetBuildingType() <= BLD_FORTRESS) : false); }

        /// Tests whether the ai player can see a point
        bool IsVisible(const MapPoint pt) const { return gwb->CalcWithAllyVisiblity(pt, playerID) == VIS_VISIBLE; }

        bool IsMilitaryBuildingNearNode(const MapPoint pt, const unsigned char player) const { return gwb->IsMilitaryBuildingNearNode(pt, player); }

        bool RoadAvailable(const MapPoint pt, unsigned char dir, bool boat_road = false) {return gwb->RoadAvailable(boat_road, pt, dir, false);}

        ///returns true when the buildingqulity at the 2nd point is lower than the bq on the first point
        bool CalcBQSumDifference(const MapPoint pt, const MapPoint t);

        /// Returns building quality on a given spot
        BuildingQuality GetBuildingQuality(const MapPoint pt) const { return gwb->CalcBQ(pt, playerID); }
		BuildingQuality GetBuildingQualityAnyOwner(const MapPoint pt) const { return gwb->CalcBQ(pt, playerID,false,true,true); }

        // Tries to find a free path for a road and return length and the route
        bool FindFreePathForNewRoad(MapPoint start, MapPoint target, std::vector<Direction> *route = NULL,
                                    unsigned* length = NULL) const;

        // Tries to find a route from start to target, returning length of that route if it exists
        bool FindPathOnRoads(const noRoadNode* start, const noRoadNode* target, unsigned* length = NULL) const;

        // Checks if it is allowed to build catapults
        bool CanBuildCatapult() const { return player->CanBuildCatapult(); }

		/// checks if the player is allowed to build the buildingtype (lua maybe later addon?)
		bool CanBuildBuildingtype(BuildingType bt) const { return player->IsBuildingEnabled(bt); }

        /// Tests whether a player is attackable or not (alliances, etc)
        bool IsPlayerAttackable(unsigned char playerID) const { return player->IsPlayerAttackable(playerID); }

		/// player->FindWarehouse
		nobBaseWarehouse* FindWarehouse(const noRoadNode* const start, bool (*IsWarehouseGood)(nobBaseWarehouse*, const void*), const RoadSegment* const forbidden, const bool to_wh, const void* param, const bool use_boat_roads, unsigned* const length = 0);
		
        /// Returns a list of military buildings around a given point and a given radius
		std::set<nobBaseMilitary*> GetMilitaryBuildings(const MapPoint pt, unsigned radius) const { return gwb->LookForMilitaryBuildings(pt, radius); }

        /// Returns the headquarter of the player (or null if destroyed)
        const nobHQ* GetHeadquarter() const;

        // Returns reference to the list of building sites
        const std::list<noBuildingSite*> &GetBuildingSites() const { return player->GetBuildingSites(); }

        // Returns a list to buildings of a given type
        const std::list<nobUsual*>& GetBuildings(const BuildingType type) const { return player->GetBuildings(type); }

        // Returns a list containing all military buildings
        const std::list<nobMilitary*>& GetMilitaryBuildings() const {return player->GetMilitaryBuildings();}

        //returns a list containing all harbors
        const std::list<nobHarborBuilding*>&GetHarbors() const {return player->GetHarbors();}

        // Returns a list containing all storehouses and harbors and the hq
        const std::list<nobBaseWarehouse*>& GetStorehouses() const {return player->GetStorehouses();}

        // Retrieves the current counts of all buildings
        void GetBuildingCount(BuildingCount& counts) const { player->GetBuildingCount(counts); }

        // Returns the inventory of the ai player
        const Goods* GetInventory() const { return player->GetInventory(); }

        // Returns the number of ships
        unsigned GetShipCount() const { return player->GetShipCount(); }

        // Returns the list of ships
        const std::vector<noShip*>&GetShips() const {return player->GetShips();}

        //returns distance
        unsigned CalcDistance(MapCoord x1, MapCoord y1, MapCoord x2, MapCoord y2) {return gwb->CalcDistance(x1, y1, x2, y2);}
        unsigned CalcDistance(MapPoint p1, MapPoint p2) {return gwb->CalcDistance(p1, p2);}

        /// Returns the ID of a given ship
        unsigned GetShipID(const noShip* ship) const { return player->GetShipID(ship); }

        /// Tests whether there is a possibility to start a expedtion in a given direction from a given position, assuming a given starting harbor
        bool IsExplorationDirectionPossible(const MapPoint pt, const nobHarborBuilding* originHarbor, Direction direction) const;

        /// Tests whether there is a possibility to start a expedtion in a given direction from a given position, assuming a given starting harbor
        bool IsExplorationDirectionPossible(const MapPoint pt, unsigned int originHarborID, Direction direction) const;

        void SetDefenders(const MapPoint pt, unsigned char rank, unsigned char count) {gcs->push_back(new gc::ChangeReserve(pt, rank, count));}

        // "Set" commands, to send commands to the game

        /// Sets new military settings for the ai player (8 values)
        void SetMilitarySettings(std::vector<unsigned char> &newSettings) { gcs->push_back(new gc::ChangeMilitary(newSettings)); }

        /// Sets new tool production settings -poc
        void SetToolSettings(std::vector<unsigned char> &newSettings) {gcs->push_back(new gc::ChangeTools(newSettings));}

        // Sets new distribution of goods
        void SetDistribution(const std::vector<unsigned char>&distribution_settings) {gcs->push_back(new gc::ChangeDistribution(distribution_settings));}

        /// Simply surrenders...
        void Surrender() { gcs->push_back(new gc::Surrender()); }

        /// Toggles coin delivery on/off for a military building
        void ToggleCoins(const MapPoint pt) { gcs->push_back(new gc::StopGold(pt)); }
        void ToggleCoins(const nobMilitary* building) { ToggleCoins(building->GetPos()); }

		///getnation
		unsigned GetNation() {return player->nation;}

		/// send out soldiers
		void SendSoldiersHome(const MapPoint pt) {gcs->push_back(new gc::SendSoldiersHome(pt));}
		
		/// order new soldiers
		void OrderNewSoldiers(const MapPoint pt) {gcs->push_back(new gc::OrderNewSoldiers(pt));}

        /// Starts Preparation of an sea expedition in a habor
        void StartExpedition(const MapPoint pt) { gcs->push_back(new gc::StartExpedition(pt)); }
        void StartExpedition(const nobHarborBuilding* harbor) { StartExpedition(harbor->GetPos()); }

        /// Changes an inventory setting of a harbor/storehouse/headquarter
        void ChangeInventorySetting(const MapPoint pt); // TODO: more parameters needed
        void ChangeInventorySetting(const nobBaseWarehouse* warehouse) { ChangeInventorySetting(warehouse->GetPos()); }

        /// Lets a ship found a colony
        void FoundColony(unsigned int shipID) { gcs->push_back(new gc::ExpeditionCommand(gc::ExpeditionCommand::FOUNDCOLONY, shipID)); }
        void FoundColony(const noShip* ship) { FoundColony(player->GetShipID(ship)); }

        /// Lets a ship travel to a new harbor spot in a given direction
        void TravelToNextSpot(Direction direction, unsigned int shipID) { gcs->push_back(new gc::ExpeditionCommand(gc::ExpeditionCommand::Action(direction + 2), shipID)); }
        void TravelToNextSpot(Direction direction, const noShip* ship) { TravelToNextSpot(direction, player->GetShipID(ship)); }

        /// Cancels an expedition
        void CancelExpedition(unsigned int shipID) { gcs->push_back(new gc::ExpeditionCommand(gc::ExpeditionCommand::CANCELEXPEDITION, shipID)); }
        void CancelExpedition(const noShip* ship) { CancelExpedition(player->GetShipID(ship)); }

        /// Toggles the construction mode of the shipyard between boat and ship
        void ToggleShipyardMode(const MapPoint pt) { gcs->push_back(new gc::ChangeShipYardMode(pt)); }
        void ToggleShipyardMode(const nobShipYard* yard) { ToggleShipyardMode(yard->GetPos()); }

        /// Destroys a building on a spot
        void DestroyBuilding(const MapPoint pt) { gcs->push_back(new gc::DestroyBuilding(pt)); }
        void DestroyBuilding(const noBuilding* building) { DestroyBuilding(building->GetPos()); }

        /// Destroys a flag on a spot
        void DestroyFlag(const MapPoint pt) { gcs->push_back(new gc::DestroyFlag(pt)); }
        void DestroyFlag(const noFlag* flag) { DestroyFlag(flag->GetPos()); }
		
        /// Destroys a road on a spot
        void DestroyRoad(const MapPoint pt, unsigned char start_dir) { gcs->push_back(new gc::DestroyRoad(pt,start_dir)); }

        /// Attacks an enemy building
        void Attack(const MapPoint pt, unsigned soldiers_count, bool strong_soldiers)
        {
            gcs->push_back(new gc::Attack(pt, soldiers_count, strong_soldiers));
        }

        /// Sea-Attacks an enemy building
        void SeaAttack(const MapPoint pt, unsigned soldiers_count, bool strong_soldiers) {gcs->push_back(new gc::SeaAttack(pt, soldiers_count, strong_soldiers));}

        /// Builds a road from a starting point along a given route
        void BuildRoad(const MapPoint pt, const std::vector<Direction> &route) { gcs->push_back(new gc::BuildRoad(pt, false, route)); }

        /// Sets a flag on a spot
        void SetFlag(const MapPoint pt) { gcs->push_back(new gc::SetFlag(pt)); }

        /// Sets a building site (new building)
        void SetBuildingSite(const MapPoint pt, BuildingType type) { gcs->push_back(new gc::SetBuildingSite(pt, type)); }

        /// Calls a geologist to a flag
        void CallGeologist(const MapPoint pt) { gcs->push_back(new gc::CallGeologist(pt)); }
        void CallGeologist(const noFlag* flag) { CallGeologist(flag->GetPos()); }

        /// Sends a chat message to all players TODO: enemy/ally-chat
        void Chat(std::string& message);

        /// Stops/starts production of a producer
        void StopProduction(const MapPoint pt) { gcs->push_back(new gc::StopProduction(pt)); }

        /// changes inventory settings for a warehouse by XOR with old settings (self fixing stupid settings)
        void ChangeInventorySetting(const MapPoint pt, unsigned char category, unsigned char state, unsigned char type)
        {
            gcs->push_back(new gc::ChangeInventorySetting(pt, category, state, type));
        }
};


#endif // AIINTERFACE_H_
