// Copyright (c) 2017- 2017 Settlers Freaks (sf-team at siedler25.org)
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
std::vector<Direction> FindPathForRoad(const T_WorldOrViewer& world, const MapPoint startPt, const MapPoint endPt, bool isBoatRoad,
                                       unsigned maxLen)
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
