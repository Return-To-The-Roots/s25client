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

#include "main.h"
#include "GameWorld.h"
#include "GameClientPlayer.h"
#include "GameCommands.h"
#include "AIJHHelper.h"
#include "nobMilitary.h"
#include "nobBaseWarehouse.h"
#include "nobHarborBuilding.h"
#include "noShip.h"
#include "noBuilding.h"
#include "nobBaseMilitary.h"
#include "noFlag.h"
#include "nobShipYard.h"
#include "GameCommands.h"

typedef unsigned char Direction;

class AIInterface
{
    public:
        AIInterface(const GameWorldBase* const gwb, const GameClientPlayer* const player,
                    const GameClientPlayerList* const players, std::vector<gc::GameCommand*> *gcs, const unsigned char playerID) :
            gwb(gwb), player(player), players(players), gcs(gcs), playerID(playerID) { assert(gcs != NULL); }

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

        /// Returns x-coordinate of the neighbouring point in given direction (6 possible directions)
        inline MapCoord GetXA(MapCoord x, MapCoord y, Direction direction) { return gwb->GetXA(x, y, direction); }

        /// Returns y-coordinate of the neighbouring point in given direction (6 possible directions)
        inline MapCoord GetYA(MapCoord x, MapCoord y, Direction direction) { return gwb->GetYA(x, y, direction); }

        /// Returns x-coordinate of the neighbouring point with radius 2  in given direction (12 possible directions)
        inline MapCoord GetXA2(MapCoord x, MapCoord y, Direction direction) { return gwb->GetXA2(x, y, direction); }

        /// Returns y-coordinate of the neighbouring point with radius 2 in given direction (12 possible directions)
        inline MapCoord GetYA2(MapCoord x, MapCoord y, Direction direction) { return gwb->GetYA2(x, y, direction); }

        /// Transforms coordinates of a point into a neighbour point in given direction
        inline void GetPointA(MapCoord& x, MapCoord& y, Direction direction) { gwb->GetPointA(x, y, direction); }

        /// Get Distance between to points (wraps around at end of world)
        unsigned GetDistance(MapCoord x1, MapCoord y1, MapCoord x2, MapCoord y2) const { return gwb->CalcDistance(x1, y1, x2, y2); }

        unsigned char GetPlayerID() const { return playerID; }

		bool IsDefeated() const {return player->isDefeated();}

        /// Returns a specific object from a position on the map (const version)
        template<typename T> const T* GetSpecObj(MapCoord x, MapCoord y) const { return gwb->GetSpecObj<T>(x, y); }

        /// Returns the resource buried on a given spot (gold, coal, ironore, granite (sub), fish, nothing)
        AIJH::Resource GetSubsurfaceResource(MapCoord x, MapCoord y) const;

        /// Returns the resource on top on a given spot (wood, stones, nothing)
        AIJH::Resource GetSurfaceResource(MapCoord x, MapCoord y) const;

        /// calculates the surface resource value on a given spot (wood/ stones/ farmland)
        /// when given a direction and lastvalue the calculation will be much faster O(n) vs O(n^2)
        int CalcResourceValue(MapCoord x, MapCoord y, AIJH::Resource res, char direction = -1, int lastval = 0xffff) const;

        /// Tests whether a given point is part of the border or not
        bool IsBorder(MapCoord x, MapCoord y) const  { return gwb->GetNode(x, y).boundary_stones[0] == (playerID + 1); }

        /// Tests whether a given point is part of own territory
        bool IsOwnTerritory(MapCoord x, MapCoord y) const { return gwb->GetNode(x, y).owner == (playerID + 1); }

        /// Get a list of dynamic objects (like figures, ships) on a given spot // TODO: zu lowlevilig?
        void GetDynamicObjects(MapCoord x, MapCoord y, list<noBase*> &objects) const { gwb->GetDynamicObjectsFrom(x, y, objects); }

        /// Checks whether there is a road on a point or not
        bool IsRoadPoint(MapCoord x, MapCoord y) const;

        bool GetPointRoad(MapCoord x, MapCoord y, Direction dir) { return gwb->GetPointRoad(x, y, dir) > 0; }

