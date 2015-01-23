// $Id: Pathfinding.cpp 9578 2015-01-23 08:28:58Z marcus $
//
// Copyright (c) 2005 - 2011 Settlers Freaks (sf-team at siedler25.org)
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

///////////////////////////////////////////////////////////////////////////////
// Header
#include "main.h"
#include "Node.h"

#include "GameWorld.h"
#include "noRoadNode.h"
#include "VideoDriverWrapper.h"
#include "Random.h"
#include "MapGeometry.h"
#include "nobHarborBuilding.h"
#include "GameClient.h"

#include <set>
#include <vector>
#include <limits>

#include <iostream>

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#if defined _WIN32 && defined _DEBUG && defined _MSC_VER
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/// Konstante für einen ungültigen Vorgänerknoten
const unsigned INVALID_PREV = 0xFFFFFFFF;

/// Vergleichsoperator für die Prioritätswarteschlange bzw. std::set beim straßengebundenen Wegfinden
class RoadNodeComperator
{
    public:
        bool operator()(const noRoadNode* const rn1, const noRoadNode* const rn2) const;
};

class RoadNodeComperatorInv
{
    public:
        bool operator()(const noRoadNode* const rn1, const noRoadNode* const rn2) const;
};
struct PathfindingPoint;

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

/// Punkte als Verweise auf die obengenannen Knoten, damit nur die beiden Koordinaten x,y im set mit rumgeschleppt
/// werden müsen
struct PathfindingPoint
{
    public:
        /// Die beiden Koordinaten des Punktes
        MapCoord x, y;
        unsigned id, distance;

    public:
        /// Konstruktoren
        PathfindingPoint(const MapCoord sx, const MapCoord sy, const unsigned sid)
        {
            x = sx;
            y = sy;
            id = sid;
            distance = gwb->CalcDistance(x, y, dst_x, dst_y);
        }

        /// Koordinaten des Ziels beim jeweils aktuellen Pathfinding, die wir zur Bewertung der Punkte benötigen
        static MapCoord dst_x, dst_y;
        /// Pointer auf GameWorld, die wir brauchen, um die IDs zu berechnen bzw. die Kartengröße zu bekommen
        static const GameWorldBase* gwb;
        /// Diese statischen Variablen zu Beginn des Pathfindings festlegen
        static void Init(const MapCoord dst_x, const MapCoord dst_y, const GameWorldBase* gwb)
        {
            PathfindingPoint::dst_x = dst_x;
            PathfindingPoint::dst_y = dst_y;
            PathfindingPoint::gwb = gwb;
        }

        /// Operator für den Vergleich
        bool operator<(const PathfindingPoint two) const
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
};


#define GetXYA(x, y, dx, dy, dir) \
    { \
        switch (dir) \
        { \
            case 0: \
                if (!x) \
                    dx = width - 1; \
                else \
                    dx = x - 1; \
                dy = y; \
                break; \
            case 1: \
                if (y & 1) \
                    dx = x; \
                else if (!x) \
                    dx = width - 1; \
                else \
                    dx = x - 1; \
                if (y > 0) \
                    dy = y - 1; \
                else \
                    dy = height - 1; \
                break; \
            case 2: \
                if (y & 1) \
                    if (x == width - 1) \
                        dx = 0; \
                    else \
                        dx = x + 1; \
                else \
                    dx = x; \
                if (y > 0) \
                    dy = y - 1; \
                else \
                    dy = height - 1; \
                break; \
            case 3: \
                if (x == width - 1) \
                    dx = 0; \
                else \
                    dx = x + 1; \
                dy = y; \
                break; \
            case 4: \
                if (y & 1) \
                    if (x == width - 1) \
                        dx = 0; \
                    else \
                        dx = x + 1; \
                else \
                    dx = x; \
                if (y == height - 1) \
                    dy = 0; \
                else \
                    dy = y + 1; \
                break; \
            case 5: \
                if (y & 1) \
                    dx = x; \
                else if (!x) \
                    dx = width - 1; \
                else \
                    dx = x - 1; \
                if (y == height - 1) \
                    dy = 0; \
                else \
                    dy = y + 1; \
                break; \
            default: \
                break; \
        } \
    }



/// Definitionen siehe oben
MapCoord PathfindingPoint::dst_x = 0;
MapCoord PathfindingPoint::dst_y = 0;
const GameWorldBase* PathfindingPoint::gwb = NULL;

