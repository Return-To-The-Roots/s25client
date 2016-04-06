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

#include "defines.h" // IWYU pragma: keep
#include "pathfinding/FreePathFinder.h"
#include "pathfinding/PathfindingPoint.h"
#include "pathfinding/NewNode.h"
#include "world/GameWorldBase.h"
#include "GameClient.h"
#include "Log.h"

// Include last!
#include "DebugNew.h" // IWYU pragma: keep

//////////////////////////////////////////////////////////////////////////
/// FreePathFinder implementation
//////////////////////////////////////////////////////////////////////////

/// MapNodes
typedef std::vector<NewNode> MapNodes;
typedef std::vector<FreePathNode> FreePathNodes;
MapNodes nodes;
FreePathNodes fpNodes;

void FreePathFinder::Init(const unsigned mapWidth, const unsigned mapHeight)
{
    currentVisit = 0;
    width_ = mapWidth;
    height_ = mapHeight;
    // Reset nodes
    nodes.clear();
    fpNodes.clear();
    nodes.resize(width_ * height_);
    fpNodes.resize(width_ * height_);
    for(unsigned y = 0; y < height_; ++y)
    {
        for(unsigned x = 0; x < width_; ++x)
        {
            const MapPoint pt = MapPoint(x, y);
            const unsigned idx = gwb_.GetIdx(pt);
            nodes[idx].mapPt = pt;
            fpNodes[idx].lastVisited = 0;
            fpNodes[idx].mapPt = pt;
            fpNodes[idx].idx = idx;
        }
    }
}

void FreePathFinder::IncreaseCurrentVisit()
{
    // if the counter reaches its maxium, tidy up
    if (currentVisit == std::numeric_limits<unsigned>::max())
    {
        for (MapNodes::iterator it = nodes.begin(); it != nodes.end(); ++it)
        {
            it->lastVisited = 0;
            it->lastVisitedEven = 0;
        }
        for (FreePathNodes::iterator it = fpNodes.begin(); it != fpNodes.end(); ++it)
        {
            it->lastVisited = 0;
        }
        currentVisit = 1;
    }else
        currentVisit++;
}

