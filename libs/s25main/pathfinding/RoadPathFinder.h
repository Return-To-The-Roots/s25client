// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
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

#ifndef RoadPathFinder_h__
#define RoadPathFinder_h__

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
    /// @param wareMode True when path will be used by a ware (Allow boat roads and check for faster roads when road points have already
    /// many wares)
    /// @param max Maximum costs allowed (Usually makes pathfinding faster)
    /// @param forbidden RoadSegment that will be ignored
    /// @param length If != nullptr will receive the final costs
    /// @param firstDir If != nullptr will receive the first direction to travel
    /// @param firstNodePos If != nullptr will receive the position of the first node
    bool FindPath(const noRoadNode& start, const noRoadNode& goal, bool wareMode, unsigned max = std::numeric_limits<unsigned>::max(),
                  const RoadSegment* forbidden = nullptr, unsigned* length = nullptr, RoadPathDirection* firstDir = nullptr,
                  MapPoint* firstNodePos = nullptr);

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
                      T_SegmentConstraints isSegmentAllowed, unsigned* length = nullptr, RoadPathDirection* firstDir = nullptr,
                      MapPoint* firstNodePos = nullptr);
};

#endif // RoadPathFinder_h__