/// Wegfinden ( A* ), O(v lg v) --> Wegfindung auf allgemeinen Terrain (ohne Straßen), für Wegbau und frei herumlaufende Berufe
bool GameWorldBase::FindFreePath(const MapCoord x_start, const MapCoord y_start,
                                 const MapCoord x_dest, const MapCoord y_dest, const bool random_route,
                                 const unsigned max_route, std::vector<unsigned char>* route, unsigned* length,
                                 unsigned char* first_dir,  FP_Node_OK_Callback IsNodeOK, FP_Node_OK_Callback IsNodeToDestOk, const void* param, const bool record) const
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
    PathfindingPoint::Init(x_dest, y_dest, this);

    // Anfangsknoten einfügen
    unsigned start_id = MakeCoordID(x_start, y_start);
    std::pair< std::set<PathfindingPoint>::iterator, bool > ret = todo.insert(PathfindingPoint(x_start, y_start, start_id));
    // Und mit entsprechenden Werten füllen
    pf_nodes[start_id].it_p = ret.first;
    pf_nodes[start_id].prev = INVALID_PREV;
    pf_nodes[start_id].lastVisited = currentVisit;
    pf_nodes[start_id].way = 0;
    pf_nodes[start_id].dir = 0;

    // TODO confirm random
    unsigned rand = (y_start * GetWidth() + x_start) * GAMECLIENT.GetGFNumber() % 6; //RANDOM.Rand(__FILE__, __LINE__, y_start * GetWidth() + x_start, 6);

    while(todo.size())
    {
        // Knoten mit den geringsten Wegkosten auswählen
        PathfindingPoint best = *todo.begin();
        // Knoten behandelt --> raus aus der todo Liste
        todo.erase(todo.begin());

        //printf("x: %u y: %u\n",best.x,best.y);

        // ID des besten Punktes ausrechnen

        unsigned best_id = best.id;

        // Dieser Knoten wurde aus dem set entfernt, daher wird der entsprechende Iterator
        // auf das Ende (also nicht definiert) gesetzt, quasi als "NULL"-Ersatz
        pf_nodes[best_id].it_p = todo.end();

        // Ziel schon erreicht? Allerdings Null-Weg, wenn Start=Ende ist, verbieten
        if(x_dest == best.x && y_dest == best.y && pf_nodes[best_id].way)
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
                    route->at(z) = pf_nodes[best_id].dir;
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
            MapCoord xa, ya;

            GetXYA(best.x, best.y, xa, ya, i);

            // ID des umliegenden Knotens bilden
            unsigned xaid = MakeCoordID(xa, ya);

            // Knoten schon auf dem Feld gebildet?
            if (pf_nodes[xaid].lastVisited == currentVisit)
            {
                // Dann nur ggf. Weg und Vorgänger korrigieren, falls der Weg kürzer ist
                if(pf_nodes[xaid].it_p != todo.end() && pf_nodes[best_id].way + 1 < pf_nodes[xaid].way)
                {
                    pf_nodes[xaid].way  = pf_nodes[best_id].way + 1;
                    pf_nodes[xaid].prev = best_id;
                    todo.erase(pf_nodes[xaid].it_p);
                    ret = todo.insert(PathfindingPoint(xa, ya, xaid));
                    pf_nodes[xaid].it_p = ret.first;
                    pf_nodes[xaid].dir = i;
                }
                // Wir wollen nicht denselben Knoten noch einmal einfügen, daher Abbruch
                continue;
            }

            // Das Ziel wollen wir auf jedenfall erreichen lassen, daher nur diese zusätzlichen
            // Bedingungen, wenn es nicht das Ziel ist
            if(!(xa == x_dest && ya == y_dest) && IsNodeOK)
            {
                if(!IsNodeOK(*this, xa, ya, i, param))
                    continue;
            }

            // Zusätzliche Bedingungen, auch die das letzte Stück zum Ziel betreffen
            if(IsNodeToDestOk)
            {
                if(!IsNodeToDestOk(*this, xa, ya, i, param))
                    continue;
            }

            // Alles in Ordnung, Knoten kann gebildet werden
            pf_nodes[xaid].lastVisited = currentVisit;
            pf_nodes[xaid].way = pf_nodes[best_id].way + 1;
            pf_nodes[xaid].dir = i;
            pf_nodes[xaid].prev = best_id;

            ret = todo.insert(PathfindingPoint(xa, ya, xaid));
            pf_nodes[xaid].it_p = ret.first;
        }
    }

    // Liste leer und kein Ziel erreicht --> kein Weg
    return false;
}

