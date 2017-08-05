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

#include "GameCommand.h"
#include "GamePlayer.h"
#include "NodalObjectTypes.h"
#include "ai/AIResource.h"
#include "factories/GameCommandFactory.h"
#include "world/GameWorldBase.h"
#include "gameTypes/Direction.h"

class nobHQ;
class nobShipYard;
class RoadSegment;
class noBuilding;
class noBuildingSite;
class noFlag;
class noRoadNode;
class noShip;
class nobBaseWarehouse;
class nobHarborBuilding;
class nobMilitary;
class nobUsual;
struct Inventory;

class AIInterface : public GameCommandFactory
{
public:
    AIInterface(const GameWorldBase& gwb, std::vector<gc::GameCommandPtr>& gcs, const unsigned char playerID)
        : gwb(gwb), player_(gwb.GetPlayer(playerID)), gcs(gcs), playerID_(playerID)
    {
    }

private:
    /// Pointer to GameWorld, containing all information about the world
    const GameWorldBase& gwb;
    /// Pointer to this player, containing all information about his economoy, buildings, etc.
    const GamePlayer& player_;
    /// Pointer to the game commands queue, to send commands to the game
    std::vector<gc::GameCommandPtr>& gcs;
    /// ID of AI player
    const unsigned char playerID_;

    bool AddGC(gc::GameCommand* gc) override
    {
        gcs.push_back(gc);
        return true;
    }

public:
    // "Get" commands, to retrieve information from the game

    /// Return the width of the map
    unsigned short GetMapWidth() const { return gwb.GetWidth(); }

    /// Return the height of the map
    unsigned short GetMapHeight() const { return gwb.GetHeight(); }

    unsigned GetIdx(MapPoint pt) const { return gwb.GetIdx(pt); }

    TerrainType GetTerrain(MapPoint pt) const { return gwb.GetNode(pt).t1; }

    /// Return x-coordinate of the neighboring point in given direction (6 possible directions)
    MapCoord GetXA(const MapPoint pt, Direction direction) { return gwb.GetXA(pt, direction.toUInt()); }
    MapCoord GetXA(const MapCoord x, const MapCoord y, Direction direction) { return gwb.GetXA(x, y, direction.toUInt()); }

    /// Transforms coordinates of a point into a neighbour point in given direction
    MapPoint GetNeighbour(const MapPoint pt, Direction direction) const { return gwb.GetNeighbour(pt, direction.toUInt()); }

    /// Return all points in a radius around pt (excluding pt) that satisfy a given condition.
    /// Points can be transformed (e.g. to flags at those points) by the functor taking a map point and a radius
    /// Number of results is constrained to maxResults (if > 0)
    template<unsigned T_maxResults, class T_TransformPt, class T_IsValidPt>
    std::vector<typename T_TransformPt::result_type> GetPointsInRadius(const MapPoint pt, const unsigned radius, T_TransformPt transformPt,
                                                                       T_IsValidPt isValid) const
    {
        return gwb.GetPointsInRadius<T_maxResults>(pt, radius, transformPt, isValid);
    }
    template<class T_TransformPt>
    std::vector<typename T_TransformPt::result_type> GetPointsInRadius(const MapPoint pt, const unsigned radius,
                                                                       T_TransformPt transformPt) const
    {
        return GetPointsInRadius<0>(pt, radius, transformPt, ReturnConst<bool, true>());
    }
    std::vector<MapPoint> GetPointsInRadius(const MapPoint pt, const unsigned radius) const
    {
        return GetPointsInRadius<0>(pt, radius, Identity<MapPoint>(), ReturnConst<bool, true>());
    }

    /// Get Distance between to points (wraps around at end of world)
    unsigned GetDistance(MapPoint p1, MapPoint p2) const { return gwb.CalcDistance(p1, p2); }

    unsigned char GetPlayerId() const { return playerID_; }
    unsigned GetPlayerCount() const { return gwb.GetPlayerCount(); }

    bool IsDefeated() const { return player_.IsDefeated(); }

    /// Return a specific object from a position on the map (const version)
    template<typename T>
    const T* GetSpecObj(const MapPoint pt) const
    {
        return gwb.GetSpecObj<T>(pt);
    }

    /// Return the resource buried on a given spot (gold, coal, ironore, granite (sub), fish, nothing)
    AIJH::Resource GetSubsurfaceResource(const MapPoint pt) const;

