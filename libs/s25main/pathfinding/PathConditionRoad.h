// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

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
    BOOST_FORCEINLINE bool isNodeOk(const MapPoint& pt) const
    {
        return worldOrViewer.isPlayerTerritory(pt) && worldOrViewer.isRoadAvailable(isBoatRoad, pt);
    }

    // Called for every edge (node to other node)
    BOOST_FORCEINLINE bool isEdgeOk(const MapPoint& /*fromPt*/, const Direction /*dir*/) const { return true; }
};

template<class T_WorldOrViewer>
PathConditionRoad<T_WorldOrViewer> makePathConditionRoad(const T_WorldOrViewer& worldOrViewer, bool isBoatRoad)
{
    return PathConditionRoad<T_WorldOrViewer>(worldOrViewer, isBoatRoad);
}