/// Wegfinden ( A* ), O(v lg v) --> Wegfindung auf allgemeinen Terrain (ohne Straßen), für Wegbau und frei herumlaufende Berufe
bool GameWorldBase::FindFreePathAlternatingConditions(const MapCoord x_start, const MapCoord y_start,
                                 const MapCoord x_dest, const MapCoord y_dest, const bool random_route,
                                 const unsigned max_route, std::vector<unsigned char>* route, unsigned* length,
                                 unsigned char* first_dir,  FP_Node_OK_Callback IsNodeOK, FP_Node_OK_Callback IsNodeOKAlternate, FP_Node_OK_Callback IsNodeToDestOk, const void* param, const bool record) const
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
    PathfindingPoint::Init(x_dest, y_dest, this);

    // Anfangsknoten einfügen
    unsigned start_id = MakeCoordID(x_start, y_start);
	todo.push_back(PathfindingPoint(x_start, y_start, start_id));
    // Und mit entsprechenden Werten füllen
    //pf_nodes[start_id].it_p = ret.first;
    pf_nodes[start_id].prevEven = INVALID_PREV;
    pf_nodes[start_id].lastVisitedEven = currentVisit;
    pf_nodes[start_id].wayEven = 0;
    pf_nodes[start_id].dirEven = 0;
	//LOG.lprintf("pf: from %i,%i to %i,%i \n",x_start,y_start,x_dest,y_dest);
    // TODO confirm random
    unsigned rand = (y_start * GetWidth() + x_start) * GAMECLIENT.GetGFNumber() % 6; //RANDOM.Rand(__FILE__, __LINE__, y_start * GetWidth() + x_start, 6);

	while(todo.size())
    {		
		if(!stepsTilSwitch) //counter for next step and switch condition
		{			
			prevstepEven=!prevstepEven;
			stepsTilSwitch=todo.size();
			//prevstepEven? LOG.lprintf("pf: even, to switch %i listsize %i ",stepsTilSwitch,todo.size()) : LOG.lprintf("pf: odd, to switch %i listsize %i ",stepsTilSwitch,todo.size());
		}
		//else
			//prevstepEven? LOG.lprintf("pf: even, to switch %i listsize %i ",stepsTilSwitch,todo.size()) : LOG.lprintf("pf: odd, to switch %i listsize %i ",stepsTilSwitch,todo.size());
		stepsTilSwitch--;

        // Knoten mit den geringsten Wegkosten auswählen
        PathfindingPoint best = *todo.begin();
        // Knoten behandelt --> raus aus der todo Liste
        todo.erase(todo.begin());

        //printf("x: %u y: %u\n",best.x,best.y);

        // ID des besten Punktes ausrechnen

        unsigned best_id = best.id;
		//LOG.lprintf(" now %i,%i id: %i \n",best.x,best.y,best_id);
        // Dieser Knoten wurde aus dem set entfernt, daher wird der entsprechende Iterator
        // auf das Ende (also nicht definiert) gesetzt, quasi als "NULL"-Ersatz
        //pf_nodes[best_id].it_p = todo.end();

        // Ziel schon erreicht? Allerdings Null-Weg, wenn Start=Ende ist, verbieten
        if(x_dest == best.x && y_dest == best.y && ((prevstepEven && pf_nodes[best_id].wayEven) || (!prevstepEven && pf_nodes[best_id].way)))
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
                    route->at(z) = alternate? pf_nodes[best_id].dirEven : pf_nodes[best_id].dir;
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
        unsigned start = random_route ? rand : 0;
		//LOG.lprintf("pf get neighbor nodes %i,%i id: %i \n",best.x,best.y,best_id);
        // Knoten in alle 6 Richtungen bilden
        for(unsigned z = start + 3; z < start + 9; ++z)
        {
            unsigned i = z % 6;

            // Koordinaten des entsprechenden umliegenden Punktes bilden
            MapCoord xa, ya;

            GetXYA(best.x, best.y, xa, ya, i);

            // ID des umliegenden Knotens bilden
            unsigned xaid = MakeCoordID(xa, ya);

            // Knoten schon auf dem Feld gebildet?
            if ((prevstepEven && pf_nodes[xaid].lastVisited == currentVisit) || (!prevstepEven && pf_nodes[xaid].lastVisitedEven == currentVisit))
            {
                continue;
            }

            // Das Ziel wollen wir auf jedenfall erreichen lassen, daher nur diese zusätzlichen
            // Bedingungen, wenn es nicht das Ziel ist
            if(!(xa == x_dest && ya == y_dest) && ((prevstepEven && IsNodeOK) || (!prevstepEven && IsNodeOKAlternate)))
            {
                if(prevstepEven)
				{
					if(!IsNodeOK(*this, xa, ya, i, param))
						continue;
				}
				else
				{
					if (!IsNodeOKAlternate(*this, xa, ya, i, param))
						continue;
					MapCoord px=best.x,py=best.y;
					MapCoord tx=px,ty=py;

					std::list<MapCoord>evenlocationsonroute;
					bool alternate=prevstepEven;
					unsigned back_id=best_id;
					for(unsigned i=pf_nodes[best_id].way-1; i>1; i--,back_id = alternate? pf_nodes[back_id].prevEven : pf_nodes[back_id].prev, alternate=!alternate) // backtrack the plannend route and check if another "even" position is too close
					{
						unsigned char pdir = alternate? pf_nodes[back_id].dirEven : pf_nodes[back_id].dir;
						px=GetXA(tx,ty,(pdir+3)%6);
						py=GetYA(tx,ty,(pdir+3)%6);
						tx=px;
						ty=py;
						if(i%2==0) //even step
						{	
							evenlocationsonroute.push_back(px);
							evenlocationsonroute.push_back(py);
						}
					}
					bool tooclose=false;
					//LOG.lprintf("pf from %i,%i to %i,%i now %i,%i ",x_start,y_start,x_dest,y_dest,xa,ya);//\n
					for(std::list<MapCoord>::const_iterator it=evenlocationsonroute.begin();it!=evenlocationsonroute.end();it++)
					{
						MapCoord temp=*it;
						it++;
						//LOG.lprintf("dist to %i,%i ",temp,*it);
						if(CalcDistance(xa,ya,temp,(*it))<2)
						{
							tooclose=true;
							break;
						}
					}
					//LOG.lprintf("\n");
					if(CalcDistance(xa,ya,x_start,y_start)<2)
						continue;
					if(CalcDistance(xa,ya,x_dest,y_dest)<2)
						continue;
					if(tooclose)
						continue;
				}
            }

            // Zusätzliche Bedingungen, auch die das letzte Stück zum Ziel betreffen
            if(IsNodeToDestOk)
            {
                if(!IsNodeToDestOk(*this, xa, ya, i, param))
                    continue;
            }

            // Alles in Ordnung, Knoten kann gebildet werden
            prevstepEven? pf_nodes[xaid].lastVisited = currentVisit			: pf_nodes[xaid].lastVisitedEven = currentVisit;
            prevstepEven? pf_nodes[xaid].way = pf_nodes[best_id].wayEven + 1: pf_nodes[xaid].wayEven = pf_nodes[best_id].way + 1;
            prevstepEven? pf_nodes[xaid].dir = i							: pf_nodes[xaid].dirEven = i;
            prevstepEven? pf_nodes[xaid].prev = best_id						: pf_nodes[xaid].prevEven = best_id	;

            todo.push_back(PathfindingPoint(xa, ya, xaid));
            //pf_nodes[xaid].it_p = ret.first;
        }
    }

    // Liste leer und kein Ziel erreicht --> kein Weg
    return false;
}

