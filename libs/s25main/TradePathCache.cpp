// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "TradePathCache.h"
#include "EventManager.h"
#include "GamePlayer.h"
#include "world/GameWorld.h"
#include "gameData/GameConsts.h"

bool TradePathCache::pathExists(const MapPoint start, const MapPoint goal, const unsigned char player)
{
    RTTR_Assert(start != goal);

    int entryIdx = findEntry(start, goal, player);
    if(entryIdx >= 0)
    {
        // Found an entry --> Check if the route is still valid
        MapPoint checkedGoal;
        if(world.CheckTradeRoute(paths[entryIdx].path.start, paths[entryIdx].path.route, 0, player, &checkedGoal))
        {
            RTTR_Assert(checkedGoal == start || checkedGoal == goal);
            paths[entryIdx].lastUse = world.GetEvMgr().GetCurrentGF();
            return true;
        } else
        {
            // TradePath is now invalid -> remove it
            if(static_cast<unsigned>(entryIdx) != paths.size() - 1u)
                std::swap(paths[entryIdx], paths.back());
            paths.pop_back();
        }
    }

    std::vector<Direction> route;
    if(!world.FindTradePath(start, goal, player, std::numeric_limits<unsigned>::max(), false, &route))
        return false;

    addEntry(TradePath(start, goal, std::move(route)), player);
    return true;
}

int TradePathCache::findEntry(const MapPoint start, const MapPoint goal, const PlayerIdx player) const
{
    const GamePlayer& thisPlayer = world.GetPlayer(player);

    for(unsigned i = 0; i < paths.size(); i++)
    {
        const Entry& pathEntry = paths[i];
        if(pathEntry.path.start != start && pathEntry.path.goal != start)
            continue;
        if(pathEntry.path.goal != goal && pathEntry.path.start != goal)
            continue;
        if(!thisPlayer.IsAlly(pathEntry.player))
            continue;
        return i;
    }
    return -1;
}

void TradePathCache::addEntry(TradePath path, const unsigned char player)
{
    Entry entry{player, world.GetEvMgr().GetCurrentGF(), std::move(path)};

    int idx = findEntry(entry.path.start, entry.path.goal, player);
    if(idx < 0)
    {
        // None found --> Find a new spot
        if(paths.size() < paths.max_size())
            paths.emplace_back(std::move(entry)); // We got space left --> append
        else
        {
            // No space left --> Replace oldest
            const auto itOldestElement = std::min_element(
              paths.begin(), paths.end(), [](const Entry& rhs, const Entry& lhs) { return rhs.lastUse < lhs.lastUse; });
            *itOldestElement = std::move(entry);
        }
    } else
        paths[idx] = std::move(entry);
}
