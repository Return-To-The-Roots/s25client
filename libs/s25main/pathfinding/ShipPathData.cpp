// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "ShipPathData.h"
#include "EventManager.h"
#include "helpers/IdRange.h"
#include "helpers/containerUtils.h"
#include "pathfinding/FreePathFinder.h"
#include "pathfinding/PathConditionShip.h"
#include "world/GameWorldBase.h"
#include <algorithm>
#include <set>

struct ShipPathData::HarborConnection::Matcher
{
    HarborId harbor;
    SeaId sea;
    constexpr bool operator()(const ShipPathData::HarborConnection& con) const
    {
        return con.destHarbor == harbor && con.sea == sea;
    }
};

std::vector<Direction> ShipPathData::getHarborConnection(HarborId startHarbor, HarborId destHarbor, SeaId seaId)
{
    if(harborConnections_.empty())
        initHarborConnections();
    const auto it = helpers::find_if(harborConnections_[startHarbor], HarborConnection::Matcher{destHarbor, seaId});
    RTTR_Assert(it != harborConnections_[startHarbor].end());
    return it->route;
}

const std::vector<Direction>* ShipPathData::findCachedPath(MapPoint start, MapPoint dest, bool& reversed) const
{
    for(const PathEntry& e : cachedPaths_)
    {
        if(e.start == start && e.dest == dest)
        {
            const_cast<PathEntry&>(e).lastUse = world_.GetEvMgr().GetCurrentGF();
            reversed = false;
            return &e.route;
        }
        if(e.start == dest && e.dest == start)
        {
            const_cast<PathEntry&>(e).lastUse = world_.GetEvMgr().GetCurrentGF();
            reversed = true;
            return &e.route;
        }
    }
    return nullptr;
}

void ShipPathData::addToCache(MapPoint start, MapPoint dest, std::vector<Direction> route)
{
    PathEntry entry{start, dest, world_.GetEvMgr().GetCurrentGF(), std::move(route)};
    if(cachedPaths_.size() < cachedPaths_.max_size())
        cachedPaths_.emplace_back(std::move(entry));
    else
    {
        const auto itOldest =
          std::min_element(cachedPaths_.begin(), cachedPaths_.end(),
                           [](const PathEntry& a, const PathEntry& b) { return a.lastUse < b.lastUse; });
        *itOldest = std::move(entry);
    }
}

void ShipPathData::initHarborConnections()
{
    harborConnections_.resize(world_.GetNumHarborPoints());
    for(const auto startHbId : helpers::idRange<HarborId>(world_.GetNumHarborPoints()))
    {
        auto& curConnections = harborConnections_[startHbId];
        std::vector<HarborPos::Neighbor> neighbors;
        for(const auto dir : helpers::EnumRange<ShipDirection>{})
        {
            const auto& curNbs = world_.GetHarborNeighbors(startHbId, dir);
            neighbors.insert(neighbors.end(), curNbs.begin(), curNbs.end());
        }
        for(const auto& nb : neighbors)
        {
            RTTR_Assert(startHbId != nb.id);
            RTTR_Assert(nb.sea);
            // Connections are unique
            RTTR_Assert(!helpers::contains_if(curConnections, HarborConnection::Matcher{nb.id, nb.sea}));
            curConnections.push_back(HarborConnection{nb.id, nb.sea, {}});
            unsigned len = 0;
            const auto startPoint = world_.GetCoastalPoint(startHbId, nb.sea);
            const auto nbPoint = world_.GetCoastalPoint(nb.id, nb.sea);
            const bool found =
              (startPoint == nbPoint)
              || world_.FindShipPath(startPoint, nbPoint, nb.distance, &curConnections.back().route, &len);
            RTTR_Assert(found);
            RTTR_Assert(len == nb.distance);
        }
    }
}