template<class _Ty,
         class _Container = std::vector<_Ty>,
         class _Pr = std::less<typename _Container::value_type> >
class openlist_container : public std::priority_queue<_Ty,  _Container, _Pr>
{
    public:
        openlist_container()
            : std::priority_queue<_Ty,  _Container, _Pr>()
        {
            std::priority_queue<_Ty,    _Container, _Pr>::c.reserve(255);
        }

        void rearrange(const _Ty& target)
        {
            typename std::vector<_Ty>::iterator it = std::find(std::priority_queue<_Ty, _Container, _Pr>::c.begin(), std::priority_queue<_Ty,   _Container, _Pr>::c.end(), target);
            std::push_heap(std::priority_queue<_Ty, _Container, _Pr>::c.begin(), it + 1, std::priority_queue<_Ty, _Container, _Pr>::comp);
        }

        void clear()
        {
            std::priority_queue<_Ty,    _Container, _Pr>::c.clear();
        }
};

// a 'second' current_visit for road pathfinding
unsigned current_visit_on_roads = 0;

// Vergleichsoperator für das straßengebundene Pathfinding, wird genauso wie das freie Pathfinding
// gehandelt, nur dass wir noRoadNodes statt direkt Points vergleichen
bool RoadNodeComperator::operator()(const noRoadNode* const rn1, const noRoadNode* const rn2) const
{
    if (rn1->estimate == rn2->estimate)
    {
        // Wenn die Wegkosten gleich sind, vergleichen wir die Koordinaten, da wir für std::set eine streng
        // monoton steigende Folge brauchen
        return (rn1->coord_id < rn2->coord_id);
    }

    return (rn1->estimate < rn2->estimate);
}

bool RoadNodeComperatorInv::operator()(const noRoadNode* const rn1, const noRoadNode* const rn2) const
{
    if (rn1->estimate == rn2->estimate)
    {
        // Wenn die Wegkosten gleich sind, vergleichen wir die Koordinaten, da wir für std::set eine streng
        // monoton steigende Folge brauchen
        return (rn1->coord_id > rn2->coord_id);
    }

    return (rn1->estimate > rn2->estimate);
}

openlist_container<const noRoadNode*, std::vector<const noRoadNode*>, RoadNodeComperatorInv> todo;