    /// Return the resource on top on a given spot (wood, stones, nothing)
    AIJH::Resource GetSurfaceResource(const MapPoint pt) const;

    /// Calculate the surface resource value on a given spot (wood/ stones/ farmland)
    /// when given a direction and lastvalue the calculation will be much faster O(n) vs O(n^2)
    int CalcResourceValue(const MapPoint pt, AIJH::Resource res, char direction = -1, int lastval = 0xffff) const;

    /// Calculate the resource value for a given point
    int GetResourceRating(const MapPoint pt, AIJH::Resource res) const;

    /// Test whether a given point is part of the border or not
    bool IsBorder(const MapPoint pt) const { return gwb.GetNode(pt).boundary_stones[0] == (playerID_ + 1); }

    /// Test whether a given point is part of own territory
    bool IsOwnTerritory(const MapPoint pt) const { return gwb.GetNode(pt).owner == (playerID_ + 1); }

    /// Get a list of dynamic objects (like figures, ships) on a given spot // TODO: to low level?
    std::vector<noBase*> GetDynamicObjects(const MapPoint pt) const { return gwb.GetDynamicObjectsFrom(pt); }

    /// Checks whether there is a road on a point or not
    bool IsRoadPoint(const MapPoint pt) const;

    bool IsRoad(const MapPoint pt, Direction dir) { return gwb.GetPointRoad(pt, dir) > 0; }

    /// Return the terrain on the right side when going in a given direction
    TerrainType GetRightTerrain(const MapPoint pt, Direction direction) const { return gwb.GetRightTerrain(pt, direction); }

    /// Test whether there is a object of a certain type on a spot
    bool IsObjectTypeOnNode(const MapPoint pt, NodalObjectType objectType) const { return gwb.GetNO(pt)->GetType() == objectType; }

    /// Test whether there is specific building on a spot
    bool IsBuildingOnNode(const MapPoint pt, BuildingType bld) const
    {
        return (gwb.GetNO(pt)->GetType() == NOP_BUILDING || gwb.GetNO(pt)->GetType() == NOP_BUILDINGSITE) ?
                 (gwb.GetSpecObj<noBaseBuilding>(pt)->GetBuildingType() == bld) :
                 false;
    }

    /// test whether there is a military building on a position
    bool IsMilitaryBuildingOnNode(const MapPoint pt) const
    {
        const NodalObjectType noType = gwb.GetNO(pt)->GetType();
        if(noType != NOP_BUILDING && noType != NOP_BUILDINGSITE)
            return false;
        const BuildingType bldType = gwb.GetSpecObj<noBaseBuilding>(pt)->GetBuildingType();
        return (bldType >= BLD_BARRACKS && bldType <= BLD_FORTRESS);
    }

    /// Test whether the ai player can see a point
    bool IsVisible(const MapPoint pt) const { return gwb.CalcVisiblityWithAllies(pt, playerID_) == VIS_VISIBLE; }

    bool IsMilitaryBuildingNearNode(const MapPoint pt, const unsigned char player) const
    {
        return gwb.IsMilitaryBuildingNearNode(pt, player);
    }

    bool RoadAvailable(const MapPoint pt, bool boat_road = false) { return gwb.IsRoadAvailable(boat_road, pt); }

    /// Return true when the building quality at the 2nd point is lower than the bq on the first point
    bool CalcBQSumDifference(const MapPoint pt, const MapPoint t);

    /// Return building quality on a given spot
    BuildingQuality GetBuildingQuality(const MapPoint pt) const;
    BuildingQuality GetBuildingQualityAnyOwner(const MapPoint pt) const;

    /// Tries to find a free path for a road and return length and the route
    bool FindFreePathForNewRoad(MapPoint start, MapPoint target, std::vector<Direction>* route = NULL, unsigned* length = NULL) const;

    /// Tries to find a route from start to target, returning length of that route if it exists
    bool FindPathOnRoads(const noRoadNode& start, const noRoadNode& target, unsigned* length = NULL) const;

    /// Checks if it is allowed to build catapults
    bool CanBuildCatapult() const { return player_.CanBuildCatapult(); }

    /// checks if the player is allowed to build the building type (lua maybe later addon?)
    bool CanBuildBuildingtype(BuildingType bt) const { return player_.IsBuildingEnabled(bt); }

    /// Test whether a player is attackable or not (alliances, etc)
    bool IsPlayerAttackable(unsigned char playerID) const { return player_.IsAttackable(playerID); }

