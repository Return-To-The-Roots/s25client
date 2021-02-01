// Copyright (c) 2005 - 2020 Settlers Freaks (sf-team at siedler25.org)
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

#include "pathfinding/FreePathFinder.h"
#include "EventManager.h"
#include "RttrForeachPt.h"
#include "helpers/containerUtils.h"
#include "pathfinding/NewNode.h"
#include "pathfinding/PathfindingPoint.h"
#include "world/GameWorldBase.h"
#include "s25util/Log.h"

//////////////////////////////////////////////////////////////////////////
/// FreePathFinder implementation
//////////////////////////////////////////////////////////////////////////

/// MapNodes
using MapNodes = std::vector<NewNode>;
using FreePathNodes = std::vector<FreePathNode>;
MapNodes nodes;
FreePathNodes fpNodes;

void FreePathFinder::Init(const MapExtent& mapSize)
{
    currentVisit = 0;
    size_ = Extent(mapSize);
    // Reset nodes
    nodes.clear();
    fpNodes.clear();
    nodes.resize(size_.x * size_.y);
    fpNodes.resize(nodes.size());
    RTTR_FOREACH_PT(MapPoint, size_)
    {
        const unsigned idx = gwb_.GetIdx(pt);
        nodes[idx].mapPt = pt;
        fpNodes[idx].lastVisited = 0;
        fpNodes[idx].mapPt = pt;
    }
}

void FreePathFinder::IncreaseCurrentVisit()
{
    // if the counter reaches its maxium, tidy up
    if(currentVisit == std::numeric_limits<unsigned>::max())
    {
        for(auto& node : nodes)
        {
            node.lastVisited = 0;
            node.lastVisitedEven = 0;
        }
        for(auto& fpNode : fpNodes)
        {
            fpNode.lastVisited = 0;
        }
        currentVisit = 1;
    } else
        currentVisit++;
}