/// Wegfinden ( A* ), O(v lg v) --> Wegfindung auf allgemeinen Terrain (ohne Straßen), für Wegbau und frei herumlaufende Berufe
bool FreePathFinder::FindPathAlternatingConditions(const MapPoint start, const MapPoint dest,
                                                   const bool randomRoute, const unsigned maxLength,
                                                   std::vector<unsigned char>* route, unsigned* length, unsigned char* firstDir,
                                                   FP_Node_OK_Callback IsNodeOK, FP_Node_OK_Callback IsNodeOKAlternate, FP_Node_OK_Callback IsNodeToDestOk, const void* param,
                                                   const bool  /*record*/)
{
    if(start == dest)
    {
        // Path where start==goal should never happen
        RTTR_Assert(false);
        LOG.lprintf("WARNING: Bug detected (GF: %u). Please report this with the savegame and replay (Start==Dest in pathfinding %u,%u)\n", GAMECLIENT.GetGFNumber(), unsigned(start.x), unsigned(start.y));
        // But for now we assume it to be valid and return (kind of) correct values
        if(route)
            route->clear();
        if(length)
            *length = 0;
        if(firstDir)
            *firstDir = 0xff;
        return true;
    }

    // increase currentVisit, so we don't have to clear the visited-states at every run
    IncreaseCurrentVisit();

    std::list<PathfindingPoint> todo;
    const unsigned destId = gwb_.GetIdx(dest);

    bool prevStepEven = true; //flips between even and odd 
    unsigned stepsTilSwitch = 1;

    // Anfangsknoten einfügen
    unsigned startId = gwb_.GetIdx(start);
    todo.push_back(PathfindingPoint(startId, gwb_.CalcDistance(start, dest), 0));
    // Und mit entsprechenden Werten füllen
    //pf_nodes[start_id].it_p = ret.first;
    nodes[startId].prevEven = INVALID_PREV;
    nodes[startId].lastVisitedEven = currentVisit;
    nodes[startId].wayEven = 0;
    nodes[startId].dirEven = 0;
    //LOG.lprintf("pf: from %i, %i to %i, %i \n", x_start, y_start, x_dest, y_dest);

    // Bei Zufälliger Richtung anfangen (damit man nicht immer denselben Weg geht, besonders für die Soldaten wichtig)
    // TODO confirm random: RANDOM.Rand(__FILE__, __LINE__, y_start * GetWidth() + x_start, 6);
    const unsigned startDir = randomRoute ? (gwb_.GetIdx(start)) * GAMECLIENT.GetGFNumber() % 6 : 0; 

    while(!todo.empty())
    {		
        if(!stepsTilSwitch) //counter for next step and switch condition
        {			
            prevStepEven = !prevStepEven;
            stepsTilSwitch = todo.size();
            //prevstepEven ? LOG.lprintf("pf: even, to switch %i listsize %i ", stepsTilSwitch, todo.size()) : LOG.lprintf("pf: odd, to switch %i listsize %i ", stepsTilSwitch, todo.size());
        }
        //else
        //prevstepEven ? LOG.lprintf("pf: even, to switch %i listsize %i ", stepsTilSwitch, todo.size()) : LOG.lprintf("pf: odd, to switch %i listsize %i ", stepsTilSwitch, todo.size());
        stepsTilSwitch--;

        // Knoten mit den geringsten Wegkosten auswählen
        PathfindingPoint best = *todo.begin();
        // Knoten behandelt --> raus aus der todo Liste
        todo.erase(todo.begin());

        //printf("x: %u y: %u\n", best.x, best.y);

        // ID des besten Punktes ausrechnen

        unsigned bestId = best.id_;
        //LOG.lprintf(" now %i, %i id: %i \n", best.x, best.y, best_id);
        // Dieser Knoten wurde aus dem set entfernt, daher wird der entsprechende Iterator
        // auf das Ende (also nicht definiert) gesetzt, quasi als "NULL"-Ersatz
        //pf_nodes[best_id].it_p = todo.end();

        // Ziel schon erreicht?
        if(destId == bestId)
        {
            // Ziel erreicht!
            // Jeweils die einzelnen Angaben zurückgeben, falls gewünscht (Pointer übergeben)
            const unsigned routeLen = prevStepEven ? nodes[bestId].wayEven : nodes[bestId].way;
            if(length)
                *length = routeLen;
            if(route)
                route->resize(routeLen);

            // Route rekonstruieren und ggf. die erste Richtung speichern, falls gewünscht
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

        //LOG.lprintf("pf get neighbor nodes %i, %i id: %i \n", best.x, best.y, best_id);
        // Knoten in alle 6 Richtungen bilden
        for(unsigned z = startDir + 3; z < startDir + 9; ++z)
        {
            unsigned i = z % 6;

            // Koordinaten des entsprechenden umliegenden Punktes bilden
            MapPoint neighbourPos = gwb_.GetNeighbour(nodes[bestId].mapPt, i);

            // ID des umliegenden Knotens bilden
            unsigned nbId = gwb_.GetIdx(neighbourPos);

            // Knoten schon auf dem Feld gebildet ?
            if ((prevStepEven && nodes[nbId].lastVisited == currentVisit) || (!prevStepEven && nodes[nbId].lastVisitedEven == currentVisit))
            {
                continue;
            }

            // Das Ziel wollen wir auf jedenfall erreichen lassen, daher nur diese zusätzlichen
            // Bedingungen, wenn es nicht das Ziel ist
            if(nbId != destId && ((prevStepEven && IsNodeOK) || (!prevStepEven && IsNodeOKAlternate)))
            {
                if(prevStepEven)
                {
                    if(!IsNodeOK(gwb_, neighbourPos, i, param))
                        continue;
                }
                else
                {
                    if (!IsNodeOKAlternate(gwb_, neighbourPos, i, param))
                        continue;
                    MapPoint p = nodes[bestId].mapPt;

                    std::vector<MapPoint> evenLocationsOnRoute;
                    bool alternate = prevStepEven;
                    unsigned back_id = bestId;
                    for(unsigned i = nodes[bestId].way-1; i>1; i--) // backtrack the plannend route and check if another "even" position is too close
                    {
                        unsigned char pdir = alternate ? nodes[back_id].dirEven : nodes[back_id].dir;
                        p = gwb_.GetNeighbour(p, (pdir+3) % 6);
                        if(i%2 == 0) //even step
                        {	
                            evenLocationsOnRoute.push_back(p);
                        }
                        back_id = alternate ? nodes[back_id].prevEven : nodes[back_id].prev;
                        alternate = !alternate;
                    }
                    bool tooClose = false;
                    //LOG.lprintf("pf from %i, %i to %i, %i now %i, %i ", x_start, y_start, x_dest, y_dest, xa, ya);//\n
                    for(std::vector<MapPoint>::const_iterator it = evenLocationsOnRoute.begin();it!= evenLocationsOnRoute.end(); ++it)
                    {
                        //LOG.lprintf("dist to %i, %i ", temp, *it);
                        if(gwb_.CalcDistance(neighbourPos, (*it)) < 2)
                        {
                            tooClose = true;
                            break;
                        }
                    }
                    //LOG.lprintf("\n");
                    if(tooClose)
                        continue;
                    if(gwb_.CalcDistance(neighbourPos, start) < 2)
                        continue;
                    if(gwb_.CalcDistance(neighbourPos, dest) < 2)
                        continue;
                }
            }

            // Zusätzliche Bedingungen, auch die das letzte Stück zum Ziel betreffen
            if(IsNodeToDestOk)
            {
                if(!IsNodeToDestOk(gwb_, neighbourPos, i, param))
                    continue;
            }

            // Alles in Ordnung, Knoten kann gebildet werden
            unsigned way;
            if(prevStepEven)
            {
                nodes[nbId].lastVisited = currentVisit;
                way = nodes[nbId].way = nodes[bestId].wayEven + 1;
                nodes[nbId].dir = i;
                nodes[nbId].prev = bestId;
            }
            else
            {
                nodes[nbId].lastVisitedEven = currentVisit;
                way = nodes[nbId].wayEven = nodes[bestId].way + 1;
                nodes[nbId].dirEven = i;
                nodes[nbId].prevEven = bestId;
            }

            todo.push_back(PathfindingPoint(nbId, gwb_.CalcDistance(neighbourPos, dest), way));
            //pf_nodes[xaid].it_p = ret.first;
        }
    }

    // Liste leer und kein Ziel erreicht --> kein Weg
    return false;
}