/// Wegfinden ( A* ), O(v lg v) --> Wegfindung auf allgemeinen Terrain (ohne Straßen), für Wegbau und frei herumlaufende Berufe
bool GameWorldBase::FindPathOnRoads(const noRoadNode* const start, const noRoadNode* const goal,
                                    const bool ware_mode, unsigned* length,
                                    unsigned char* first_dir,  Point<MapCoord>* next_harbor,
                                    const RoadSegment* const forbidden, const bool record, unsigned max) const
{
    // Aus Replay lesen?
    if(GameClient::inst().ArePathfindingResultsAvailable() && record)
    {
        unsigned char dir;
        if(GameClient::inst().ReadPathfindingResult(&dir, length, next_harbor))
        {
            if(first_dir) *first_dir = dir;
            return (dir != 0xff);
        }
    }

    // Irgendwelche Null-Anfänge oder Ziele? --> Kein Weg
    if(!start || !goal)
    {
        if(record)
            GameClient::inst().AddPathfindingResult(0xff, length, next_harbor);
        return false;
    }

    // increase current_visit_on_roads, so we don't have to clear the visited-states at every run
    current_visit_on_roads++;

    // if the counter reaches its maxium, tidy up
    if (current_visit_on_roads == std::numeric_limits<unsigned>::max())
    {
        for (int idx = width * height; idx >= 0; --idx)
        {
            const noRoadNode* node = dynamic_cast<const noRoadNode*>(nodes[idx].obj);

            if (node)
            {
                node->last_visit = 0;
            }
        }

        current_visit_on_roads = 1;
    }

    // Anfangsknoten einfügen
    todo.clear();

    start->distance = start->estimate = CalcDistance(start->GetX(), start->GetY(), goal->GetX(), goal->GetY());
    start->last_visit = current_visit_on_roads;
    start->prev = NULL;
    start->cost = start->dir = 0;

    todo.push(start);

    while (todo.size())
    {
        // Knoten mit den geringsten Wegkosten auswählen
        const noRoadNode* best = todo.top();

        // Knoten behandelt --> raus aus der todo Liste
        todo.pop();

        // Ziel erreicht, allerdings keine Nullwege erlauben?
        if (best == goal && best->cost)
        {
            // Jeweils die einzelnen Angaben zurückgeben, falls gewünscht (Pointer übergeben)
            if (length)
            {
                *length = best->cost;
            }

            const noRoadNode* last = best;

            while (best != start)
            {
                last = best;
                best = best->prev;
            }

            if (first_dir)
            {
                *first_dir = (unsigned char) last->dir;
            }

            if (next_harbor)
            {
                next_harbor->x = last->GetX();
                next_harbor->y = last->GetY();
            }

            // Fertig, es wurde ein Pfad gefunden
            if (record)
            {
                GameClient::inst().AddPathfindingResult((unsigned char) last->dir, length, next_harbor);
            }

            return true;
        }

        // Nachbarflagge bzw. Wege in allen 6 Richtungen verfolgen
        for (unsigned i = 0; i < 6; ++i)
        {
            // Gibt es auch einen solchen Weg bzw. Nachbarflagge?
            noRoadNode* neighbour = best->GetNeighbour(i);

            // Wenn nicht, brauchen wir mit dieser Richtung gar nicht weiter zu machen
            if (!neighbour)
                continue;

            // this eliminates 1/6 of all nodes and avoids cost calculation and further checks,
            // therefore - and because the profiler says so - it is more efficient that way
            if (neighbour == best->prev)
                continue;

            // evtl verboten?
            if (best->routes[i] == forbidden)
                continue;

            // Keine Umwege über Gebäude, ausgenommen Häfen und Ziele
            if ((i == 1) && (neighbour != goal) && (neighbour->GetGOT() != GOT_FLAG) && (neighbour->GetGOT() != GOT_NOB_HARBORBUILDING))
            {
                continue;
            }

            // Neuer Weg für diesen neuen Knoten berechnen
            unsigned cost = best->cost + best->routes[i]->GetLength();

            // Im Warenmodus müssen wir Strafpunkte für überlastete Träger hinzuaddieren,
            // damit der Algorithmus auch Ausweichrouten auswählt
            if (ware_mode)
            {
                cost += best->GetPunishmentPoints(i);
            }
            else if (best->routes[i]->GetRoadType() == RoadSegment::RT_BOAT)    // evtl Wasserstraße?
            {
                continue;
            }

            if (cost > max)
                continue;

            // Knoten schon auf dem Feld gebildet?
            if (neighbour->last_visit == current_visit_on_roads)
            {
                // Dann nur ggf. Weg und Vorgänger korrigieren, falls der Weg kürzer ist
                if (cost < neighbour->cost)
                {
                    neighbour->cost = cost;
                    neighbour->prev = best;
                    neighbour->estimate = neighbour->distance + cost;
                    todo.rearrange(neighbour);
                    neighbour->dir = i;
                }

                continue;
            }

            // Alles in Ordnung, Knoten kann gebildet werden
            neighbour->last_visit = current_visit_on_roads;
            neighbour->cost = cost;
            neighbour->dir = i;
            neighbour->prev = best;

            neighbour->distance = CalcDistance(neighbour->GetX(), neighbour->GetY(), goal->GetX(), goal->GetY());
            neighbour->estimate = neighbour->distance + cost;

            todo.push(neighbour);
        }

        // Stehen wir hier auf einem Hafenplatz
        if (best->GetGOT() == GOT_NOB_HARBORBUILDING)
        {
            std::vector<nobHarborBuilding::ShipConnection> scs;
            static_cast<const nobHarborBuilding*>(best)->GetShipConnections(scs);

            for (unsigned i = 0; i < scs.size(); ++i)
            {
                // Neuer Weg für diesen neuen Knoten berechnen
                unsigned cost = best->cost + scs[i].way_costs;

                if (cost > max)
                    continue;

                // Knoten schon auf dem Feld gebildet?
                if (scs[i].dest->last_visit == current_visit_on_roads)
                {
                    // Dann nur ggf. Weg und Vorgänger korrigieren, falls der Weg kürzer ist
                    if (cost < scs[i].dest->cost)
                    {
                        scs[i].dest->dir = 100;
                        scs[i].dest->cost = cost;
                        scs[i].dest->prev = best;
                        scs[i].dest->estimate = scs[i].dest->distance + cost;
                        todo.rearrange(scs[i].dest);
                    }

                    continue;
                }

                // Alles in Ordnung, Knoten kann gebildet werden
                scs[i].dest->last_visit = current_visit_on_roads;

                scs[i].dest->dir = 100;
                scs[i].dest->prev = best;
                scs[i].dest->cost = cost;

                scs[i].dest->distance = CalcDistance(scs[i].dest->GetX(), scs[i].dest->GetY(), goal->GetX(), goal->GetY());
                scs[i].dest->estimate = scs[i].dest->distance + cost;

                todo.push(scs[i].dest);
            }
        }
    }

    // Liste leer und kein Ziel erreicht --> kein Weg
    if(record)
        GameClient::inst().AddPathfindingResult(0xff, length, next_harbor);
    return false;
}



