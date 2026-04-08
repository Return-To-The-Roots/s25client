// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "gameTypes/BuildingType.h"
#include "gameTypes/MapCoordinates.h"
#include "helpers/OptionalEnum.h"

#include <memory>
#include <vector>

class noFlag;
class noShip;

namespace AIEvent {
class Base;
}

namespace AIJH {

class AIPlayerJH;

class AIEventHandler
{
public:
    explicit AIEventHandler(AIPlayerJH& owner) : owner_(owner) {}

    void ExecuteAIJob();
    void HandleNewMilitaryBuildingOccupied(MapPoint pt);
    void HandleBuildingDestroyed(MapPoint pt, BuildingType bld);
    void HandleRoadConstructionComplete(MapPoint pt, Direction dir);
    void HandleRoadConstructionFailed(MapPoint pt, Direction dir);
    void HandleMilitaryBuildingLost(MapPoint pt);
    void HandleBuildingFinished(MapPoint pt, BuildingType bld);
    void HandleNewColonyFounded(MapPoint pt);
    void HandleExpedition(const noShip* ship);
    void HandleExpedition(MapPoint pt);
    void HandleTreeChopped(MapPoint pt);
    void HandleNoMoreResourcesReachable(MapPoint pt, BuildingType bld);
    void HandleShipBuilt(MapPoint pt);
    void HandleBorderChanged(MapPoint pt);
    void HandleLostLand(MapPoint pt);
    void CheckExpeditions();
    void SendAIEvent(std::unique_ptr<AIEvent::Base> ev);
    bool IsFlagPartOfCircle(const noFlag& startFlag, unsigned maxlen, const noFlag& curFlag,
                            helpers::OptionalEnum<Direction> excludeDir, std::vector<const noFlag*> oldFlags);
    void RemoveAllUnusedRoads(MapPoint pt);
    void CheckForUnconnectedBuildingSites();
    bool RemoveUnusedRoad(const noFlag& startFlag, helpers::OptionalEnum<Direction> excludeDir, bool firstflag = true,
                          bool allowcircle = true, bool keepstartflag = false);

private:
    AIPlayerJH& owner_;
};

} // namespace AIJH
