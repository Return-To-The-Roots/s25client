// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "FindPathReachable.h"
#include "pathfinding/FreePathFinderImpl.h"
#include "pathfinding/PathConditionReachable.h"
#include "world/GameWorldBase.h"
#include "nodeObjs/noBase.h"

namespace {

// Keep this narrower than PathConditionHuman, but reject permanently blocking static objects:
// Human pathing can never accept them and they cannot be removed.
struct PathConditionReachableWithStaticBlockers : PathConditionReachable
{
    PathConditionReachableWithStaticBlockers(const World& world) : PathConditionReachable(world) {}

    BOOST_FORCEINLINE bool isNodeOk(const MapPoint& pt) const
    {
        const auto* obj = world.getNode(pt).obj;
        if(obj && obj->getGOT() == GO_Type::Staticobject && obj->getBM() != BlockingManner::None)
            return false;

        return PathConditionReachable::isNodeOk(pt);
    }
};

} // namespace

bool doesReachablePathExist(const GameWorldBase& world, const MapPoint startPt, const MapPoint endPt, unsigned maxLen)
{
    RTTR_Assert(startPt != endPt);
    return world.getFreePathFinder().findPath(startPt, endPt, false, maxLen, nullptr, nullptr, nullptr,
                                              PathConditionReachableWithStaticBlockers(world));
}