    /// player.FindWarehouse
    template<class T_IsWarehouseGood>
    nobBaseWarehouse* FindWarehouse(const noRoadNode& start, const T_IsWarehouseGood& isWarehouseGood, const bool to_wh,
                                    const bool use_boat_roads, unsigned* const length = 0, const RoadSegment* const forbidden = NULL) const
    {
        return player_.FindWarehouse(start, isWarehouseGood, to_wh, use_boat_roads, length, forbidden);
    }

    /// Return a list of military buildings around a given point and a given radius
    sortedMilitaryBlds GetMilitaryBuildings(const MapPoint pt, unsigned radius) const { return gwb.LookForMilitaryBuildings(pt, radius); }

    /// Return the headquarter of the player (or null if destroyed)
    const nobHQ* GetHeadquarter() const;

    /// Return reference to the list of building sites
    const std::list<noBuildingSite*>& GetBuildingSites() const { return player_.GetBuildingSites(); }
    const std::list<noBuildingSite*>& GetPlayerBuildingSites(unsigned playerId) const { return gwb.GetPlayer(playerId).GetBuildingSites(); }

    /// Return a list to buildings of a given type
    const std::list<nobUsual*>& GetBuildings(const BuildingType type) const { return player_.GetBuildings(type); }
    const std::list<nobUsual*>& GetPlayerBuildings(const BuildingType type, unsigned playerId) const
    {
        return gwb.GetPlayer(playerId).GetBuildings(type);
    }

    // Return a list containing all military buildings
    const std::list<nobMilitary*>& GetMilitaryBuildings() const { return player_.GetMilitaryBuildings(); }

    /// Return a list containing all harbors
    const std::list<nobHarborBuilding*>& GetHarbors() const { return player_.GetHarbors(); }

    /// Return a list containing all storehouses and harbors and the hq
    const std::list<nobBaseWarehouse*>& GetStorehouses() const { return player_.GetStorehouses(); }

    /// Retrieves the current counts of all buildings
    BuildingCount GetBuildingCount() const;

    /// Return the inventory of the AI player
    const Inventory& GetInventory() const { return player_.GetInventory(); }

    /// Return the number of ships
    unsigned GetShipCount() const { return player_.GetShipCount(); }

    /// Return the list of ships
    const std::vector<noShip*>& GetShips() const { return player_.GetShips(); }

    /// returns distance
    unsigned CalcDistance(MapPoint p1, MapPoint p2) { return gwb.CalcDistance(p1, p2); }

    /// Return the ID of a given ship
    unsigned GetShipID(const noShip* ship) const { return player_.GetShipID(ship); }

    /// Test whether there is a possibility to start a expedition in a given direction from a given position, assuming a given starting
    /// harbor
    bool IsExplorationDirectionPossible(const MapPoint pt, const nobHarborBuilding* originHarbor, ShipDirection direction) const;

    /// Test whether there is a possibility to start a expedition in a given direction from a given position, assuming a given starting
    /// harbor
    bool IsExplorationDirectionPossible(const MapPoint pt, unsigned originHarborID, ShipDirection direction) const;

    void SetCoinsAllowed(const nobMilitary* building, const bool enabled);
    using GameCommandFactory::SetCoinsAllowed;

    unsigned GetNation() { return player_.nation; }

    void StartExpedition(const nobHarborBuilding* harbor);
    using GameCommandFactory::StartExpedition;

    /// Lets a ship found a colony
    void FoundColony(const noShip* ship) { FoundColony(GetShipID(ship)); }
    using GameCommandFactory::FoundColony;

    void TravelToNextSpot(ShipDirection direction, const noShip* ship) { TravelToNextSpot(direction, GetShipID(ship)); }
    using GameCommandFactory::TravelToNextSpot;

    void CancelExpedition(const noShip* ship) { CancelExpedition(GetShipID(ship)); }
    using GameCommandFactory::CancelExpedition;

    void ToggleShipYardMode(const nobShipYard* yard);
    using GameCommandFactory::ToggleShipYardMode;

    void DestroyBuilding(const noBuilding* building);
    using GameCommandFactory::DestroyBuilding;

    void DestroyFlag(const noFlag* flag);
    using GameCommandFactory::DestroyFlag;

    void CallGeologist(const noFlag* flag);
    using GameCommandFactory::CallGeologist;
};

#endif // AIINTERFACE_H_