/// Ermittelt, ob eine freie Route noch passierbar ist und gibt den Endpunkt der Route zurück
bool GameWorldBase::CheckFreeRoute(const MapCoord x_start, const MapCoord y_start, const std::vector<unsigned char>& route, const unsigned pos,
                                   FP_Node_OK_Callback IsNodeOK, FP_Node_OK_Callback IsNodeToDestOk,  MapCoord* x_dest, MapCoord* y_dest, const void* const param) const
{
    MapCoord x = x_start, y = y_start;

    assert(pos < route.size());

    for(unsigned i = pos; i < route.size(); ++i)
    {
        GetPointA(x, y, route[i]);
        if(!IsNodeToDestOk(*this, x, y, route[i], param))
            return false;
        if(i < route.size() - 1 && !IsNodeOK(*this, x, y, route[i], param))
            return false;
    }

    if(x_dest)
        *x_dest = x;
    if(y_dest)
        *y_dest = y;

    return true;
}

/// Paremter-Struktur für Straßenbaupathfinding
struct Param_RoadPath
{
    /// Straßenbaumodus erlaubt?
    bool boat_road;
};

/// Abbruch-Bedingungen für Straßenbau-Pathfinding
bool IsPointOK_RoadPath(const GameWorldBase& gwb, const MapCoord x, const MapCoord y, const unsigned char dir, const void* param)
{
    const Param_RoadPath* prp = static_cast<const Param_RoadPath*>(param);

    // Auch auf unserem Territorium?
    if(!gwb.IsPlayerTerritory(x, y))
        return false;

    // Feld bebaubar?
    if(!gwb.RoadAvailable(prp->boat_road, x, y, dir))
        return false;

    return true;
}

/// Abbruch-Bedingungen für Straßenbau-Pathfinding for comfort road construction with a possible flag every 2 steps
bool IsPointOK_RoadPathEvenStep(const GameWorldBase& gwb, const MapCoord x, const MapCoord y, const unsigned char dir, const void* param)
{
    const Param_RoadPath* prp = static_cast<const Param_RoadPath*>(param);

    // Auch auf unserem Territorium?
    if(!gwb.IsPlayerTerritory(x, y))
        return false;

    // Feld bebaubar?
    if(!gwb.RoadAvailable(prp->boat_road, x, y, dir))
        return false;
	if(!prp->boat_road && (gwb.CalcBQ(x,y,gwb.GetNode(x,y).owner-1,true,false) != BQ_FLAG))
		return false;	

    return true;
}

