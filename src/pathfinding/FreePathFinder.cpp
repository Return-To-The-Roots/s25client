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

#include "defines.h"
#include "pathfinding/FreePathFinder.h"
#include "GameWorldBase.h"
#include "GameClient.h"
#include <set>

/// Konstante für einen ungültigen Vorgänerknoten
const unsigned INVALID_PREV = 0xFFFFFFFF;

/// Punkte als Verweise auf die obengenannen Knoten, damit nur die beiden Koordinaten x, y im set mit rumgeschleppt
/// werden müsen
struct PathfindingPoint
{
public:
    /// Die beiden Koordinaten des Punktes
    MapPoint pt;
    unsigned id, distance;

public:
    /// Konstruktoren
    PathfindingPoint(const MapPoint s, const unsigned sid): pt(s)
    {
        id = sid;
        distance = gwb->CalcDistance(s, dst);
    }

    /// Koordinaten des Ziels beim jeweils aktuellen Pathfinding, die wir zur Bewertung der Punkte benötigen
    static MapPoint dst;
    /// Pointer auf GameWorld, die wir brauchen, um die IDs zu berechnen bzw. die Kartengröße zu bekommen
    static const GameWorldBase* gwb;
    /// Diese statischen Variablen zu Beginn des Pathfindings festlegen
    static void Init(const MapPoint destPt, const GameWorldBase* gameWorld)
    {
        PathfindingPoint::dst = destPt;
        PathfindingPoint::gwb = gameWorld;
    }

    /// Operator für den Vergleich
    bool operator<(const PathfindingPoint& two) const;
};


/// Klass für einen Knoten mit dazugehörigen Informationen
/// Wir speichern einfach die gesamte Map und sparen uns so das dauernde Allokieren und Freigeben von Speicher
/// Die Knoten können im Array mit einer eindeutigen ID (gebildet aus y*Kartenbreite+x) identifiziert werden
struct NewNode
{
    NewNode() : way(0), dir(0), prev(INVALID_PREV), lastVisited(0) {}

    /// Wegkosten, die vom Startpunkt bis zu diesem Knoten bestehen
    unsigned way;
    unsigned wayEven;
    /// Die Richtung, über die dieser Knoten erreicht wurde
    unsigned char dir;
    unsigned char dirEven;
    /// ID (gebildet aus y*Kartenbreite+x) des Vorgänngerknotens
    unsigned prev;
    unsigned prevEven;
    /// Iterator auf Position in der Prioritätswarteschlange (std::set), freies Pathfinding
    std::set<PathfindingPoint>::iterator it_p;
    /// Wurde Knoten schon besucht (für A*-Algorithmus), wenn lastVisited == currentVisit
    unsigned lastVisited;
    unsigned lastVisitedEven; //used for road pathfinding (for ai only for now)
};

const unsigned maxMapSize = 1024;
/// Die Knoten der Map gespeichert, größtmöglichste Kartengröße nehmen
NewNode pf_nodes[maxMapSize* maxMapSize];
unsigned currentVisit = 0;
//unsigned currentVisitEven = 0; //used for road pathfinding (for now only the ai gets the comfort version)


bool PathfindingPoint::operator<(const PathfindingPoint& two) const
{
    // Weglängen schätzen für beide Punkte, indem man den bisherigen Weg mit der Luftlinie vom aktullen
    // Punkt zum Ziel addiert und auf diese Weise den kleinsten Weg auswählt
    unsigned way1 = pf_nodes[id].way + distance;
    unsigned way2 = pf_nodes[two.id].way + two.distance;

    // Wenn die Wegkosten gleich sind, vergleichen wir die Koordinaten, da wir für std::set eine streng
    // monoton steigende Folge brauchen
    if(way1 == way2)
        return (id < two.id);
    else
        return (way1 < way2);
}


/// Definitionen siehe oben
MapPoint PathfindingPoint::dst = MapPoint();
const GameWorldBase* PathfindingPoint::gwb = NULL;