        /// Returns the terrain around a given point in a given direction
        unsigned char GetTerrainAround(MapCoord x, MapCoord y, Direction direction) const { return gwb->GetTerrainAround(x, y, direction); }

        /// Tests whether there is a object of a certain type on a spot
        bool IsObjectTypeOnNode(MapCoord x, MapCoord y, NodalObjectType objectType) const { return gwb->GetNO(x, y)->GetType() == objectType; }

        /// Tests whether there is specific building on a spot
        bool IsBuildingOnNode(MapCoord x, MapCoord y, BuildingType bld) const { return (gwb->GetNO(x, y)->GetType() == NOP_BUILDING || gwb->GetNO(x, y)->GetType() == NOP_BUILDINGSITE) ? (gwb->GetSpecObj<noBaseBuilding>(x, y)->GetBuildingType() == bld) : false; }

		/// test whether there is a military building on a position
		bool IsMilitaryBuildingOnNode(MapCoord x, MapCoord y) const {return ((gwb->GetNO(x,y)->GetType()==NOP_BUILDING || gwb->GetNO(x, y)->GetType() == NOP_BUILDINGSITE) ? (gwb->GetSpecObj<noBaseBuilding>(x, y)->GetBuildingType() >= BLD_BARRACKS && gwb->GetSpecObj<noBaseBuilding>(x, y)->GetBuildingType() <= BLD_FORTRESS) : false); }

        /// Tests whether the ai player can see a point
        bool IsVisible(MapCoord x, MapCoord y) const { return gwb->CalcWithAllyVisiblity(x, y, playerID) == VIS_VISIBLE; }

        bool IsMilitaryBuildingNearNode(MapCoord x, MapCoord y, const unsigned char player) const { return gwb->IsMilitaryBuildingNearNode(x, y, player); }

        bool RoadAvailable(MapCoord x, MapCoord y, unsigned char dir, bool boat_road = false) {return gwb->RoadAvailable(boat_road, x, y, dir, false);}

        ///returns true when the buildingqulity at the 2nd point is lower than the bq on the first point
        bool CalcBQSumDifference(MapCoord x, MapCoord y, MapCoord tx, MapCoord ty);

        /// Returns building quality on a given spot
        BuildingQuality GetBuildingQuality(MapCoord x, MapCoord y) const { return gwb->CalcBQ(x, y, playerID); }
		BuildingQuality GetBuildingQualityAnyOwner(MapCoord x, MapCoord y) const { return gwb->CalcBQ(x, y, playerID,false,true,true); }

