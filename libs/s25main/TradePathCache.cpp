// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
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

#include "TradePathCache.h"
#include "EventManager.h"
#include "GamePlayer.h"
#include "world/GameWorldGame.h"
#include "gameData/GameConsts.h"

bool TradePathCache::PathExists(const GameWorldGame& gwg, const MapPoint& start, const MapPoint& goal, const unsigned char player)
{
    RTTR_Assert(start != goal);

    unsigned entryIdx = FindEntry(gwg, start, goal, player);
    if(entryIdx < curSize)
    {
        // Found an entry --> Check if the route is still valid
        MapPoint checkedGoal;
        if(gwg.CheckTradeRoute(pathes[entryIdx].path.start, pathes[entryIdx].path.route, 0, player, &checkedGoal))
        {
            RTTR_Assert(checkedGoal == start || checkedGoal == goal);
            pathes[entryIdx].lastUse = gwg.GetEvMgr().GetCurrentGF();
            return true;
        } else
        {
            // TradePath is now invalid -> remove it
            curSize--;
            if(entryIdx != curSize)
                pathes[entryIdx] = pathes[curSize];
        }
    }

    TradePath path;
    if(!gwg.FindTradePath(start, goal, player, std::numeric_limits<unsigned>::max(), false, &path.route))
        return false;

    path.start = start;
    path.goal = goal;

    AddEntry(gwg, path, player);
    return true;
}

unsigned TradePathCache::FindEntry(const GameWorldGame& gwg, const MapPoint& start, const MapPoint& goal, const unsigned char player) const
{
    const GamePlayer& thisPlayer = gwg.GetPlayer(player);

    for(unsigned i = 0; i < curSize; i++)
    {
        const Entry& pathEntry = pathes[i];
        if(pathEntry.path.start != start && pathEntry.path.goal != start)
            continue;
        if(pathEntry.path.goal != goal && pathEntry.path.start != goal)
            continue;
        if(!thisPlayer.IsAlly(pathEntry.player))
            continue;
        return i;
    }
    return curSize;
}

void TradePathCache::AddEntry(const GameWorldGame& gwg, const TradePath& path, const unsigned char player)
{
    // Find the idx of the new entry
    // First look for an existing entry
    unsigned idx = FindEntry(gwg, path.start, path.goal, player);
    if(idx >= curSize)
    {
        // None found --> Find a new spot
        if(curSize < pathes.size())
            idx = curSize++; // We got space left --> append
        else
        {
            // No space left --> Replace oldest
            idx = 0;
            unsigned minLastUse = pathes[0].lastUse;
            for(unsigned i = 1; i < curSize; i++)
            {
                if(pathes[i].lastUse < minLastUse)
                {
                    minLastUse = pathes[i].lastUse;
                    idx = i;
                }
            }
        }
    }

    pathes[idx].player = player;
    pathes[idx].lastUse = gwg.GetEvMgr().GetCurrentGF();
    pathes[idx].path = path;
}
