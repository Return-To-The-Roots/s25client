// Copyright (c) 2016 Settlers Freaks (sf-team at siedler25.org)
//
// This file is part of Return To The Roots.
//
// Return To The Roots is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation,  either version 2 of the License,  or
// (at your option) any later version.
//
// Return To The Roots is distributed in the hope that it will be useful, 
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Return To The Roots. If not,  see <http://www.gnu.org/licenses/>.

#ifndef FindRoad_h__
#define FindRoad_h__

#include "pathfinding/FreePathFinderImpl.h"
#include "world/GameWorldBase.h"
#include <vector>

namespace detail{
    /// Default functor for FindRoadCondition that calls IsRoadAvailable
    template<class T_WorldOrView>
    struct DefaultFindRoadCondition
    {
        const T_WorldOrView& world;
        const bool isBoatRoad;

        DefaultFindRoadCondition(const T_WorldOrView& world, const bool isBoatRoad): world(world), isBoatRoad(isBoatRoad){}

        FORCE_INLINE bool operator()(const MapPoint& pt) const
        {
            return world.IsRoadAvailable(isBoatRoad, pt);
        }
    };

    /// Class that can be passed as a NodeChecker to FreePathFinder::FindPath and
    /// is specific to finding roads (only IsNodeOk needs to be specified with a simple functor)
    template<class T_NodeCondition>
    struct FindRoadCondition
    {
        const T_NodeCondition condition;

        FindRoadCondition(const T_NodeCondition& condition): condition(condition){}

        // Called for every node but the start & goal and should return true, if this point is usable
        FORCE_INLINE bool IsNodeOk(const MapPoint& pt) const
        {
            return condition(pt);
        }

        // Called for every edge (node to other node)
        FORCE_INLINE bool IsEdgeOk(const MapPoint& /*fromPt*/, const unsigned char /*dir*/) const
        {
            return true;
        }
    };

}

/// Wrapper for finding a path for a road
/// Takes a bool functor for checking nodes
/// Returns the road directions as a vector or an empty vector if no possible road was found
template<class T_NodeCondition>
inline std::vector<unsigned char> FindPathForRoad(const GameWorldBase& world, const MapPoint startPt, const MapPoint endPt,
                                                  const T_NodeCondition& nodeCondition)
{
    RTTR_Assert(startPt != endPt);
    std::vector<unsigned char> road;
    world.GetFreePathFinder().FindPath(startPt, endPt, false, 100, &road, NULL, NULL, detail::FindRoadCondition<T_NodeCondition>(nodeCondition));
    return road;
}

/// Wrapper for finding a path for a road
/// Takes a world or worldView instance that has a IsRoadAvailable function to call for each Node
/// Returns the road directions as a vector or an empty vector if no possible road was found
template<class T_WorldOrView>
inline std::vector<unsigned char> FindPathForRoad(const GameWorldBase& world, const MapPoint startPt, const MapPoint endPt,
                                                  const T_WorldOrView& worldOrView, const bool isBoatRoad)
{
    return FindPathForRoad(world, startPt, endPt, detail::DefaultFindRoadCondition<T_WorldOrView>(worldOrView, isBoatRoad));
}

#endif // FindRoad_h__
