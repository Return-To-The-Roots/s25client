// Copyright (c) 2005 - 2015 Settlers Freaks (sf-team at siedler25.org)
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

#include "pathfinding/FreePathFinder.h"
#include "pathfinding/PathfindingPoint.h"
#include "pathfinding/NewNode.h"
#include "pathfinding/OpenListPrioQueue.h"
#include "pathfinding/OpenListBinaryHeap.h"

typedef std::vector<FreePathNode> FreePathNodes;
extern FreePathNodes fpNodes;

struct NodePtrCmpGreater
{
    bool operator()(const FreePathNode* const lhs, const FreePathNode* const rhs) const
    {
        if (lhs->estimatedDistance == rhs->estimatedDistance)
        {
            // Enforce strictly monotonic increasing order
            return (lhs->idx > rhs->idx);
        }

        return (lhs->estimatedDistance > rhs->estimatedDistance);
    }
};

struct GetEstimatedDistance
{
    unsigned operator()(const FreePathNode& lhs) const
    {
        return lhs.estimatedDistance;
    }
};

//typedef OpenListPrioQueue<NewNode2*, NodePtrCmpGreater> QueueImpl;
typedef OpenListBinaryHeap<FreePathNode, GetEstimatedDistance> QueueImpl;

template<class TNodeChecker>
bool FreePathFinder::FindPath(const MapPoint start, const MapPoint dest,
              const bool randomRoute, const unsigned maxLength,
              std::vector<unsigned char> * route, unsigned* length, unsigned char* firstDir,
              const TNodeChecker& nodeChecker,
              const bool record)
{
    assert(start != dest);

    // increase currentVisit, so we don't have to clear the visited-states at every run
    IncreaseCurrentVisit();

    QueueImpl todo;
    const unsigned startId = gwb_.GetIdx(start);
    const unsigned destId  = gwb_.GetIdx(dest);
    FreePathNode& startNode = fpNodes[startId];
    FreePathNode& destNode  = fpNodes[destId];

    // Anfangsknoten einfügen Und mit entsprechenden Werten füllen
    startNode.targetDistance = gwb_.CalcDistance(start, dest);
    startNode.estimatedDistance = startNode.targetDistance;
    startNode.lastVisited = currentVisit;
    startNode.prev = NULL;
    startNode.curDistance = 0;
    startNode.dir = 0;

    todo.push(&startNode);

    // Bei Zufälliger Richtung anfangen (damit man nicht immer denselben Weg geht, besonders für die Soldaten wichtig)
    // TODO confirm random: RANDOM.Rand(__FILE__, __LINE__, y_start * GetWidth() + x_start, 6);
    const unsigned startDir = randomRoute ? (gwb_.GetIdx(start)) * GAMECLIENT.GetGFNumber() % 6 : 0; 

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

            assert(curNode == &startNode);

            // Fertig, es wurde ein Pfad gefunden
            return true;
        }

        // Maximaler Weg schon erreicht? In dem Fall brauchen wir keine weiteren Knoten von diesem aus bilden
        if(best.curDistance >= maxLength)
            continue;

        // Knoten in alle 6 Richtungen bilden
        for(unsigned z = startDir; z < startDir + 6; ++z)
        {
            unsigned dir = z % 6;

            // Koordinaten des entsprechenden umliegenden Punktes bilden
            MapPoint neighbourPos = gwb_.GetNeighbour(best.mapPt, dir);

            // ID des umliegenden Knotens bilden
            unsigned nbId = gwb_.GetIdx(neighbourPos);
            FreePathNode& neighbour = fpNodes[nbId];

            // Don't try to go back where we came from (would also bail out in the conditions below)
            if(best.prev == &neighbour)
                continue;

            // Knoten schon auf dem Feld gebildet?
            if (neighbour.lastVisited == currentVisit)
            {
                // Dann nur ggf. Weg und Vorgänger korrigieren, falls der Weg kürzer ist
                if(best.curDistance + 1 < neighbour.curDistance)
                {
                    neighbour.curDistance  = best.curDistance + 1;
                    neighbour.estimatedDistance = neighbour.curDistance + neighbour.targetDistance;
                    neighbour.prev = &best;
                    neighbour.dir = dir;
                    todo.rearrange(&neighbour);
                }
            }else
            {
                // Das Ziel wollen wir auf jedenfall erreichen lassen, daher nur diese zusätzlichen
                // Bedingungen, wenn es nicht das Ziel ist
                if(&neighbour != &destNode)
                {
                    if(!nodeChecker.IsNodeOk(neighbourPos, dir))
                        continue;
                }

                // Zusätzliche Bedingungen, auch die das letzte Stück zum Ziel betreffen
                if(!nodeChecker.IsNodeToDestOk(neighbourPos, dir))
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


#endif // FreePathFinderImpl_h__