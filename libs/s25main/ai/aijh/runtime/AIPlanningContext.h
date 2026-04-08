// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "ai/aijh/runtime/AIWorldView.h"

#include "gameTypes/JobTypes.h"

#include <vector>

namespace AIJH {

class AIConstruction;

class AIPlanningContext : public AIWorldView
{
public:
    ~AIPlanningContext() override = default;
    using AIWorldView::FindBestPosition;

    virtual AIConstruction& GetConstruction() = 0;

    virtual MapPoint FindBestPosition(BuildingType bt) = 0;
    virtual void AddBuildJob(BuildingType type, MapPoint pt, bool front = false, bool searchPosition = true) = 0;
    virtual void AddGlobalBuildJob(BuildingType type) = 0;
    virtual void RecalcGround(MapPoint buildingPos, std::vector<Direction>& route_road) = 0;
    virtual void SetFarmedNodes(MapPoint pt, bool set) = 0;
    virtual void ExecuteLuaConstructionOrder(MapPoint pt, BuildingType bt, bool forced = false) = 0;

    virtual void HandleNewMilitaryBuildingOccupied(MapPoint pt) = 0;
    virtual void HandleMilitaryBuildingLost(MapPoint pt) = 0;
    virtual void HandleBuildingDestroyed(MapPoint pt, BuildingType bld) = 0;
    virtual void HandleNoMoreResourcesReachable(MapPoint pt, BuildingType bld) = 0;
    virtual void HandleShipBuilt(MapPoint pt) = 0;
    virtual void HandleRoadConstructionComplete(MapPoint pt, Direction dir) = 0;
    virtual void HandleRoadConstructionFailed(MapPoint pt, Direction dir) = 0;
    virtual void HandleBorderChanged(MapPoint pt) = 0;
    virtual void HandleBuildingFinished(MapPoint pt, BuildingType bld) = 0;
    virtual void HandleExpedition(MapPoint pt) = 0;
    virtual void HandleTreeChopped(MapPoint pt) = 0;
    virtual void HandleNewColonyFounded(MapPoint pt) = 0;
    virtual void HandleLostLand(MapPoint pt) = 0;

    virtual unsigned AmountInStorage(GoodType good) const = 0;
    virtual unsigned AmountInStorage(Job job) const = 0;
};

} // namespace AIJH
