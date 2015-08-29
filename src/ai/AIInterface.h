// $Id: AIInterface.h
//
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

#ifndef AIINTERFACE_H_
#define AIINTERFACE_H_

//TODO: Cleanup includes!
#include "defines.h"
#include "GameWorld.h"
#include "GameClientPlayer.h"
#include "AIJHHelper.h"
#include "buildings/nobMilitary.h"
#include "buildings/nobBaseWarehouse.h"
#include "buildings/nobHarborBuilding.h"
#include "nodeObjs/noShip.h"
#include "buildings/noBuilding.h"
#include "buildings/nobBaseMilitary.h"
#include "nodeObjs/noFlag.h"
#include "buildings/nobShipYard.h"
#include "factories/GameCommandFactory.h"

class AIInterface: public GameCommandFactory<AIInterface>
{
    public:
        AIInterface(const GameWorldBase& gwb, const GameClientPlayer& player,
                    const GameClientPlayerList& players, std::vector<gc::GameCommandPtr>& gcs, const unsigned char playerID) :
            gwb(gwb), player(player), players(players), gcs(gcs), playerID(playerID) {}

    private:
        typedef GameCommandFactory<AIInterface> GC_Factory;
        friend class GameCommandFactory<AIInterface>;

        /// Pointer to GameWorld, containing all information about the world
        const GameWorldBase& gwb;
        /// Pointer to this player, containing all information about his economoy, buildings, etc.
        const GameClientPlayer& player;
        /// Pointer to list with all other players, for alliances, etc
        const GameClientPlayerList& players;
        /// Pointer to the game commands queue, to send commands to the game
        std::vector<gc::GameCommandPtr>& gcs;
        /// ID of AI player
        const unsigned char playerID;

        bool AddGC(gc::GameCommand* gc)
        {
            gcs.push_back(gc);
            return true;
        }

    public:
        // "Get" commands, to retrieve information from the game

        /// Returns the width of the map
        unsigned short GetMapWidth() const { return gwb.GetWidth(); }

        /// Returns the height of the map
        unsigned short GetMapHeight() const { return gwb.GetHeight(); }

        unsigned GetIdx(MapPoint pt) const { return gwb.GetIdx(pt); }

        /// Returns x-coordinate of the neighbouring point in given direction (6 possible directions)
        inline MapCoord GetXA(const MapPoint pt, Direction direction) { return gwb.GetXA(pt, direction.toUInt()); }
        inline MapCoord GetXA(const MapCoord x, const MapCoord y, Direction direction) { return gwb.GetXA(x, y, direction.toUInt()); }

        /// Transforms coordinates of a point into a neighbour point in given direction
        inline MapPoint GetNeighbour(const MapPoint pt, Direction direction) const { return gwb.GetNeighbour(pt, direction.toUInt()); }

        /// Get Distance between to points (wraps around at end of world)
        unsigned GetDistance(MapPoint p1, MapPoint p2) const { return gwb.CalcDistance(p1, p2); }

        unsigned char GetPlayerID() const { return playerID; }

		bool IsDefeated() const {return player.isDefeated();}

        /// Returns a specific object from a position on the map (const version)
        template<typename T> const T* GetSpecObj(const MapPoint pt) const { return gwb.GetSpecObj<T>(pt); }

        /// Returns the resource buried on a given spot (gold, coal, ironore, granite (sub), fish, nothing)
        AIJH::Resource GetSubsurfaceResource(const MapPoint pt) const;

        /// Returns the resource on top on a given spot (wood, stones, nothing)
        AIJH::Resource GetSurfaceResource(const MapPoint pt) const;

        /// calculates the surface resource value on a given spot (wood/ stones/ farmland)
        /// when given a direction and lastvalue the calculation will be much faster O(n) vs O(n^2)
        int CalcResourceValue(const MapPoint pt, AIJH::Resource res, char direction = -1, int lastval = 0xffff) const;

        /// Calculates the resource value for a given point
        int GetResourceRating(const MapPoint pt, AIJH::Resource res) const;

        /// Tests whether a given point is part of the border or not
        bool IsBorder(const MapPoint pt) const  { return gwb.GetNode(pt).boundary_stones[0] == (playerID + 1); }

