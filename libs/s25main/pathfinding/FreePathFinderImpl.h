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

#ifndef FreePathFinderImpl_h__
#define FreePathFinderImpl_h__

#include "EventManager.h"
#include "pathfinding/FreePathFinder.h"
#include "pathfinding/NewNode.h"
#include "pathfinding/OpenListBinaryHeap.h"
#include "pathfinding/OpenListPrioQueue.h"
#include "pathfinding/PathfindingPoint.h"
#include "world/GameWorldBase.h"

typedef std::vector<FreePathNode> FreePathNodes;
extern FreePathNodes fpNodes;

struct NodePtrCmpGreater
{
    bool operator()(const FreePathNode* const lhs, const FreePathNode* const rhs) const
    {
        if(lhs->estimatedDistance == rhs->estimatedDistance)
        {
            // Enforce strictly monotonic increasing order
            return (lhs->idx > rhs->idx);
        }

        return (lhs->estimatedDistance > rhs->estimatedDistance);
    }
};

struct GetEstimatedDistance
{
    unsigned operator()(const FreePathNode& lhs) const { return lhs.estimatedDistance; }
};

// typedef OpenListPrioQueue<NewNode2*, NodePtrCmpGreater> QueueImpl;
typedef OpenListBinaryHeap<FreePathNode, GetEstimatedDistance> QueueImpl;

template<class TNodeChecker>
bool FreePathFinder::FindPath(const MapPoint start, const MapPoint dest, bool randomRoute, unsigned maxLength,
                              std::vector<Direction>* route, unsigned* length, Direction* firstDir, const TNodeChecker& nodeChecker)
{
    RTTR_Assert(start != dest);

    // increase currentVisit, so we don't have to clear the visited-states at every run
    IncreaseCurrentVisit();

    QueueImpl todo;
    const unsigned startId = gwb_.GetIdx(start);
    const unsigned destId = gwb_.GetIdx(dest);
    FreePathNode& startNode = fpNodes[startId];
    FreePathNode& destNode = fpNodes[destId];

    // Anfangsknoten einfügen Und mit entsprechenden Werten füllen
    startNode.targetDistance = gwb_.CalcDistance(start, dest);
    startNode.estimatedDistance = startNode.targetDistance;
    startNode.lastVisited = currentVisit;
    startNode.prev = nullptr;
    startNode.curDistance = 0;

    todo.push(&startNode);

    // Bei Zufälliger Richtung anfangen (damit man nicht immer denselben Weg geht, besonders für die Soldaten wichtig)
    // TODO confirm random: RANDOM.Rand(__FILE__, __LINE__, y_start * GetWidth() + x_start, 6);
    const unsigned startDir = randomRoute ? (gwb_.GetIdx(start)) * gwb_.GetEvMgr().GetCurrentGF() % 6 : 0;

    while(!todo.empty())
    {
        // Knoten mit den geringsten Wegkosten auswählen
        FreePathNode& best = *todo.pop();

        // Ziel schon erreicht?
        if(&best == &destNode)
        {
            // Ziel erreicht!
            // Jeweils die einzelnen Angaben zurückgeben, falls gewünscht (Pointer übergeben)
            if(length)
                *length = best.curDistance;
            if(route)
                route->resize(best.curDistance);

            FreePathNode* curNode = &best;
            // Route rekonstruieren und ggf. die erste Richtung speichern, falls gewünscht
            for(unsigned z = best.curDistance; z > 0; --z)
            {
                if(route)
                    (*route)[z - 1] = curNode->dir;
                if(firstDir && z == 1)
                    *firstDir = curNode->dir;
                curNode = curNode->prev;
            }

            RTTR_Assert(curNode == &startNode);

            // Fertig, es wurde ein Pfad gefunden
            return true;
        }

        // Maximaler Weg schon erreicht? In dem Fall brauchen wir keine weiteren Knoten von diesem aus bilden
        if(best.curDistance >= maxLength)
            continue;

        // Knoten in alle 6 Richtungen bilden
        for(unsigned z = startDir; z < startDir + 6; ++z)
        {
            Direction dir(z);

            // Koordinaten des entsprechenden umliegenden Punktes bilden
            MapPoint neighbourPos = gwb_.GetNeighbour(best.mapPt, dir);

            // ID des umliegenden Knotens bilden
            unsigned nbId = gwb_.GetIdx(neighbourPos);
            FreePathNode& neighbour = fpNodes[nbId];

            // Don't try to go back where we came from (would also bail out in the conditions below)
            if(best.prev == &neighbour)
                continue;

            // Knoten schon auf dem Feld gebildet?
            if(neighbour.lastVisited == currentVisit)
            {
                // Dann nur ggf. Weg und Vorgänger korrigieren, falls der Weg kürzer ist
                if(best.curDistance + 1 < neighbour.curDistance)
                {
                    // Check if we can use this transition
                    if(!nodeChecker.IsEdgeOk(best.mapPt, dir))
                        continue;

                    neighbour.curDistance = best.curDistance + 1;
                    neighbour.estimatedDistance = neighbour.curDistance + neighbour.targetDistance;
                    neighbour.prev = &best;
                    neighbour.dir = dir;
                    todo.rearrange(&neighbour);
                }
            } else
            {
                // Check node for all but the goal (goal is assumed to be ok)
                if(&neighbour != &destNode)
                {
                    if(!nodeChecker.IsNodeOk(neighbourPos))
                        continue;
                }

                // Check if we can use this transition
                if(!nodeChecker.IsEdgeOk(best.mapPt, dir))
                    continue;

                // Alles in Ordnung, Knoten kann gebildet werden
                neighbour.lastVisited = currentVisit;
                neighbour.curDistance = best.curDistance + 1;
                neighbour.targetDistance = gwb_.CalcDistance(neighbourPos, dest);
                neighbour.estimatedDistance = neighbour.curDistance + neighbour.targetDistance;
                neighbour.dir = dir;
                neighbour.prev = &best;

                todo.push(&neighbour);
            }
        }
    }

    // Liste leer und kein Ziel erreicht --> kein Weg
    return false;
}

/// Ermittelt, ob eine freie Route noch passierbar ist und gibt den Endpunkt der Route zurück
template<class TNodeChecker>
bool FreePathFinder::CheckRoute(const MapPoint start, const std::vector<Direction>& route, unsigned pos, const TNodeChecker& nodeChecker,
                                MapPoint* dest) const
{
    RTTR_Assert(pos < route.size());

    MapPoint curPt(start);
    // Check all but last step
    unsigned sizeM1 = route.size() - 1;
    for(unsigned i = pos; i < sizeM1; ++i)
    {
        if(!nodeChecker.IsEdgeOk(curPt, route[i]))
            return false;
        curPt = gwb_.GetNeighbour(curPt, route[i]);
        if(!nodeChecker.IsNodeOk(curPt))
            return false;
    }

    // Last step
    if(!nodeChecker.IsEdgeOk(curPt, route.back()))
        return false;

    if(dest)
        *dest = gwb_.GetNeighbour(curPt, route.back());

    return true;
}

#endif // FreePathFinderImpl_h__
