// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "helpers/OptionalEnum.h"
#include "gameTypes/Direction.h"
#include "gameTypes/MapCoordinates.h"
#include <vector>

class noFlag;

namespace AIJH {

class AIPlayerJH;

class AIRoadController
{
public:
    explicit AIRoadController(AIPlayerJH& owner);

    void HandleRoadConstructionComplete(MapPoint pt, Direction dir);
    void HandleRoadConstructionFailed(MapPoint pt, Direction dir);
    bool IsFlagPartofCircle(const noFlag& startFlag, unsigned maxlen, const noFlag& curFlag,
                            helpers::OptionalEnum<Direction> excludeDir, std::vector<const noFlag*> oldFlags);
    void RemoveAllUnusedRoads(MapPoint pt);
    void CheckForUnconnectedBuildingSites();
    bool RemoveUnusedRoad(const noFlag& startFlag, helpers::OptionalEnum<Direction> excludeDir, bool firstflag = true,
                          bool allowcircle = true, bool keepstartflag = false);

private:
    AIPlayerJH& owner_;
};

} // namespace AIJH