/// Wegfinden ( A* ), O(v lg v) --> Wegfindung auf allgemeinen Terrain (ohne Straßen), für Wegbau und frei herumlaufende Berufe
bool FreePathFinder::FindPath(const MapPoint start, const MapPoint dest,
                              const bool random_route, const unsigned max_route,
                              std::vector<unsigned char>* route, unsigned* length, unsigned char* first_dir,
                              FP_Node_OK_Callback IsNodeOK, FP_Node_OK_Callback IsNodeToDestOk, const void* param,
                              const bool record)
{
    // increase currentVisit, so we don't have to clear the visited-states at every run
    currentVisit++;

    // if the counter reaches its maxium, tidy up
    if (currentVisit == std::numeric_limits<unsigned>::max() - 1)
    {
        for (unsigned i = 0; i < (maxMapSize * maxMapSize); ++i)
        {
            pf_nodes[i].lastVisited = 0;
            pf_nodes[i].lastVisitedEven = 0;
        }
        currentVisit = 1;
    }

    std::set<PathfindingPoint> todo;
    PathfindingPoint::Init(dest, &this->gwb_);

    // Anfangsknoten einfügen
    unsigned start_id = gwb_.GetIdx(start);
    std::pair< std::set<PathfindingPoint>::iterator, bool > ret = todo.insert(PathfindingPoint(start, start_id));
    // Und mit entsprechenden Werten füllen
    pf_nodes[start_id].it_p = ret.first;
    pf_nodes[start_id].prev = INVALID_PREV;
    pf_nodes[start_id].lastVisited = currentVisit;
    pf_nodes[start_id].way = 0;
    pf_nodes[start_id].dir = 0;

    // TODO confirm random
    unsigned rand = (gwb_.GetIdx(start)) * GAMECLIENT.GetGFNumber() % 6; //RANDOM.Rand(__FILE__, __LINE__, y_start * GetWidth() + x_start, 6);

    while(!todo.empty())
    {
        // Knoten mit den geringsten Wegkosten auswählen
        PathfindingPoint best = *todo.begin();
        // Knoten behandelt --> raus aus der todo Liste
        todo.erase(todo.begin());

        //printf("x: %u y: %u\n", best.x, best.y);

        // ID des besten Punktes ausrechnen

        unsigned best_id = best.id;

        // Dieser Knoten wurde aus dem set entfernt, daher wird der entsprechende Iterator
        // auf das Ende (also nicht definiert) gesetzt, quasi als "NULL"-Ersatz
        pf_nodes[best_id].it_p = todo.end();

        // Ziel schon erreicht? Allerdings Null-Weg, wenn Start=Ende ist, verbieten
        if(dest == best.pt && pf_nodes[best_id].way)
        {
            // Ziel erreicht!
            // Jeweils die einzelnen Angaben zurückgeben, falls gewünscht (Pointer übergeben)
            if(length)
                *length = pf_nodes[best_id].way;
            if(route)
                route->resize(pf_nodes[best_id].way);

            // Route rekonstruieren und ggf. die erste Richtung speichern, falls gewünscht
            for(unsigned z = pf_nodes[best_id].way - 1; best_id != start_id; --z, best_id = pf_nodes[best_id].prev)
            {
                if(route)
                    (*route)[z] = pf_nodes[best_id].dir;
                if(first_dir && z == 0)
                    *first_dir = pf_nodes[best_id].dir;
            }

            // Fertig, es wurde ein Pfad gefunden
            return true;
        }

        // Maximaler Weg schon erreicht? In dem Fall brauchen wir keine weiteren Knoten von diesem aus bilden
        if(pf_nodes[best_id].way == max_route)
            continue;

        // Bei Zufälliger Richtung anfangen (damit man nicht immer denselben Weg geht, besonders für die Soldaten wichtig)
        unsigned start = random_route ? rand : 0;

        // Knoten in alle 6 Richtungen bilden
        for(unsigned z = start + 3; z < start + 9; ++z)
        {
            unsigned i = z % 6;

            // Koordinaten des entsprechenden umliegenden Punktes bilden
            MapPoint na = gwb_.GetNeighbour(best.pt, i);

            // ID des umliegenden Knotens bilden
            unsigned xaid = gwb_.GetIdx(na);

            // Knoten schon auf dem Feld gebildet?
            if (pf_nodes[xaid].lastVisited == currentVisit)
            {
                // Dann nur ggf. Weg und Vorgänger korrigieren, falls der Weg kürzer ist
                if(pf_nodes[xaid].it_p != todo.end() && pf_nodes[best_id].way + 1 < pf_nodes[xaid].way)
                {
                    pf_nodes[xaid].way  = pf_nodes[best_id].way + 1;
                    pf_nodes[xaid].prev = best_id;
                    todo.erase(pf_nodes[xaid].it_p);
                    ret = todo.insert(PathfindingPoint(na, xaid));
                    pf_nodes[xaid].it_p = ret.first;
                    pf_nodes[xaid].dir = i;
                }
                // Wir wollen nicht denselben Knoten noch einmal einfügen, daher Abbruch
                continue;
            }

            // Das Ziel wollen wir auf jedenfall erreichen lassen, daher nur diese zusätzlichen
            // Bedingungen, wenn es nicht das Ziel ist
            if(na != dest && IsNodeOK)
            {
                if(!IsNodeOK(gwb_, na, i, param))
                    continue;
            }

            // Zusätzliche Bedingungen, auch die das letzte Stück zum Ziel betreffen
            if(IsNodeToDestOk)
            {
                if(!IsNodeToDestOk(gwb_, na, i, param))
                    continue;
            }

            // Alles in Ordnung, Knoten kann gebildet werden
            pf_nodes[xaid].lastVisited = currentVisit;
            pf_nodes[xaid].way = pf_nodes[best_id].way + 1;
            pf_nodes[xaid].dir = i;
            pf_nodes[xaid].prev = best_id;

            ret = todo.insert(PathfindingPoint(na, xaid));
            pf_nodes[xaid].it_p = ret.first;
        }
    }

    // Liste leer und kein Ziel erreicht --> kein Weg
    return false;
}

