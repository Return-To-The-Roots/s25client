// Copyright (C) 2018 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "FindPathReachable.h"
#include "pathfinding/FreePathFinderImpl.h"
#include "pathfinding/PathConditionReachable.h"
#include "world/GameWorldBase.h"

bool DoesReachablePathExist(const GameWorldBase& world, const MapPoint startPt, const MapPoint endPt, unsigned maxLen)
{
    RTTR_Assert(startPt != endPt);
    return world.GetFreePathFinder().FindPath(startPt, endPt, false, maxLen, nullptr, nullptr, nullptr,
                                              PathConditionReachable(world));
}