        /// Tests whether a given point is part of own territory
        bool IsOwnTerritory(const MapPoint pt) const { return gwb.GetNode(pt).owner == (playerID + 1); }

        /// Get a list of dynamic objects (like figures, ships) on a given spot // TODO: zu lowlevilig?
        std::vector<noBase*> GetDynamicObjects(const MapPoint pt) const { return gwb.GetDynamicObjectsFrom(pt); }

        /// Checks whether there is a road on a point or not
        bool IsRoadPoint(const MapPoint pt) const;

        bool GetPointRoad(const MapPoint pt, Direction dir) { return gwb.GetPointRoad(pt, dir.toUInt()) > 0; }

        /// Returns the terrain around a given point in a given direction
        TerrainType GetTerrainAround(const MapPoint pt, Direction direction) const { return gwb.GetTerrainAround(pt, direction.toUInt()); }

        /// Tests whether there is a object of a certain type on a spot
        bool IsObjectTypeOnNode(const MapPoint pt, NodalObjectType objectType) const { return gwb.GetNO(pt)->GetType() == objectType; }

        /// Tests whether there is specific building on a spot
        bool IsBuildingOnNode(const MapPoint pt, BuildingType bld) const { return (gwb.GetNO(pt)->GetType() == NOP_BUILDING || gwb.GetNO(pt)->GetType() == NOP_BUILDINGSITE) ? (gwb.GetSpecObj<noBaseBuilding>(pt)->GetBuildingType() == bld) : false; }

		/// test whether there is a military building on a position
		bool IsMilitaryBuildingOnNode(const MapPoint pt) const {return ((gwb.GetNO(pt)->GetType()==NOP_BUILDING || gwb.GetNO(pt)->GetType() == NOP_BUILDINGSITE) ? (gwb.GetSpecObj<noBaseBuilding>(pt)->GetBuildingType() >= BLD_BARRACKS && gwb.GetSpecObj<noBaseBuilding>(pt)->GetBuildingType() <= BLD_FORTRESS) : false); }

        /// Tests whether the ai player can see a point
        bool IsVisible(const MapPoint pt) const { return gwb.CalcWithAllyVisiblity(pt, playerID) == VIS_VISIBLE; }

        bool IsMilitaryBuildingNearNode(const MapPoint pt, const unsigned char player) const { return gwb.IsMilitaryBuildingNearNode(pt, player); }

        bool RoadAvailable(const MapPoint pt, unsigned char dir, bool boat_road = false) {return gwb.RoadAvailable(boat_road, pt, dir, false);}

        ///returns true when the buildingqulity at the 2nd point is lower than the bq on the first point
        bool CalcBQSumDifference(const MapPoint pt, const MapPoint t);

        /// Returns building quality on a given spot
        BuildingQuality GetBuildingQuality(const MapPoint pt) const { return gwb.CalcBQ(pt, playerID); }
		BuildingQuality GetBuildingQualityAnyOwner(const MapPoint pt) const { return gwb.CalcBQ(pt, playerID,false,true,true); }

        // Tries to find a free path for a road and return length and the route
        bool FindFreePathForNewRoad(MapPoint start, MapPoint target, std::vector<unsigned char> *route = NULL,
                                    unsigned* length = NULL) const;

        // Tries to find a route from start to target, returning length of that route if it exists
        bool FindPathOnRoads(const noRoadNode* start, const noRoadNode* target, unsigned* length = NULL) const;

        // Checks if it is allowed to build catapults
        bool CanBuildCatapult() const { return player.CanBuildCatapult(); }

		/// checks if the player is allowed to build the buildingtype (lua maybe later addon?)
		bool CanBuildBuildingtype(BuildingType bt) const { return player.IsBuildingEnabled(bt); }

        /// Tests whether a player is attackable or not (alliances, etc)
        bool IsPlayerAttackable(unsigned char playerID) const { return player.IsPlayerAttackable(playerID); }

		/// player.FindWarehouse
		nobBaseWarehouse* FindWarehouse(const noRoadNode* const start, bool (*IsWarehouseGood)(nobBaseWarehouse*, const void*), const RoadSegment* const forbidden, const bool to_wh, const void* param, const bool use_boat_roads, unsigned* const length = 0);
		
        /// Returns a list of military buildings around a given point and a given radius
		sortedMilitaryBlds GetMilitaryBuildings(const MapPoint pt, unsigned radius) const { return gwb.LookForMilitaryBuildings(pt, radius); }

