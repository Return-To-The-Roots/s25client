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

typedef std::vector<NewNode> MapNodes;
extern MapNodes nodes;

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

    std::set<PathfindingPoint> todo;
    const unsigned destId = gwb_.GetIdx(dest);

    // Anfangsknoten einfügen Und mit entsprechenden Werten füllen
    unsigned startId = gwb_.GetIdx(start);
    nodes[startId].it_p = todo.insert(PathfindingPoint(startId, gwb_.CalcDistance(start, dest), 0)).first;
    nodes[startId].prev = INVALID_PREV;
    nodes[startId].lastVisited = currentVisit;
    nodes[startId].way = 0;
    nodes[startId].dir = 0;

    // Bei Zufälliger Richtung anfangen (damit man nicht immer denselben Weg geht, besonders für die Soldaten wichtig)
    // TODO confirm random: RANDOM.Rand(__FILE__, __LINE__, y_start * GetWidth() + x_start, 6);
    const unsigned startDir = randomRoute ? (gwb_.GetIdx(start)) * GAMECLIENT.GetGFNumber() % 6 : 0; 

    while(!todo.empty())
    {
        // Knoten mit den geringsten Wegkosten auswählen
        PathfindingPoint best = *todo.begin();
        // Knoten behandelt --> raus aus der todo Liste
        todo.erase(todo.begin());

        unsigned bestId = best.id_;

        // Dieser Knoten wurde aus dem set entfernt, daher wird der entsprechende Iterator
        // auf das Ende (also nicht definiert) gesetzt, quasi als "NULL"-Ersatz
        nodes[bestId].it_p = todo.end();

        // Ziel schon erreicht?
        if(destId == bestId)
        {
            // Ziel erreicht!
            // Jeweils die einzelnen Angaben zurückgeben, falls gewünscht (Pointer übergeben)
            if(length)
                *length = nodes[bestId].way;
            if(route)
                route->resize(nodes[bestId].way);

            // Route rekonstruieren und ggf. die erste Richtung speichern, falls gewünscht
            for(unsigned z = nodes[bestId].way - 1; bestId != startId; --z, bestId = nodes[bestId].prev)
            {
                if(route)
                    (*route)[z] = nodes[bestId].dir;
                if(firstDir && z == 0)
                    *firstDir = nodes[bestId].dir;
            }

            // Fertig, es wurde ein Pfad gefunden
            return true;
        }

        // Maximaler Weg schon erreicht? In dem Fall brauchen wir keine weiteren Knoten von diesem aus bilden
        if(nodes[bestId].way == maxLength)
            continue;

        // Knoten in alle 6 Richtungen bilden
        for(unsigned z = startDir; z < startDir + 6; ++z)
        {
            unsigned dir = z % 6;

            // Koordinaten des entsprechenden umliegenden Punktes bilden
            MapPoint neighbourPos = gwb_.GetNeighbour(nodes[bestId].mapPt, dir);

            // ID des umliegenden Knotens bilden
            unsigned nbId = gwb_.GetIdx(neighbourPos);

            // Don't try to go back where we came from (would also bail out in the conditions below)
            if(nodes[bestId].prev == nbId)
                continue;

            // Knoten schon auf dem Feld gebildet?
            if (nodes[nbId].lastVisited == currentVisit)
            {
                // Dann nur ggf. Weg und Vorgänger korrigieren, falls der Weg kürzer ist
                if(nodes[nbId].it_p != todo.end() && nodes[bestId].way + 1 < nodes[nbId].way)
                {
                    nodes[nbId].way  = nodes[bestId].way + 1;
                    nodes[nbId].prev = bestId;
                    PathfindingPoint neighborPt = *nodes[nbId].it_p;
                    neighborPt.estimate_ = neighborPt.distance_ + nodes[nbId].way;
                    todo.erase(nodes[nbId].it_p);
                    nodes[nbId].it_p = todo.insert(neighborPt).first;
                    nodes[nbId].dir = dir;
                }
                // Wir wollen nicht denselben Knoten noch einmal einfügen, daher Abbruch
                continue;
            }

            // Das Ziel wollen wir auf jedenfall erreichen lassen, daher nur diese zusätzlichen
            // Bedingungen, wenn es nicht das Ziel ist
            if(nbId != destId)
            {
                if(!nodeChecker.IsNodeOk(neighbourPos, dir))
                    continue;
            }

            // Zusätzliche Bedingungen, auch die das letzte Stück zum Ziel betreffen
            if(!nodeChecker.IsNodeToDestOk(neighbourPos, dir))
                continue;

            // Alles in Ordnung, Knoten kann gebildet werden
            nodes[nbId].lastVisited = currentVisit;
            nodes[nbId].way = nodes[bestId].way + 1;
            nodes[nbId].dir = dir;
            nodes[nbId].prev = bestId;

            nodes[nbId].it_p = todo.insert(PathfindingPoint(nbId, gwb_.CalcDistance(neighbourPos, dest), nodes[nbId].way)).first;
        }
    }

    // Liste leer und kein Ziel erreicht --> kein Weg
    return false;
}


#endif // FreePathFinderImpl_h__