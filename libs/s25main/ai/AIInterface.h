// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "ai/AICommandSink.h"
#include "ai/AIQueryService.h"

class AIInterface : public AICommandSink
{
public:
    AIInterface(const GameWorldBase& gwb, std::vector<gc::GameCommandPtr>& gcs, unsigned char playerID);
    ~AIInterface() override;

    // Role-based accessors
    AIQueryService& Queries() { return queryService_; }
    const AIQueryService& Queries() const { return queryService_; }
    AICommandSink& Commands() { return *this; }
    const AICommandSink& Commands() const { return *this; }

    // Core context
    unsigned char GetPlayerId() const { return queryService_.GetPlayerId(); }
    unsigned GetNumPlayers() const { return queryService_.GetNumPlayers(); }
    const std::vector<unsigned>& getUsableHarbors() const { return queryService_.getUsableHarbors(); }
    bool IsDefeated() const { return queryService_.IsDefeated(); }

    // World, resource, and visibility queries
    AISubSurfaceResource GetSubsurfaceResource(MapPoint pt) const { return queryService_.GetSubsurfaceResource(pt); }
    AISurfaceResource GetSurfaceResource(MapPoint pt) const { return queryService_.GetSurfaceResource(pt); }
    int CalcResourceValue(MapPoint pt, AIResource res, helpers::OptionalEnum<Direction> direction = boost::none,
                          int lastval = 0xffff) const
    {
        return queryService_.CalcResourceValue(pt, res, direction, lastval);
    }
    int GetResourceRating(MapPoint pt, AIResource res) const { return queryService_.GetResourceRating(pt, res); }
    bool IsBorder(const MapPoint pt) const { return queryService_.IsBorder(pt); }
    bool IsOwnTerritory(const MapPoint pt) const { return queryService_.IsOwnTerritory(pt); }
    bool IsRoad(const MapPoint pt, Direction dir) const { return queryService_.IsRoad(pt, dir); }
    bool IsObjectTypeOnNode(const MapPoint pt, NodalObjectType objectType) const
    {
        return queryService_.IsObjectTypeOnNode(pt, objectType);
    }
    bool IsBuildingOnNode(const MapPoint pt, BuildingType bld) const
    {
        return queryService_.IsBuildingOnNode(pt, bld);
    }
    bool IsVisible(const MapPoint pt) const { return queryService_.IsVisible(pt); }
    bool CalcBQSumDifference(MapPoint pt1, MapPoint pt2) const { return queryService_.CalcBQSumDifference(pt1, pt2); }
    BuildingQuality GetBuildingQuality(MapPoint pt) const { return queryService_.GetBuildingQuality(pt); }
    BuildingQuality GetBuildingQualityAnyOwner(MapPoint pt) const
    {
        return queryService_.GetBuildingQualityAnyOwner(pt);
    }
    bool FindFreePathForNewRoad(MapPoint start, MapPoint target, std::vector<Direction>* route = nullptr,
                                unsigned* length = nullptr) const
    {
        return queryService_.FindFreePathForNewRoad(start, target, route, length);
    }
    bool FindPathOnRoads(const noRoadNode& start, const noRoadNode& target, unsigned* length = nullptr) const
    {
        return queryService_.FindPathOnRoads(start, target, length);
    }
    bool CanBuildCatapult() const { return queryService_.CanBuildCatapult(); }
    bool CanBuildBuildingtype(BuildingType bt) const { return queryService_.CanBuildBuildingtype(bt); }
    bool IsPlayerAttackable(unsigned char playerID) const { return queryService_.IsPlayerAttackable(playerID); }

    // Building, warehouse, and player-state lookups
    template<class T_IsWarehouseGood>
    nobBaseWarehouse* FindWarehouse(const noRoadNode& start, const T_IsWarehouseGood& isWarehouseGood, bool to_wh,
                                    bool use_boat_roads, unsigned* length = nullptr,
                                    const RoadSegment* forbidden = nullptr) const
    {
        return queryService_.FindWarehouse(start, isWarehouseGood, to_wh, use_boat_roads, length, forbidden);
    }

    const nobHQ* GetHeadquarter() const { return queryService_.GetHeadquarter(); }
    const std::list<noBuildingSite*>& GetBuildingSites() const { return queryService_.GetBuildingSites(); }
    const std::list<noBuildingSite*>& GetPlayerBuildingSites(unsigned playerId) const
    {
        return queryService_.GetPlayerBuildingSites(playerId);
    }
    const std::list<nobUsual*>& GetBuildings(const BuildingType type) const { return queryService_.GetBuildings(type); }
    const std::list<nobUsual*>& GetPlayerBuildings(const BuildingType type, unsigned playerId) const
    {
        return queryService_.GetPlayerBuildings(type, playerId);
    }
    const std::list<nobMilitary*>& GetMilitaryBuildings() const { return queryService_.GetMilitaryBuildings(); }
    const std::list<nobHarborBuilding*>& GetHarbors() const { return queryService_.GetHarbors(); }
    const std::list<nobBaseWarehouse*>& GetStorehouses() const { return queryService_.GetStorehouses(); }
    bool isBuildingNearby(BuildingType bldType, MapPoint pt, unsigned maxDistance) const
    {
        return queryService_.isBuildingNearby(bldType, pt, maxDistance);
    }
    bool isHarborPosClose(MapPoint pt, unsigned maxDistance, bool onlyEmpty = false) const
    {
        return queryService_.isHarborPosClose(pt, maxDistance, onlyEmpty);
    }
    const Inventory& GetInventory() const { return queryService_.GetInventory(); }
    unsigned GetNumShips() const { return queryService_.GetNumShips(); }
    const std::vector<noShip*>& GetShips() const { return queryService_.GetShips(); }
    unsigned GetShipID(const noShip* ship) const { return queryService_.GetShipID(ship); }

    // Harbor and expedition feasibility queries
    bool IsExplorationDirectionPossible(MapPoint pt, const nobHarborBuilding* originHarbor,
                                        ShipDirection direction) const
    {
        return queryService_.IsExplorationDirectionPossible(pt, originHarbor, direction);
    }
    bool IsExplorationDirectionPossible(MapPoint pt, unsigned originHarborID, ShipDirection direction) const
    {
        return queryService_.IsExplorationDirectionPossible(pt, originHarborID, direction);
    }
    Nation GetNation() const { return queryService_.GetNation(); }

    // Shared world context
    const GameWorldBase& gwb;

private:
    AIQueryService queryService_;
};