/// Pathfinder ( A* ), O(v lg v) --> Normal terrain (ignoring roads) for road building and free walking jobs
bool FreePathFinder::FindPathAlternatingConditions(const MapPoint start, const MapPoint dest, const bool randomRoute,
                                                   const unsigned maxLength, std::vector<Direction>* route,
                                                   unsigned* length, Direction* firstDir, FP_Node_OK_Callback IsNodeOK,
                                                   FP_Node_OK_Callback IsNodeOKAlternate,
                                                   FP_Node_OK_Callback IsNodeToDestOk, const void* param)
{
    if(start == dest)
    {
        // Path where start==goal should never happen
        RTTR_Assert(false);
        LOG.write("WARNING: Bug detected (GF: %u). Please report this with the savegame and replay (Start==Dest in "
                  "pathfinding %u,%u)\n")
          % gwb_.GetEvMgr().GetCurrentGF() % unsigned(start.x) % unsigned(start.y);
        // But for now we assume it to be valid and return (kind of) correct values
        if(route)
            route->clear();
        if(length)
            *length = 0;
        if(firstDir)
            *firstDir = Direction::East;
        return true;
    }

    // increase currentVisit, so we don't have to clear the visited-states at every run
    IncreaseCurrentVisit();

    std::list<PathfindingPoint> todo;
    const unsigned destId = gwb_.GetIdx(dest);

    bool prevStepEven = true; // flips between even and odd
    unsigned stepsTilSwitch = 1;

    // Add start node
    unsigned startId = gwb_.GetIdx(start);
    todo.push_back(PathfindingPoint(startId, gwb_.CalcDistance(start, dest), 0));
    // And init it
    nodes[startId].prevEven = INVALID_PREV;
    nodes[startId].lastVisitedEven = currentVisit;
    nodes[startId].wayEven = 0;
    // LOG.write(("pf: from %i, %i to %i, %i \n", x_start, y_start, x_dest, y_dest);

    // Start at random dir (so different jobs may use different roads)
    const Direction startDir =
      randomRoute ? convertToDirection(gwb_.GetIdx(start) * gwb_.GetEvMgr().GetCurrentGF()) : Direction::West;

    while(!todo.empty())
    {
        if(!stepsTilSwitch) // counter for next step and switch condition
        {
            prevStepEven = !prevStepEven;
            stepsTilSwitch = todo.size();
            // prevstepEven ? LOG.write(("pf: even, to switch %i listsize %i ", stepsTilSwitch, todo.size()) :
            // LOG.write(("pf: odd, to switch %i listsize %i ", stepsTilSwitch, todo.size());
        }
        // else
        // prevstepEven ? LOG.write(("pf: even, to switch %i listsize %i ", stepsTilSwitch, todo.size()) :
        // LOG.write(("pf: odd, to switch %i listsize %i ", stepsTilSwitch, todo.size());
        stepsTilSwitch--;

        // Get node with lowest cost
        PathfindingPoint best = *todo.begin();
        // Knoten behandelt --> raus aus der todo Liste
        todo.erase(todo.begin());

        // printf("x: %u y: %u\n", best.x, best.y);

        // ID des besten Punktes ausrechnen

        unsigned bestId = best.id_;
        // LOG.write((" now %i, %i id: %i \n", best.x, best.y, best_id);
        // Dieser Knoten wurde aus dem set entfernt, daher wird der entsprechende Iterator
        // auf das Ende (also nicht definiert) gesetzt, quasi als "nullptr"-Ersatz
        // pf_nodes[best_id].it_p = todo.end();

        // Ziel schon erreicht?
        if(destId == bestId)
        {
            // Ziel erreicht!
            // Return the values if requested
            const unsigned routeLen = prevStepEven ? nodes[bestId].wayEven : nodes[bestId].way;
            if(length)
                *length = routeLen;
            if(route)
                route->resize(routeLen);

            // Reconstruct route and get first direction (if requested)
            bool alternate = prevStepEven;
            for(unsigned z = routeLen - 1; bestId != startId; --z)
            {
                if(route)
                    (*route)[z] = alternate ? nodes[bestId].dirEven : nodes[bestId].dir;
                if(firstDir && z == 0)
                    *firstDir = nodes[bestId].dirEven;

                bestId = alternate ? nodes[bestId].prevEven : nodes[bestId].prev;
                alternate = !alternate;
            }

            // Fertig, es wurde ein Pfad gefunden
            return true;
        }

        // Maximaler Weg schon erreicht ? In dem Fall brauchen wir keine weiteren Knoten von diesem aus bilden
        if((prevStepEven && nodes[bestId].wayEven == maxLength) || (!prevStepEven && nodes[bestId].way == maxLength))
            continue;

        // LOG.write(("pf get neighbor nodes %i, %i id: %i \n", best.x, best.y, best_id);
        // Knoten in alle 6 Richtungen bilden
        for(const auto dir : helpers::enumRange(startDir))
        {
            // Koordinaten des entsprechenden umliegenden Punktes bilden
            MapPoint neighbourPos = gwb_.GetNeighbour(nodes[bestId].mapPt, dir);

            // ID des umliegenden Knotens bilden
            unsigned nbId = gwb_.GetIdx(neighbourPos);

            // Knoten schon auf dem Feld gebildet ?
            if((prevStepEven && nodes[nbId].lastVisited == currentVisit)
               || (!prevStepEven && nodes[nbId].lastVisitedEven == currentVisit))
            {
                continue;
            }

            // Check additional constraints for non-destination points
            if(nbId != destId && ((prevStepEven && IsNodeOK) || (!prevStepEven && IsNodeOKAlternate)))
            {
                if(prevStepEven)
                {
                    if(!IsNodeOK(gwb_, neighbourPos, dir, param))
                        continue;
                } else
                {
                    if(!IsNodeOKAlternate(gwb_, neighbourPos, dir, param))
                        continue;
                    MapPoint p = nodes[bestId].mapPt;

                    std::vector<MapPoint> evenLocationsOnRoute;
                    bool alternate = false;
                    unsigned back_id = bestId;
                    for(unsigned i = nodes[bestId].way - 1; i > 1;
                        i--) // backtrack the plannend route and check if another "even" position is too close
                    {
                        Direction pdir = alternate ? nodes[back_id].dirEven : nodes[back_id].dir;
                        p = gwb_.GetNeighbour(p, pdir + 3u);
                        if(i % 2 == 0) // even step
                        {
                            evenLocationsOnRoute.push_back(p);
                        }
                        back_id = alternate ? nodes[back_id].prevEven : nodes[back_id].prev;
                        alternate = !alternate;
                    }
                    bool tooClose =
                      helpers::contains_if(evenLocationsOnRoute, [this, neighbourPos](const MapPoint& it) {
                          return gwb_.CalcDistance(neighbourPos, it) < 2;
                      });
                    if(tooClose)
                        continue;
                    if(gwb_.CalcDistance(neighbourPos, start) < 2)
                        continue;
                    if(gwb_.CalcDistance(neighbourPos, dest) < 2)
                        continue;
                }
            }

            // Conditions for all nodes
            if(IsNodeToDestOk)
            {
                if(!IsNodeToDestOk(gwb_, neighbourPos, dir, param))
                    continue;
            }

            // Alles in Ordnung, Knoten kann gebildet werden
            unsigned way;
            if(prevStepEven)
            {
                nodes[nbId].lastVisited = currentVisit;
                way = nodes[nbId].way = nodes[bestId].wayEven + 1;
                nodes[nbId].dir = dir;
                nodes[nbId].prev = bestId;
            } else
            {
                nodes[nbId].lastVisitedEven = currentVisit;
                way = nodes[nbId].wayEven = nodes[bestId].way + 1;
                nodes[nbId].dirEven = dir;
                nodes[nbId].prevEven = bestId;
            }

            todo.push_back(PathfindingPoint(nbId, gwb_.CalcDistance(neighbourPos, dest), way));
            // pf_nodes[xaid].it_p = ret.first;
        }
    }

    // Liste leer und kein Ziel erreicht --> kein Weg
    return false;
}
