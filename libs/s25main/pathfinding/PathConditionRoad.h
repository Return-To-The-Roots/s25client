// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
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

#pragma once

#include <boost/config.hpp>

template<class T_WorldOrViewer>
struct PathConditionRoad
{
    const T_WorldOrViewer& worldOrViewer;
    const bool isBoatRoad;

    PathConditionRoad(const T_WorldOrViewer& worldOrViewer, bool isBoatRoad)
        : worldOrViewer(worldOrViewer), isBoatRoad(isBoatRoad)
    {}

    // Called for every node but the start & goal and should return true, if this point is usable
    BOOST_FORCEINLINE bool IsNodeOk(const MapPoint& pt) const
    {
        return worldOrViewer.IsPlayerTerritory(pt) && worldOrViewer.IsRoadAvailable(isBoatRoad, pt);
    }

    // Called for every edge (node to other node)
    BOOST_FORCEINLINE bool IsEdgeOk(const MapPoint& /*fromPt*/, const Direction /*dir*/) const { return true; }
};

template<class T_WorldOrViewer>
PathConditionRoad<T_WorldOrViewer> makePathConditionRoad(const T_WorldOrViewer& worldOrViewer, bool isBoatRoad)
{
    return PathConditionRoad<T_WorldOrViewer>(worldOrViewer, isBoatRoad);
}