        // Tries to find a free path for a road and return length and the route
        bool FindFreePathForNewRoad(MapCoord startX, MapCoord startY, MapCoord targetX, MapCoord targetY, std::vector<Direction> *route = NULL,
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
        void GetMilitaryBuildings(MapCoord x, MapCoord y, unsigned radius, std::list<nobBaseMilitary*> &miliaryBuildings) const { gwb->LookForMilitaryBuildings(miliaryBuildings, x, y, radius); }

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

        /// Returns the ID of a given ship
        unsigned GetShipID(const noShip* ship) const { return player->GetShipID(ship); }

        /// Tests whether there is a possibility to start a expedtion in a given direction from a given position, assuming a given starting harbor
        bool IsExplorationDirectionPossible(MapCoord x, MapCoord y, const nobHarborBuilding* originHarbor, Direction direction) const;

        /// Tests whether there is a possibility to start a expedtion in a given direction from a given position, assuming a given starting harbor
        bool IsExplorationDirectionPossible(MapCoord x, MapCoord y, unsigned int originHarborID, Direction direction) const;

        void SetDefenders(MapCoord x, MapCoord y, unsigned char rank, unsigned char count) {gcs->push_back(new gc::ChangeReserve(x, y, rank, count));}

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
        void ToggleCoins(MapCoord x, MapCoord y) { gcs->push_back(new gc::StopGold(x, y)); }
        void ToggleCoins(const nobMilitary* building) { ToggleCoins(building->GetX(), building->GetY()); }

		///getnation
		unsigned GetNation() {return player->nation;}

		/// send out soldiers
		void SendSoldiersHome(MapCoord x,MapCoord y) {gcs->push_back(new gc::SendSoldiersHome(x,y));}
		
		/// order new soldiers
		void OrderNewSoldiers(MapCoord x,MapCoord y) {gcs->push_back(new gc::OrderNewSoldiers(x,y));}

        /// Starts Preparation of an sea expedition in a habor
        void StartExpedition(MapCoord x, MapCoord y) { gcs->push_back(new gc::StartExpedition(x, y)); }
        void StartExpedition(const nobHarborBuilding* harbor) { StartExpedition(harbor->GetX(), harbor->GetY()); }

        /// Changes an inventory setting of a harbor/storehouse/headquarter
        void ChangeInventorySetting(MapCoord x, MapCoord y); // TODO: more parameters needed
        void ChangeInventorySetting(const nobBaseWarehouse* warehouse) { ChangeInventorySetting(warehouse->GetX(), warehouse->GetY()); }

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
        void ToggleShipyardMode(MapCoord x, MapCoord y) { gcs->push_back(new gc::ChangeShipYardMode(x, y)); }
        void ToggleShipyardMode(const nobShipYard* yard) { ToggleShipyardMode(yard->GetX(), yard->GetY()); }

        /// Destroys a building on a spot
        void DestroyBuilding(MapCoord x, MapCoord y) { gcs->push_back(new gc::DestroyBuilding(x, y)); }
        void DestroyBuilding(const noBuilding* building) { DestroyBuilding(building->GetX(), building->GetY()); }

        /// Destroys a flag on a spot
        void DestroyFlag(MapCoord x, MapCoord y) { gcs->push_back(new gc::DestroyFlag(x, y)); }
        void DestroyFlag(const noFlag* flag) { DestroyFlag(flag->GetX(), flag->GetY()); }
		
        /// Destroys a road on a spot
        void DestroyRoad(MapCoord x, MapCoord y, unsigned char start_dir) { gcs->push_back(new gc::DestroyRoad(x, y,start_dir)); }

        /// Attacks an enemy building
        void Attack(MapCoord x, MapCoord y, unsigned soldiers_count, bool strong_soldiers)
        {
            gcs->push_back(new gc::Attack(x, y, soldiers_count, strong_soldiers));
        }

        /// Sea-Attacks an enemy building
        void SeaAttack(MapCoord x, MapCoord y, unsigned soldiers_count, bool strong_soldiers) {gcs->push_back(new gc::SeaAttack(x, y, soldiers_count, strong_soldiers));}

        /// Builds a road from a starting point along a given route
        void BuildRoad(MapCoord x, MapCoord y, const std::vector<Direction> &route) { gcs->push_back(new gc::BuildRoad(x, y, false, route)); }

        /// Sets a flag on a spot
        void SetFlag(MapCoord x, MapCoord y) { gcs->push_back(new gc::SetFlag(x, y)); }

        /// Sets a building site (new building)
        void SetBuildingSite(MapCoord x, MapCoord y, BuildingType type) { gcs->push_back(new gc::SetBuildingSite(x, y, type)); }

        /// Calls a geologist to a flag
        void CallGeologist(MapCoord x, MapCoord y) { gcs->push_back(new gc::CallGeologist(x, y)); }
        void CallGeologist(const noFlag* flag) { CallGeologist(flag->GetX(), flag->GetY()); }

        /// Sends a chat message to all players TODO: enemy/ally-chat
        void Chat(std::string& message);

        /// Stops/starts production of a producer
        void StopProduction(MapCoord x, MapCoord y) { gcs->push_back(new gc::StopProduction(x, y)); }

        /// changes inventory settings for a warehouse by XOR with old settings (self fixing stupid settings)
        void ChangeInventorySetting(MapCoord x, MapCoord y, unsigned char category, unsigned char state, unsigned char type)
        {
            gcs->push_back(new gc::ChangeInventorySetting(x, y, category, state, type));
        }
};


#endif // AIINTERFACE_H_
