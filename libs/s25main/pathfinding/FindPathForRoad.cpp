// Copyright (C) 2017 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "FindPathForRoad.h"
#include "pathfinding/FreePathFinderImpl.h"
#include "pathfinding/PathConditionRoad.h"
#include "world/GameWorldViewer.h"

namespace {
/// Trait to get the world out of a GameWorldBase or GameWorldViewer class
template<class T_WorldOrViewer>
struct GetWorld
{
    static const GameWorldBase& get(const T_WorldOrViewer& world) { return world; }
};
template<>
struct GetWorld<GameWorldViewer>
{
    static const GameWorldBase& get(const GameWorldViewer& gwv) { return gwv.GetWorld(); }
};

template<class T_WorldOrViewer>
const GameWorldBase& getWorld(const T_WorldOrViewer& world)
{
    return GetWorld<T_WorldOrViewer>::get(world);
}
} // namespace

namespace detail {

template<class T_WorldOrViewer>
std::vector<Direction> FindPathForRoad(const T_WorldOrViewer& world, const MapPoint startPt, const MapPoint endPt,
                                       bool isBoatRoad, unsigned maxLen)
{
    RTTR_Assert(startPt != endPt);
    std::vector<Direction> road;
    getWorld(world).GetFreePathFinder().FindPath(startPt, endPt, false, maxLen, &road, nullptr, nullptr,
                                                 makePathConditionRoad(world, isBoatRoad));
    return road;
}

template std::vector<Direction> FindPathForRoad(const GameWorldBase&, const MapPoint, const MapPoint, bool, unsigned);
template std::vector<Direction> FindPathForRoad(const GameWorldViewer&, const MapPoint, const MapPoint, bool, unsigned);
} // namespace detail