/// Wegfinden ( A* ), O(v lg v) --> Wegfindung auf allgemeinen Terrain (ohne Straßen), für Wegbau und frei herumlaufende Berufe
bool FreePathFinder::FindPathAlternatingConditions(const MapPoint start, const MapPoint dest,
                                                   const bool random_route, const unsigned max_route,
                                                   std::vector<unsigned char>* route, unsigned* length, unsigned char* first_dir,
                                                   FP_Node_OK_Callback IsNodeOK, FP_Node_OK_Callback IsNodeOKAlternate, FP_Node_OK_Callback IsNodeToDestOk, const void* param,
                                                   const bool record)
{
    // increase currentVisit, so we don't have to clear the visited-states at every run
    currentVisit++;
    //currentVisitEven++;

    // if the counter reaches its maxium, tidy up
    if (currentVisit == std::numeric_limits<unsigned>::max() - 1)
    {
        for (unsigned i = 0; i < (maxMapSize * maxMapSize); ++i)
        {
            pf_nodes[i].lastVisited = 0;
            pf_nodes[i].lastVisitedEven = 0;
        }
        currentVisit = 1;
        //currentVisitEven = 1;
    }

    std::list<PathfindingPoint> todo;
    bool prevstepEven=true; //flips between even and odd 
    unsigned stepsTilSwitch=1;
    PathfindingPoint::Init(dest, &gwb_);

    // Anfangsknoten einfügen
    unsigned start_id = gwb_.GetIdx(start);
    todo.push_back(PathfindingPoint(start, start_id));
    // Und mit entsprechenden Werten füllen
    //pf_nodes[start_id].it_p = ret.first;
    pf_nodes[start_id].prevEven = INVALID_PREV;
    pf_nodes[start_id].lastVisitedEven = currentVisit;
    pf_nodes[start_id].wayEven = 0;
    pf_nodes[start_id].dirEven = 0;
    //LOG.lprintf("pf: from %i, %i to %i, %i \n", x_start, y_start, x_dest, y_dest);
    // TODO confirm random
    unsigned rand = gwb_.GetIdx(start) * GAMECLIENT.GetGFNumber() % 6; //RANDOM.Rand(__FILE__, __LINE__, y_start * GetWidth() + x_start, 6);

    while(!todo.empty())
    {		
        if(!stepsTilSwitch) //counter for next step and switch condition
        {			
            prevstepEven=!prevstepEven;
            stepsTilSwitch=todo.size();
            //prevstepEven? LOG.lprintf("pf: even, to switch %i listsize %i ", stepsTilSwitch, todo.size()) : LOG.lprintf("pf: odd, to switch %i listsize %i ", stepsTilSwitch, todo.size());
        }
        //else
        //prevstepEven? LOG.lprintf("pf: even, to switch %i listsize %i ", stepsTilSwitch, todo.size()) : LOG.lprintf("pf: odd, to switch %i listsize %i ", stepsTilSwitch, todo.size());
        stepsTilSwitch--;

        // Knoten mit den geringsten Wegkosten auswählen
        PathfindingPoint best = *todo.begin();
        // Knoten behandelt --> raus aus der todo Liste
        todo.erase(todo.begin());

        //printf("x: %u y: %u\n", best.x, best.y);

        // ID des besten Punktes ausrechnen

        unsigned best_id = best.id;
        //LOG.lprintf(" now %i, %i id: %i \n", best.x, best.y, best_id);
        // Dieser Knoten wurde aus dem set entfernt, daher wird der entsprechende Iterator
        // auf das Ende (also nicht definiert) gesetzt, quasi als "NULL"-Ersatz
        //pf_nodes[best_id].it_p = todo.end();

        // Ziel schon erreicht? Allerdings Null-Weg, wenn Start=Ende ist, verbieten
        if(dest == best.pt && ((prevstepEven && pf_nodes[best_id].wayEven) || (!prevstepEven && pf_nodes[best_id].way)))
        {
            // Ziel erreicht!
            // Jeweils die einzelnen Angaben zurückgeben, falls gewünscht (Pointer übergeben)
            if(length)
                *length = prevstepEven ? pf_nodes[best_id].wayEven : pf_nodes[best_id].way;
            if(route)
                prevstepEven? route->resize(pf_nodes[best_id].wayEven) : route->resize(pf_nodes[best_id].way);

            // Route rekonstruieren und ggf. die erste Richtung speichern, falls gewünscht
            bool alternate=prevstepEven;
            for(unsigned z = prevstepEven? pf_nodes[best_id].wayEven - 1 : pf_nodes[best_id].way - 1; best_id != start_id; --z, best_id = alternate? pf_nodes[best_id].prevEven : pf_nodes[best_id].prev, alternate=!alternate)
            {
                if(route)
                    (*route)[z] = alternate? pf_nodes[best_id].dirEven : pf_nodes[best_id].dir;
                if(first_dir && z == 0)
                    *first_dir = pf_nodes[best_id].dirEven;				
            }

            // Fertig, es wurde ein Pfad gefunden
            return true;
        }

        // Maximaler Weg schon erreicht? In dem Fall brauchen wir keine weiteren Knoten von diesem aus bilden
        if((prevstepEven && pf_nodes[best_id].wayEven)==max_route || (!prevstepEven && pf_nodes[best_id].way == max_route))
            continue;

        // Bei Zufälliger Richtung anfangen (damit man nicht immer denselben Weg geht, besonders für die Soldaten wichtig)
        unsigned startDir = random_route ? rand : 0;
        //LOG.lprintf("pf get neighbor nodes %i, %i id: %i \n", best.x, best.y, best_id);
        // Knoten in alle 6 Richtungen bilden
        for(unsigned z = startDir + 3; z < startDir + 9; ++z)
        {
            unsigned i = z % 6;

            // Koordinaten des entsprechenden umliegenden Punktes bilden
            MapPoint na = gwb_.GetNeighbour(best.pt, i);

            // ID des umliegenden Knotens bilden
            unsigned xaid = gwb_.GetIdx(na);

            // Knoten schon auf dem Feld gebildet?
            if ((prevstepEven && pf_nodes[xaid].lastVisited == currentVisit) || (!prevstepEven && pf_nodes[xaid].lastVisitedEven == currentVisit))
            {
                continue;
            }

            // Das Ziel wollen wir auf jedenfall erreichen lassen, daher nur diese zusätzlichen
            // Bedingungen, wenn es nicht das Ziel ist
            if(na != dest && ((prevstepEven && IsNodeOK) || (!prevstepEven && IsNodeOKAlternate)))
            {
                if(prevstepEven)
                {
                    if(!IsNodeOK(gwb_, na, i, param))
                        continue;
                }
                else
                {
                    if (!IsNodeOKAlternate(gwb_, na, i, param))
                        continue;
                    MapPoint p = best.pt;

                    std::vector<MapPoint>evenlocationsonroute;
                    bool alternate=prevstepEven;
                    unsigned back_id=best_id;
                    for(unsigned i=pf_nodes[best_id].way-1; i>1; i--, back_id = alternate? pf_nodes[back_id].prevEven : pf_nodes[back_id].prev, alternate=!alternate) // backtrack the plannend route and check if another "even" position is too close
                    {
                        unsigned char pdir = alternate? pf_nodes[back_id].dirEven : pf_nodes[back_id].dir;
                        p = gwb_.GetNeighbour(p, (pdir+3)%6);
                        if(i%2==0) //even step
                        {	
                            evenlocationsonroute.push_back(p);
                        }
                    }
                    bool tooclose=false;
                    //LOG.lprintf("pf from %i, %i to %i, %i now %i, %i ", x_start, y_start, x_dest, y_dest, xa, ya);//\n
                    for(std::vector<MapPoint>::const_iterator it=evenlocationsonroute.begin();it!=evenlocationsonroute.end(); ++it)
                    {
                        //LOG.lprintf("dist to %i, %i ", temp, *it);
                        if(gwb_.CalcDistance(na, (*it))<2)
                        {
                            tooclose=true;
                            break;
                        }
                    }
                    //LOG.lprintf("\n");
                    if(gwb_.CalcDistance(na, start)<2)
                        continue;
                    if(gwb_.CalcDistance(na, dest)<2)
                        continue;
                    if(tooclose)
                        continue;
                }
            }

            // Zusätzliche Bedingungen, auch die das letzte Stück zum Ziel betreffen
            if(IsNodeToDestOk)
            {
                if(!IsNodeToDestOk(gwb_, na, i, param))
                    continue;
            }

            // Alles in Ordnung, Knoten kann gebildet werden
            prevstepEven? pf_nodes[xaid].lastVisited = currentVisit			: pf_nodes[xaid].lastVisitedEven = currentVisit;
            prevstepEven? pf_nodes[xaid].way = pf_nodes[best_id].wayEven + 1: pf_nodes[xaid].wayEven = pf_nodes[best_id].way + 1;
            prevstepEven? pf_nodes[xaid].dir = i							: pf_nodes[xaid].dirEven = i;
            prevstepEven? pf_nodes[xaid].prev = best_id						: pf_nodes[xaid].prevEven = best_id	;

            todo.push_back(PathfindingPoint(na, xaid));
            //pf_nodes[xaid].it_p = ret.first;
        }
    }

    // Liste leer und kein Ziel erreicht --> kein Weg
    return false;
}

/// Ermittelt, ob eine freie Route noch passierbar ist und gibt den Endpunkt der Route zurück
bool FreePathFinder::CheckRoute(const MapPoint start, const std::vector<unsigned char>& route, const unsigned pos, 
                                FP_Node_OK_Callback IsNodeOK, FP_Node_OK_Callback IsNodeToDestOk, MapPoint* dest, const void* const param) const
{
    MapPoint pt(start);

    assert(pos < route.size());

    for(unsigned i = pos; i < route.size(); ++i)
    {
        pt = gwb_.GetNeighbour(pt, route[i]);
        if(!IsNodeToDestOk(gwb_, pt, route[i], param))
            return false;
        if(i < route.size() - 1 && !IsNodeOK(gwb_, pt, route[i], param))
            return false;
    }

    if(dest)
        *dest = pt;

    return true;
}