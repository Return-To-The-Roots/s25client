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

#include "PositionSearch.h"
#include "AIPlayerJH.h"
#include "gameData/BuildingConsts.h"

AIJH::PositionSearch::PositionSearch(const AIPlayerJH& player, const MapPoint pt, AIResource res, int minimum,
                                     BuildingType bld, bool searchGlobalOptimum /*= false*/)
    : startPt(pt), res(res), minimum(minimum), size(BUILDING_SIZE[bld]), bld(bld),
      searchGlobalOptimum(searchGlobalOptimum), nodesPerStep(25), // TODO: Make it depend on something...
      resultPt(MapPoint::Invalid()), resultValue(0)
{
    tested.resize(prodOfComponents(player.GetWorld().GetSize()));

    // insert start position as first node to test
    toTest.push(pt);
    tested[player.GetWorld().GetIdx(pt)] = true;
}

AIJH::PositionSearchState AIJH::PositionSearch::execute(const AIPlayerJH& player)
{
    const AIResourceMap& resMap = player.GetResMap(res);
    // make nodesPerStep tests
    for(int i = 0; i < nodesPerStep; i++)
    {
        // no more nodes to test? end this!
        if(toTest.empty())
            break;

        // get the node
        MapPoint pt = toTest.front();
        toTest.pop();
        const Node& node = player.GetAINode(pt);

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
        for(const MapPoint neighbourPt : player.GetWorld().GetNeighbours(pt))
        {
            unsigned nIdx = player.GetWorld().GetIdx(neighbourPt);

            // test if already tested or not in territory
            if(!tested[nIdx] && player.GetAINode(neighbourPt).owned)
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
