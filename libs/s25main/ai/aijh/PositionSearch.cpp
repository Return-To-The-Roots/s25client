// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "PositionSearch.h"
#include "AIPlayerJH.h"
#include "gameData/BuildingConsts.h"

AIJH::PositionSearch::PositionSearch(const AIPlayerJH& player, const MapPoint pt, AIResource res, int minimum,
                                     BuildingType bld, bool searchGlobalOptimum /*= false*/)
    : startPt(pt), res(res), minimum(minimum), size(BUILDING_SIZE[bld]), bld(bld),
      searchGlobalOptimum(searchGlobalOptimum), nodesPerStep(25), // TODO: Make it depend on something...
      resultPt(MapPoint::invalid()), resultValue(0)
{
    tested.resize(prodOfComponents(player.getWorld().getSize()));

    // insert start position as first node to test
    toTest.push(pt);
    tested[player.getWorld().getIdx(pt)] = true;
}

AIJH::PositionSearchState AIJH::PositionSearch::execute(const AIPlayerJH& player)
{
    const AIResourceMap& resMap = player.getResMap(res);
    // make nodesPerStep tests
    for(int i = 0; i < nodesPerStep; i++)
    {
        // no more nodes to test? end this!
        if(toTest.empty())
            break;

        // get the node
        MapPoint pt = toTest.front();
        toTest.pop();
        const Node& node = player.getAINode(pt);

        // and test it... TODO exception at res::borderland?
        if(resMap[pt] > resultValue                        // value better
           && node.owned && node.reachable && !node.farmed // available node
           && canUseBq(node.bq, size)                      // matching size
        )
        {
            // store location & value
            resultValue = resMap[pt];
            resultPt = pt;
        }

        // now insert neighbouring nodes...
        for(const MapPoint neighbourPt : player.getWorld().getNeighbours(pt))
        {
            unsigned nIdx = player.getWorld().getIdx(neighbourPt);

            // test if already tested or not in territory
            if(!tested[nIdx] && player.getAINode(neighbourPt).owned)
            {
                toTest.push(pt);
                tested[nIdx] = true;
            }
        }
    }

    // decide the state of the search
    if(toTest.empty())
    {
        // no more nodes to test
        // fail iff not reached minimum
        if(resultValue < minimum)
            return PositionSearchState::Failed;
        else
            return PositionSearchState::Successfull;
    } else if(resultValue >= minimum && !searchGlobalOptimum)
    {
        // reached minimal satisfying value and we were not looking for the best
        return PositionSearchState::Successfull;
    } else
        return PositionSearchState::InProgress;
}
