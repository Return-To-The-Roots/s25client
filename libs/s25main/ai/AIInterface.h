// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "ai/AICommandSink.h"
#include "ai/AIQueryService.h"

/// Transitional combined AI facade.
/// New code should prefer Queries() for read-only access and Commands() for command emission.
class AIInterface : public AICommandSink
{
public:
    AIInterface(const GameWorldBase& gwb, std::vector<gc::GameCommandPtr>& gcs, unsigned char playerID);
    ~AIInterface() override;

    // Primary role-based accessors
    AIQueryService& Queries() { return queryService_; }
    const AIQueryService& Queries() const { return queryService_; }
    AICommandSink& Commands() { return *this; }
    const AICommandSink& Commands() const { return *this; }

    // Minimal legacy compatibility surface for unmigrated callers
    unsigned char GetPlayerId() const { return queryService_.GetPlayerId(); }
    const std::vector<unsigned>& getUsableHarbors() const { return queryService_.getUsableHarbors(); }

    AISubSurfaceResource GetSubsurfaceResource(MapPoint pt) const { return queryService_.GetSubsurfaceResource(pt); }
    AISurfaceResource GetSurfaceResource(MapPoint pt) const { return queryService_.GetSurfaceResource(pt); }
    bool IsBorder(const MapPoint pt) const { return queryService_.IsBorder(pt); }
    bool IsOwnTerritory(const MapPoint pt) const { return queryService_.IsOwnTerritory(pt); }
    bool IsObjectTypeOnNode(const MapPoint pt, NodalObjectType objectType) const
    {
        return queryService_.IsObjectTypeOnNode(pt, objectType);
    }
    bool IsBuildingOnNode(const MapPoint pt, BuildingType bld) const
    {
        return queryService_.IsBuildingOnNode(pt, bld);
    }
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

    template<class T_IsWarehouseGood>
    nobBaseWarehouse* FindWarehouse(const noRoadNode& start, const T_IsWarehouseGood& isWarehouseGood, bool to_wh,
                                    bool use_boat_roads, unsigned* length = nullptr,
                                    const RoadSegment* forbidden = nullptr) const
    {
        return queryService_.FindWarehouse(start, isWarehouseGood, to_wh, use_boat_roads, length, forbidden);
    }

    const std::list<noBuildingSite*>& GetBuildingSites() const { return queryService_.GetBuildingSites(); }
    const std::list<nobUsual*>& GetBuildings(const BuildingType type) const { return queryService_.GetBuildings(type); }
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

    bool IsExplorationDirectionPossible(MapPoint pt, const nobHarborBuilding* originHarbor,
                                        ShipDirection direction) const
    {
        return queryService_.IsExplorationDirectionPossible(pt, originHarbor, direction);
    }
    bool IsExplorationDirectionPossible(MapPoint pt, unsigned originHarborID, ShipDirection direction) const
    {
        return queryService_.IsExplorationDirectionPossible(pt, originHarborID, direction);
    }

    // Shared world context
    const GameWorldBase& gwb;

private:
    AIQueryService queryService_;
};