/// Straßenbau-Pathfinding
bool GameWorldViewer::FindRoadPath(const MapCoord x_start, const MapCoord y_start, const MapCoord x_dest, const MapCoord y_dest, std::vector<unsigned char>& route, const bool boat_road)
{
    Param_RoadPath prp = { boat_road };
    return FindFreePath(x_start, y_start, x_dest, y_dest, false, 100, &route, NULL, NULL, IsPointOK_RoadPath, NULL, &prp, false);
}

/// Abbruch-Bedingungen für freien Pfad für Menschen
bool IsPointOK_HumanPath(const GameWorldBase& gwb, const MapCoord x, const MapCoord y, const unsigned char dir, const void* param)
{
    // Feld passierbar?
    noBase::BlockingManner bm = gwb.GetNO(x, y)->GetBM();
    if(bm != noBase::BM_NOTBLOCKING && bm != noBase::BM_TREE && bm != noBase::BM_FLAG)
        return false;

    return true;
}



/// Zusätzliche Abbruch-Bedingungen für freien Pfad für Menschen, die auch bei der letzen Kante
/// zum Ziel eingehalten werden müssen
bool IsPointToDestOK_HumanPath(const GameWorldBase& gwb, const MapCoord x, const MapCoord y, const unsigned char dir, const void* param)
{
    // Feld passierbar?
    // Nicht über Wasser, Lava, Sümpfe gehen
    if(!gwb.IsNodeToNodeForFigure(x, y, (dir + 3) % 6))
        return false;

    return true;
}

/// Abbruch-Bedingungen für freien Pfad für Schiffe
bool IsPointOK_ShipPath(const GameWorldBase& gwb, const MapCoord x, const MapCoord y, const unsigned char dir, const void* param)
{
    // Ein Meeresfeld?
    for(unsigned i = 0; i < 6; ++i)
    {
        if(gwb.GetTerrainAround(x, y, i) != TT_WATER)
            return false;
    }

    return true;
}

/// Zusätzliche Abbruch-Bedingungen für freien Pfad für Schiffe, die auch bei der letzen Kante
/// zum Ziel eingehalten werden müssen
bool IsPointToDestOK_ShipPath(const GameWorldBase& gwb, const MapCoord x, const MapCoord y, const unsigned char dir, const void* param)
{
    // Der Übergang muss immer aus Wasser sein zu beiden Seiten
    if(gwb.GetWalkingTerrain1(x, y, (dir + 3) % 6) == TT_WATER && gwb.GetWalkingTerrain2(x, y, (dir + 3) % 6) == TT_WATER)
        return true;
    else
        return false;
}

/// Findet einen Weg für Figuren
unsigned char GameWorldBase::FindHumanPath(const MapCoord x_start, const MapCoord y_start,
        const MapCoord x_dest, const MapCoord y_dest, const unsigned max_route, const bool random_route, unsigned* length, const bool record) const
{
    // Aus Replay lesen?
    if(GameClient::inst().ArePathfindingResultsAvailable() && !random_route)
    {
        unsigned char dir;
        if(GameClient::inst().ReadPathfindingResult(&dir, length, NULL))
            return dir;
    }

    unsigned char first_dir = 0xFF;
    FindFreePath(x_start, y_start, x_dest, y_dest, random_route, max_route, NULL, length, &first_dir, IsPointOK_HumanPath,
                 IsPointToDestOK_HumanPath, NULL, record);

    if(!random_route)
        GameClient::inst().AddPathfindingResult(first_dir, length, NULL);

    return first_dir;

}

/// Wegfindung für Menschen im Straßennetz
unsigned char GameWorldGame::FindHumanPathOnRoads(const noRoadNode* const start, const noRoadNode* const goal, unsigned* length, Point<MapCoord>* next_harbor, const RoadSegment* const forbidden)
{
    unsigned char first_dir = 0xFF;
    if(FindPathOnRoads(start, goal, false, length, &first_dir, next_harbor, forbidden))
        return first_dir;
    else
        return 0xFF;
}

/// Wegfindung für Waren im Straßennetz
unsigned char GameWorldGame::FindPathForWareOnRoads(const noRoadNode* const start, const noRoadNode* const goal, unsigned* length, Point<MapCoord>* next_harbor, unsigned max)
{
    unsigned char first_dir = 0xFF;
    if(FindPathOnRoads(start, goal, true, length, &first_dir, next_harbor, NULL, true, max))
        return first_dir;
    else
        return 0xFF;
}


