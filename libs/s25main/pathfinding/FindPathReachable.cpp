// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "FindPathReachable.h"
#include "pathfinding/FreePathFinderImpl.h"
#include "pathfinding/PathConditionReachable.h"
#include "world/GameWorldBase.h"
#include "nodeObjs/noBase.h"

namespace {

struct PathConditionReachableWithStaticBlockers : PathConditionReachable
{
    PathConditionReachableWithStaticBlockers(const World& world) : PathConditionReachable(world) {}

    BOOST_FORCEINLINE bool IsNodeOk(const MapPoint& pt) const
    {
        const auto* obj = world.GetNode(pt).obj;
        if(obj && obj->GetGOT() == GO_Type::Staticobject && obj->GetBM() != BlockingManner::None)
            return false;

        return PathConditionReachable::IsNodeOk(pt);
    }
};

} // namespace

bool DoesReachablePathExist(const GameWorldBase& world, const MapPoint startPt, const MapPoint endPt, unsigned maxLen)
{
    RTTR_Assert(startPt != endPt);
    return world.GetFreePathFinder().FindPath(startPt, endPt, false, maxLen, nullptr, nullptr, nullptr,
                                              PathConditionReachableWithStaticBlockers(world));
}
