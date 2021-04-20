// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "gameTypes/MapCoordinates.h"
#include "gameTypes/RoadPathDirection.h"
#include <limits>

class GameWorldBase;
class noRoadNode;
class RoadSegment;

class RoadPathFinder
{
    GameWorldBase& gwb_;
    unsigned currentVisit;

public:
    RoadPathFinder(GameWorldBase& gwb) : gwb_(gwb), currentVisit(0) {}

    /// Calculates the best path from start to goal
    /// Outputs are only valid if true is returned!
    /// Direction might additionally be boost::none or SHIP_DIR
    ///
    /// @param wareMode True when path will be used by a ware (Allow boat roads and check for faster roads when road
    /// points have already many wares)
    /// @param max Maximum costs allowed (Usually makes pathfinding faster)
    /// @param forbidden RoadSegment that will be ignored
    /// @param length If != nullptr will receive the final costs
    /// @param firstDir If != nullptr will receive the first direction to travel
    /// @param firstNodePos If != nullptr will receive the position of the first node
    bool FindPath(const noRoadNode& start, const noRoadNode& goal, bool wareMode,
                  unsigned max = std::numeric_limits<unsigned>::max(), const RoadSegment* forbidden = nullptr,
                  unsigned* length = nullptr, RoadPathDirection* firstDir = nullptr, MapPoint* firstNodePos = nullptr);

    /// Checks if there is ANY path from start to goal
    ///
    /// @param allowWaterRoads True to allow boat roads (mostly: Ware=true, Person=false)
    /// @param max Maximum costs allowed (Usually makes pathfinding faster)
    /// @param forbidden RoadSegment that will be ignored
    bool PathExists(const noRoadNode& start, const noRoadNode& goal, bool allowWaterRoads,
                    unsigned max = std::numeric_limits<unsigned>::max(), const RoadSegment* forbidden = nullptr);

private:
    template<class T_AdditionalCosts, class T_SegmentConstraints>
    bool FindPathImpl(const noRoadNode& start, const noRoadNode& goal, unsigned max, T_AdditionalCosts addCosts,
                      T_SegmentConstraints isSegmentAllowed, unsigned* length = nullptr,
                      RoadPathDirection* firstDir = nullptr, MapPoint* firstNodePos = nullptr);
};