/// Wegfindung für Schiffe auf dem Wasser
bool GameWorldBase::FindShipPath(const MapCoord x_start, const MapCoord y_start, const MapCoord x_dest,
                                 const MapCoord y_dest, std::vector<unsigned char>* route, unsigned* length, const unsigned max_length,
                                 GameWorldBase::CrossBorders* cb)
{
    return FindFreePath(x_start, y_start, x_dest, y_dest, true, 400, route, length, NULL, IsPointOK_ShipPath,
                        IsPointToDestOK_ShipPath, NULL, false);
}

/// Prüft, ob eine Schiffsroute noch Gültigkeit hat
bool GameWorldGame::CheckShipRoute(const MapCoord x_start, const MapCoord y_start, const std::vector<unsigned char>& route, const unsigned pos,
                                   MapCoord* x_dest,  MapCoord* y_dest)
{
    return CheckFreeRoute(x_start, y_start, route, pos, IsPointOK_ShipPath,
                          IsPointToDestOK_ShipPath, x_dest, y_dest, NULL);
}


/// Abbruch-Bedingungen für freien Pfad für Menschen
bool IsPointOK_TradePath(const GameWorldBase& gwb, const MapCoord x, const MapCoord y, const unsigned char dir, const void* param)
{
    // Feld passierbar?
    noBase::BlockingManner bm = gwb.GetNO(x, y)->GetBM();
    if(bm != noBase::BM_NOTBLOCKING && bm != noBase::BM_TREE && bm != noBase::BM_FLAG)
        return false;


    unsigned char player = gwb.GetNode(x, y).owner;
    // Ally or no player? Then ok
    if(player == 0 || gwb.GetPlayer(*((unsigned char*)param))->IsAlly(player - 1))
        return true;
    else
        return false;
}

bool IsPointToDestOK_TradePath(const GameWorldBase& gwb, const MapCoord x, const MapCoord y, const unsigned char dir, const void* param)
{
    // Feld passierbar?
    // Nicht über Wasser, Lava, Sümpfe gehen
    if(!gwb.IsNodeToNodeForFigure(x, y, (dir + 3) % 6))
        return false;

    // Not trough hostile territory?
    unsigned char old_player = gwb.GetNode(gwb.GetXA(x, y, (dir + 3) % 6), gwb.GetYA(x, y, (dir + 3) % 6)).owner,
                  new_player = gwb.GetNode(x, y).owner;
    // Ally or no player? Then ok
    if(new_player == 0 || gwb.GetPlayer(*((unsigned char*)param))->IsAlly(new_player - 1))
        return true;
    else
    {
        // Old player also evil?
        if(old_player != 0 && !gwb.GetPlayer(*((unsigned char*)param))->IsAlly(old_player - 1))
            return true;
        else
            return false;
    }
}


/// Find a route for trade caravanes
unsigned char GameWorldGame::FindTradePath(const Point<MapCoord> start,
        const Point<MapCoord> dest, const unsigned char player, const unsigned max_route, const bool random_route,
        std::vector<unsigned char>* route, unsigned* length,
        const bool record) const
{
    //unsigned tt = GetTickCount();
    //static unsigned cc = 0;
    //++cc;

    unsigned char pp = GetNode(dest.x, dest.y).owner;
    if(!(pp == 0 || GetPlayer(player)->IsAlly(pp - 1)))
        return 0xff;
    bool is_warehouse_at_goal = false;
    if(GetNO(dest.x, dest.y)->GetType() == NOP_BUILDING)
    {
        if(GetSpecObj<noBuilding>(dest.x, dest.y)->IsWarehouse())
            is_warehouse_at_goal = true;
    }

    if(!IsNodeForFigures(dest.x, dest.y) && !is_warehouse_at_goal )
        return 0xff;

    unsigned char first_dir = 0xFF;
    FindFreePath(start.x, start.y, dest.x, dest.y, random_route, max_route, route, length, &first_dir, IsPointOK_TradePath,
                 IsPointToDestOK_TradePath, &player, record);

    //if(GetTickCount()-tt > 100)
    //  printf("%u: %u ms; (%u,%u) to (%u,%u)\n",cc,GetTickCount()-tt,start.x,start.y,dest.x,dest.y);

    return first_dir;
}

/// Check whether trade path is still valid
bool GameWorldGame::CheckTradeRoute(const Point<MapCoord> start, const std::vector<unsigned char>& route,
                                    const unsigned pos, const unsigned char player,
                                    Point<MapCoord>* dest) const
{
    return CheckFreeRoute(start.x, start.y, route, pos, IsPointOK_TradePath,
                          IsPointToDestOK_HumanPath, dest ? &dest->x : NULL, dest ? &dest->y : NULL, &player);
}