        /// Returns the headquarter of the player (or null if destroyed)
        const nobHQ* GetHeadquarter() const;

        // Returns reference to the list of building sites
        const std::list<noBuildingSite*> &GetBuildingSites() const { return player.GetBuildingSites(); }

        // Returns a list to buildings of a given type
        const std::list<nobUsual*>& GetBuildings(const BuildingType type) const { return player.GetBuildings(type); }

        // Returns a list containing all military buildings
        const std::list<nobMilitary*>& GetMilitaryBuildings() const {return player.GetMilitaryBuildings();}

        //returns a list containing all harbors
        const std::list<nobHarborBuilding*>&GetHarbors() const {return player.GetHarbors();}

        // Returns a list containing all storehouses and harbors and the hq
        const std::list<nobBaseWarehouse*>& GetStorehouses() const {return player.GetStorehouses();}

        // Retrieves the current counts of all buildings
        void GetBuildingCount(BuildingCount& counts) const { player.GetBuildingCount(counts); }

        // Returns the inventory of the ai player
        const Goods* GetInventory() const { return player.GetInventory(); }

        // Returns the number of ships
        unsigned GetShipCount() const { return player.GetShipCount(); }

        // Returns the list of ships
        const std::vector<noShip*>&GetShips() const {return player.GetShips();}

        //returns distance
        unsigned CalcDistance(MapCoord x1, MapCoord y1, MapCoord x2, MapCoord y2) {return gwb.CalcDistance(x1, y1, x2, y2);}
        unsigned CalcDistance(MapPoint p1, MapPoint p2) {return gwb.CalcDistance(p1, p2);}

        /// Returns the ID of a given ship
        unsigned GetShipID(const noShip* ship) const { return player.GetShipID(ship); }

        /// Tests whether there is a possibility to start a expedtion in a given direction from a given position, assuming a given starting harbor
        bool IsExplorationDirectionPossible(const MapPoint pt, const nobHarborBuilding* originHarbor, Direction direction) const;

        /// Tests whether there is a possibility to start a expedtion in a given direction from a given position, assuming a given starting harbor
        bool IsExplorationDirectionPossible(const MapPoint pt, unsigned int originHarborID, Direction direction) const;

        void ToggleCoins(const nobMilitary* building) { ToggleCoins(building->GetPos()); }
        using GC_Factory::ToggleCoins;

		///getnation
		unsigned GetNation() {return player.nation;}

        void StartExpedition(const nobHarborBuilding* harbor) { StartExpedition(harbor->GetPos()); }
        using GC_Factory::StartExpedition;

        /// Changes an inventory setting of a harbor/storehouse/headquarter
        void ChangeInventorySetting(const MapPoint pt); // TODO: more parameters needed
        void ChangeInventorySetting(const nobBaseWarehouse* warehouse) { ChangeInventorySetting(warehouse->GetPos()); }
        using GC_Factory::ChangeInventorySetting;

        /// Lets a ship found a colony
        void FoundColony(const noShip* ship) { FoundColony(player.GetShipID(ship)); }
        using GC_Factory::FoundColony;

        void TravelToNextSpot(Direction direction, const noShip* ship) { TravelToNextSpot(direction, player.GetShipID(ship)); }
        using GC_Factory::TravelToNextSpot;

        void CancelExpedition(const noShip* ship) { CancelExpedition(player.GetShipID(ship)); }
        using GC_Factory::CancelExpedition;

        void ToggleShipYardMode(const nobShipYard* yard) { ToggleShipYardMode(yard->GetPos()); }
        using GC_Factory::ToggleShipYardMode;

        void DestroyBuilding(const noBuilding* building) { DestroyBuilding(building->GetPos()); }
        using GC_Factory::DestroyBuilding;

        void DestroyFlag(const noFlag* flag) { DestroyFlag(flag->GetPos()); }
        using GC_Factory::DestroyFlag;

        void CallGeologist(const noFlag* flag) { CallGeologist(flag->GetPos()); }
        using GC_Factory::CallGeologist;

        /// Sends a chat message to all players TODO: enemy/ally-chat
        void Chat(std::string& message);
};


#endif // AIINTERFACE_H_
