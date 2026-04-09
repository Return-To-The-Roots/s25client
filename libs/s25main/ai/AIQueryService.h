// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "GamePlayer.h"
#include "ai/AIResource.h"
#include "helpers/OptionalEnum.h"
#include "world/GameWorldBase.h"
#include "gameTypes/BuildingQuality.h"
#include "gameTypes/BuildingType.h"
#include "gameTypes/Direction.h"
#include "gameTypes/ShipDirection.h"
#include <list>
#include <vector>

class Inventory;
class RoadSegment;
class noBuildingSite;
class noRoadNode;
class noShip;
class nobBaseWarehouse;
class nobHQ;
class nobHarborBuilding;
class nobMilitary;
class nobUsual;

class AIQueryService
{
public:
    AIQueryService(const GameWorldBase& gwb, unsigned char playerID);
    ~AIQueryService();

    unsigned char GetPlayerId() const { return playerID_; }
    unsigned GetNumPlayers() const { return gwb.GetNumPlayers(); }
    const std::vector<unsigned>& getUsableHarbors() const { return usableHarbors_; }

    bool IsDefeated() const { return player_.IsDefeated(); }
    AISubSurfaceResource GetSubsurfaceResource(MapPoint pt) const;
    AISurfaceResource GetSurfaceResource(MapPoint pt) const;
    int CalcResourceValue(MapPoint pt, AIResource res, helpers::OptionalEnum<Direction> direction = boost::none,
                          int lastval = 0xffff) const;
    int GetResourceRating(MapPoint pt, AIResource res) const;
    bool IsBorder(MapPoint pt) const;
    bool IsOwnTerritory(MapPoint pt) const;
    bool IsRoad(MapPoint pt, Direction dir) const;
    bool IsObjectTypeOnNode(MapPoint pt, NodalObjectType objectType) const;
    bool IsBuildingOnNode(MapPoint pt, BuildingType bld) const;
    bool IsVisible(MapPoint pt) const;
    bool CalcBQSumDifference(MapPoint pt1, MapPoint pt2) const;
    BuildingQuality GetBuildingQuality(MapPoint pt) const;
    BuildingQuality GetBuildingQualityAnyOwner(MapPoint pt) const;
    unsigned EstimateBuildLocationBQPenalty(MapPoint buildingPos) const;
    unsigned EstimateRoadRouteBQPenalty(MapPoint start, const std::vector<Direction>& route) const;
    bool FindFreePathForNewRoad(MapPoint start, MapPoint target, std::vector<Direction>* route = nullptr,
                                unsigned* length = nullptr) const;
    bool FindPathOnRoads(const noRoadNode& start, const noRoadNode& target, unsigned* length = nullptr) const;
    bool CanBuildCatapult() const;
    bool CanBuildBuildingtype(BuildingType bt) const;
    bool IsPlayerAttackable(unsigned char playerID) const;

    template<class T_IsWarehouseGood>
    nobBaseWarehouse* FindWarehouse(const noRoadNode& start, const T_IsWarehouseGood& isWarehouseGood, bool to_wh,
                                    bool use_boat_roads, unsigned* length = nullptr,
                                    const RoadSegment* forbidden = nullptr) const
    {
        return player_.FindWarehouse(start, isWarehouseGood, to_wh, use_boat_roads, length, forbidden);
    }

    const nobHQ* GetHeadquarter() const;
    const std::list<noBuildingSite*>& GetBuildingSites() const;
    const std::list<noBuildingSite*>& GetPlayerBuildingSites(unsigned playerId) const;
    const std::list<nobUsual*>& GetBuildings(BuildingType type) const;
    const std::list<nobUsual*>& GetPlayerBuildings(BuildingType type, unsigned playerId) const;
    const std::list<nobMilitary*>& GetMilitaryBuildings() const;
    const std::list<nobHarborBuilding*>& GetHarbors() const;
    const std::list<nobBaseWarehouse*>& GetStorehouses() const;
    bool isBuildingNearby(BuildingType bldType, MapPoint pt, unsigned maxDistance) const;
    bool isHarborPosClose(MapPoint pt, unsigned maxDistance, bool onlyEmpty = false) const;
    const Inventory& GetInventory() const;
    unsigned GetNumShips() const;
    const std::vector<noShip*>& GetShips() const;
    unsigned GetShipID(const noShip* ship) const;
    bool IsExplorationDirectionPossible(MapPoint pt, const nobHarborBuilding* originHarbor,
                                        ShipDirection direction) const;
    bool IsExplorationDirectionPossible(MapPoint pt, unsigned originHarborID, ShipDirection direction) const;
    Nation GetNation() const;

private:
    void InitUsableHarbors();

    const GameWorldBase& gwb;
    const GamePlayer& player_;
    const unsigned char playerID_;
    std::vector<unsigned> usableHarbors_;
};
