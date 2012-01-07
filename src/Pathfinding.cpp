// $Id: Pathfinding.cpp 7763 2012-01-07 00:57:12Z marcus $
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
	NewNode() : way(0), dir(0), prev(INVALID_PREV), /*visited(false),*/ lastVisited(0), destination(NULL) {}

	/// Wegkosten, die vom Startpunkt bis zu diesem Knoten bestehen
	unsigned way;
	/// Die Richtung, über die dieser Knoten erreicht wurde
	unsigned char dir;
	/// ID (gebildet aus y*Kartenbreite+x) des Vorgänngerknotens
	unsigned prev;
	/// Iterator auf Position in der Prioritätswarteschlange (std::set), freies Pathfinding
	std::set<PathfindingPoint>::iterator it_p;
	/// Wurde Knoten schon besucht (für A*-Algorithmus), wenn lastVisited == currentVisit
	unsigned lastVisited;
	// Aktuelles Pathfinding-Ziel (zum Unterscheiden der Richtung)
	noRoadNode *destination;
};

const unsigned maxMapSize = 1024;
/// Die Knoten der Map gespeichert, größtmöglichste Kartengröße nehmen
NewNode pf_nodes[maxMapSize*maxMapSize];
unsigned currentVisit = 0;

/// Punkte als Verweise auf die obengenannen Knoten, damit nur die beiden Koordinaten x,y im set mit rumgeschleppt
/// werden müsen
struct PathfindingPoint {
public:
	/// Die beiden Koordinaten des Punktes
	MapCoord x,y;
	unsigned distance;

public:
	/// Konstruktoren
	PathfindingPoint(const MapCoord sx, const MapCoord sy)
	{
		x = sx;
		y = sy;
		distance = gwb->CalcDistance(x,y,dst_x,dst_y);
	}

	/// Koordinaten des Ziels beim jeweils aktuellen Pathfinding, die wir zur Bewertung der Punkte benötigen
	static MapCoord dst_x, dst_y;
	/// Pointer auf GameWorld, die wir brauchen, um die IDs zu berechnen bzw. die Kartengröße zu bekommen
	static const GameWorldBase * gwb;
	/// Diese statischen Variablen zu Beginn des Pathfindings festlegen
	static void Init(const MapCoord dst_x, const MapCoord dst_y,const GameWorldBase * gwb)
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
		unsigned way1 = pf_nodes[gwb->MakeCoordID(x,y)].way + distance;
		unsigned way2 = pf_nodes[gwb->MakeCoordID(two.x,two.y)].way + two.distance;

		// Wenn die Wegkosten gleich sind, vergleichen wir die Koordinaten, da wir für std::set eine streng
		// monoton steigende Folge brauchen
		if(way1 == way2)
			return (gwb->MakeCoordID(x,y) < gwb->MakeCoordID(two.x,two.y) );
		else
			return (way1<way2);
	}
};

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

/// Definitionen siehe oben
MapCoord PathfindingPoint::dst_x = 0;
MapCoord PathfindingPoint::dst_y = 0;
const GameWorldBase * PathfindingPoint::gwb = NULL;

/// Wegfinden ( A* ), O(v lg v) --> Wegfindung auf allgemeinen Terrain (ohne Straßen), für Wegbau und frei herumlaufende Berufe
bool GameWorldBase::FindFreePath(const MapCoord x_start,const MapCoord y_start,
				  const MapCoord x_dest, const MapCoord y_dest, const bool random_route, 
				  const unsigned max_route, std::vector<unsigned char> * route, unsigned *length,
				  unsigned char * first_dir,  FP_Node_OK_Callback IsNodeOK, FP_Node_OK_Callback IsNodeToDestOk, const void * param, const bool record) const
{
	// increase currentVisit, so we don't have to clear the visited-states at every run
	currentVisit++;

	// if the counter reaches its maxium, tidy up
	if (currentVisit == std::numeric_limits<unsigned>::max() - 1)
	{
		for (unsigned i = 0; i < (maxMapSize*maxMapSize); ++i)
		{
			pf_nodes[i].lastVisited = 0;
		}
		currentVisit = 1;
	}

	std::set<PathfindingPoint> todo;
	PathfindingPoint::Init(x_dest,y_dest,this);

	// Anfangsknoten einfügen
	unsigned start_id = MakeCoordID(x_start,y_start);
	std::pair< std::set<PathfindingPoint>::iterator, bool > ret = todo.insert(PathfindingPoint(x_start,y_start));
	// Und mit entsprechenden Werten füllen
	pf_nodes[start_id].it_p = ret.first;
	pf_nodes[start_id].prev = INVALID_PREV;
	pf_nodes[start_id].lastVisited = currentVisit;
	pf_nodes[start_id].way = 0;
	pf_nodes[start_id].dir = 0;

	while(todo.size())
	{
		// Knoten mit den geringsten Wegkosten auswählen
		PathfindingPoint best = *todo.begin();
		// Knoten behandelt --> raus aus der todo Liste
		todo.erase(todo.begin());
		
		//printf("x: %u y: %u\n",best.x,best.y);

		// ID des besten Punktes ausrechnen

		unsigned best_id = MakeCoordID(best.x,best.y);

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
			for(unsigned z = pf_nodes[best_id].way-1;best_id!=start_id;--z,best_id = pf_nodes[best_id].prev)
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
		unsigned start = random_route?RANDOM.Rand("pf",__LINE__,y_start*GetWidth()+x_start,6):0;
		
		// Knoten in alle 6 Richtungen bilden
		for(unsigned z = start;z<start+6;++z)
		{
			unsigned i = (z+3)%6;

			// Koordinaten des entsprechenden umliegenden Punktes bilden
			MapCoord xa = GetXA(best.x,best.y,i),
				ya = GetYA(best.x,best.y,i);

			// ID des umliegenden Knotens bilden
			unsigned xaid = MakeCoordID(xa,ya);

			// Knoten schon auf dem Feld gebildet?
			if (pf_nodes[xaid].lastVisited == currentVisit)
			{
				// Dann nur ggf. Weg und Vorgänger korrigieren, falls der Weg kürzer ist
				if(pf_nodes[xaid].it_p != todo.end() && pf_nodes[best_id].way+1 < pf_nodes[xaid].way)
				{
					pf_nodes[xaid].way  = pf_nodes[best_id].way+1;
					pf_nodes[xaid].prev = best_id;
					todo.erase(pf_nodes[xaid].it_p);
					ret = todo.insert(PathfindingPoint(xa,ya));
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
				if(!IsNodeOK(*this,xa,ya,i,param))
					continue;
			}

			// Zusätzliche Bedingungen, auch die das letzte Stück zum Ziel betreffen
			if(IsNodeToDestOk)
			{
				if(!IsNodeToDestOk(*this,xa,ya,i,param))
					continue;
			}

			// Alles in Ordnung, Knoten kann gebildet werden
			pf_nodes[xaid].lastVisited = currentVisit;
			pf_nodes[xaid].way = pf_nodes[best_id].way+1;
			pf_nodes[xaid].dir = i;
			pf_nodes[xaid].prev = best_id;

			ret = todo.insert(PathfindingPoint(xa,ya));
			pf_nodes[xaid].it_p = ret.first;
		}
	}

	// Liste leer und kein Ziel erreicht --> kein Weg
	return false;
}

template<class _Ty,	
	class _Container = std::vector<_Ty>,	
	class _Pr = std::less<typename _Container::value_type> >
class openlist_container : public std::priority_queue<_Ty,	_Container,	_Pr> 
{
public:
	openlist_container()
		: std::priority_queue<_Ty,	_Container,	_Pr>()
	{
		std::priority_queue<_Ty,	_Container,	_Pr>::c.reserve(255);
	}
	
	void rearrange(const _Ty& target)
	{
		typename std::vector<_Ty>::iterator it = std::find(std::priority_queue<_Ty,	_Container,	_Pr>::c.begin(), std::priority_queue<_Ty,	_Container,	_Pr>::c.end(), target);
		std::push_heap(std::priority_queue<_Ty,	_Container,	_Pr>::c.begin(), it+1, std::priority_queue<_Ty,	_Container,	_Pr>::comp);
	}

	void clear()
	{
		std::priority_queue<_Ty,	_Container,	_Pr>::c.clear();
	}
};


openlist_container<const noRoadNode*, std::vector<const noRoadNode*>, RoadNodeComperatorInv> s2d, d2s;

/*
Pathfinding fun: how does this work?
- search is performed from start to destination AND destination to start
- the direction with the smallest open list is next
  - this is done to keep the open list at minimal size
- a path that may be found isn't necessarily the best
  - keep searching with a new limit of max_way = best_length - 1
  - best path is: res (direction), max_way (length - 1), next_id (id for next flag)
- if there isn't an element left in any of the two open lists: check results (res == 0xFF)

Comparism of CPU ticks per pathfinding call (averaged over ~8000 GF):

      total     self
old  447217   171610
new  159424    67220
fac     2.8     2.55
*/

/// Wegfindung auf allgemeinen Terrain (ohne Straßen), für Wegbau und frei herumlaufende Berufe
bool GameWorldBase::FindPathOnRoads(const noRoadNode * const start, const noRoadNode * const destination,
									const bool ware_mode, unsigned * length, 
									unsigned char * first_dir,  Point<MapCoord> * next_harbor,
									const RoadSegment * const forbidden, const bool record, unsigned max_way) const
{
	// Aus Replay lesen?
	if(GameClient::inst().ArePathfindingResultsAvailable() && record)
	{
		unsigned char dir;
		if(GameClient::inst().ReadPathfindingResult(&dir,length,next_harbor))
		{
			if(first_dir) *first_dir = dir;
			return (dir != 0xff);
		}
	}

	// Irgendwelche Null-Anfänge oder Ziele? --> Kein Weg
	if(!start || !destination)
	{
		if(record)
			GameClient::inst().AddPathfindingResult(0xff,length,next_harbor);
		return false;
	}

	// increase currentVisit, so we don't have to clear the visited-states at every run
	currentVisit++;

	// if the counter reaches its maxium, tidy up
	if (currentVisit == std::numeric_limits<unsigned>::max() - 1)
	{
		for (unsigned i = 0; i < (maxMapSize*maxMapSize); ++i)
		{
			pf_nodes[i].lastVisited = 0;
		}
		currentVisit = 1;
	}

	unsigned distance = CalcDistance(start->GetX(), start->GetY(), destination->GetX(), destination->GetY());

	s2d.clear();

	start->distance = start->estimate = distance;

	s2d.push(start);

	unsigned start_id = start->coord_id;

	pf_nodes[start_id].destination = (noRoadNode *) destination;
	pf_nodes[start_id].prev = INVALID_PREV;
	pf_nodes[start_id].lastVisited = currentVisit;
	pf_nodes[start_id].way = 0;
	pf_nodes[start_id].dir = 0;

	d2s.clear();

	destination->distance = destination->estimate = distance;

	d2s.push(destination);

	unsigned destination_id = destination->coord_id;

	pf_nodes[destination_id].destination = (noRoadNode *) start;
	pf_nodes[destination_id].prev = INVALID_PREV;
	pf_nodes[destination_id].lastVisited = currentVisit;
	pf_nodes[destination_id].way = 0;
	pf_nodes[destination_id].dir = 0;

//	fprintf(stderr, "FIND PATH: %u,%u -> %u,%u\n", start->GetX(), start->GetY(), destination->GetX(), destination->GetY());

	unsigned char res = 0xFF;
	unsigned next_id = 0;

	while (true)
	{
		unsigned other_way;
		noRoadNode *current_destination;
		openlist_container<const noRoadNode*, std::vector<const noRoadNode*>, RoadNodeComperatorInv> *todo;

		if (d2s.size() < s2d.size())
		{
			if (d2s.size() == 0)
			{
				break;
			}

			todo = &d2s;
			current_destination = (noRoadNode *) start;
			other_way = pf_nodes[s2d.top()->coord_id].way;
		} else
		{
			if (s2d.size() == 0)
			{
				break;
			}

			todo = &s2d;
			current_destination = (noRoadNode *) destination;
			other_way = pf_nodes[d2s.top()->coord_id].way;
		}

		const noRoadNode *best = todo->top();

		// Knoten behandelt --> raus aus der todo Liste
		todo->pop();

		// Nachbarflagge bzw. Wege in allen 6 Richtungen verfolgen
		for (unsigned i = 0; i < 6; ++i)
		{
			// Gibt es auch einen solchen Weg bzw. Nachbarflagge?
			noRoadNode *rna = best->GetNeighbour(i);

			// Wenn nicht, brauchen wir mit dieser Richtung gar nicht weiter zu machen
			if (!rna)
				continue;

			// Neuer Weg für diesen neuen Knoten berechnen
			unsigned new_way = 0;

			// Im Warenmodus müssen wir Strafpunkte für überlastete Träger hinzuaddieren,
			// damit der Algorithmus auch Ausweichrouten auswählt
			if (ware_mode)
			{
				new_way += best->GetPunishmentPoints(i);
			} else if (best->routes[i]->GetRoadType() == RoadSegment::RT_BOAT)	// evtl Wasserstraße?
			{
				continue;
			}

			// evtl verboten?
			if (best->routes[i] == forbidden)
				continue;

			new_way += pf_nodes[best->coord_id].way + best->routes[i]->GetLength();

			if (new_way + other_way > max_way)
				continue;

			// ID des umliegenden Knotens
			unsigned xaid = rna->coord_id;

			// Knoten schon auf dem Feld gebildet?
			if (pf_nodes[xaid].lastVisited == currentVisit)
			{
				// "collision detection"
				if (current_destination != pf_nodes[xaid].destination)
				{
					if (pf_nodes[best->coord_id].way + pf_nodes[xaid].way + new_way > max_way)
					{
						// we found a new way that's worse than what we've already found
						continue;
					}

					// limit our search to only find new ways with a better score
					max_way = pf_nodes[best->coord_id].way + pf_nodes[xaid].way + new_way - 1;

					// find last node of start -> destination pathfinding
					unsigned id = (current_destination == destination) ? best->coord_id : xaid;

					if (id == start_id)	// special case: what we just found is our first road segment
					{
						// we have to reverse destination -> start directions
						res = (current_destination == destination) ? i : (i + 3) % 6;
						next_id = (current_destination == destination) ? xaid : best->coord_id;
						continue;
					}

					for (; id != start_id; id = pf_nodes[id].prev)
					{
						res = pf_nodes[id].dir;
						next_id = id;
					}

					continue;
				}

				// Dann nur ggf. Weg und Vorgänger korrigieren, falls der Weg kürzer ist
				if (new_way < pf_nodes[xaid].way)
				{
					pf_nodes[xaid].way  = new_way;
					pf_nodes[xaid].prev = best->coord_id;
					rna->estimate = rna->distance + new_way;
					todo->rearrange(rna);
					pf_nodes[xaid].dir = i;
				}

				continue;
			}

			// Alles in Ordnung, Knoten kann gebildet werden
			pf_nodes[xaid].lastVisited = currentVisit;
			pf_nodes[xaid].way = new_way;
			pf_nodes[xaid].dir = i;
			pf_nodes[xaid].prev = best->coord_id;
			pf_nodes[xaid].destination = current_destination;

			rna->distance = CalcDistance(rna->GetX(), rna->GetY(), current_destination->GetX(), current_destination->GetY());
			rna->estimate = rna->distance + new_way;

			todo->push(rna);
		}

		// Stehen wir hier auf einem Hafenplatz
		if (best->GetGOT() == GOT_NOB_HARBORBUILDING)
		{
			std::vector<nobHarborBuilding::ShipConnection> scs;
			static_cast<const nobHarborBuilding*>(best)->GetShipConnections(scs);

			for (unsigned i = 0; i < scs.size(); ++i)
			{
				// ID des umliegenden Knotens bilden
				unsigned xaid = scs[i].dest->coord_id;

				// Neuer Weg für diesen neuen Knoten berechnen
				unsigned new_way = pf_nodes[best->coord_id].way  + scs[i].way_costs;

				if (new_way + other_way > max_way)
					continue;

				// Knoten schon auf dem Feld gebildet?
				if (pf_nodes[xaid].lastVisited == currentVisit)
				{
					// "collision detection"
					if (current_destination != pf_nodes[xaid].destination)
					{
						if (pf_nodes[best->coord_id].way + pf_nodes[xaid].way + new_way > max_way)
						{
							// we found a new way that's worse than what we've already found
							continue;
						}

						// limit our search to only find new ways with a better score
						max_way = pf_nodes[best->coord_id].way + pf_nodes[xaid].way + new_way - 1;

						// find last node of start -> destination pathfinding
						unsigned id = (current_destination == destination) ? best->coord_id : xaid;

						if (id == start_id)	// special case: what we just found is our first road segment
						{
							// go by ship :)
							res = 100;
							next_id = (current_destination == destination) ? xaid : best->coord_id;
							continue;
						}

						for (; id != start_id; id = pf_nodes[id].prev)
						{
							res = pf_nodes[id].dir;
							next_id = id;
						}

						continue;
					}

					// Dann nur ggf. Weg und Vorgänger korrigieren, falls der Weg kürzer ist
					if (new_way < pf_nodes[xaid].way)
					{
						pf_nodes[xaid].way  = new_way;
						pf_nodes[xaid].prev = best->coord_id;
						scs[i].dest->estimate = scs[i].dest->distance + new_way;
						todo->rearrange(scs[i].dest);
						pf_nodes[xaid].dir = 100;
					}

					continue;
				}

				// Alles in Ordnung, Knoten kann gebildet werden
				pf_nodes[xaid].lastVisited = currentVisit;
				pf_nodes[xaid].way = new_way;
				pf_nodes[xaid].dir = 100;
				pf_nodes[xaid].prev = best->coord_id;
				pf_nodes[xaid].destination = current_destination;

				scs[i].dest->distance = CalcDistance(scs[i].dest->GetX(), scs[i].dest->GetY(), current_destination->GetX(), current_destination->GetY());
				scs[i].dest->estimate = scs[i].dest->distance + new_way;

				todo->push(scs[i].dest);
			}
		}
	}

	if (res == 0xFF)
	{
		if (record)
		{
			GameClient::inst().AddPathfindingResult(0xff,length,next_harbor);
		}

		return false;
	}

	// we found a path, this means that max_way has been set to length -1 and res is our result.

	if (first_dir)
		*first_dir = res;

	if (length)
		*length = max_way + 1;

	if (next_harbor)
	{
		next_harbor->x = next_id % width;
		next_harbor->y = next_id / width;
	}

	if (record)
	{
		GameClient::inst().AddPathfindingResult(res, length, next_harbor);
	}

	return(true);
}

/// Ermittelt, ob eine freie Route noch passierbar ist und gibt den Endpunkt der Route zurück
bool GameWorldBase::CheckFreeRoute(const MapCoord x_start,const MapCoord y_start, const std::vector<unsigned char>& route, const unsigned pos, 
	FP_Node_OK_Callback IsNodeOK, FP_Node_OK_Callback IsNodeToDestOk,  MapCoord* x_dest, MapCoord* y_dest, const void * const param) const
{
	MapCoord x = x_start, y = y_start;

	for(unsigned i = pos;i<route.size();++i)
	{
		GetPointA(x,y,route[i]);
		if(!IsNodeToDestOk(*this,x,y,route[i],param))
			return false;
		if(i < route.size()-1 && !IsNodeOK(*this,x,y,route[i],param))
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
bool IsPointOK_RoadPath(const GameWorldBase& gwb, const MapCoord x, const MapCoord y, const unsigned char dir, const void *param)
{
	const Param_RoadPath * prp = static_cast<const Param_RoadPath*>(param);

	// Feld bebaubar?
	if(!gwb.RoadAvailable(prp->boat_road,x,y,dir))
		return false;
	// Auch auf unserem Territorium?
	if(!gwb.IsPlayerTerritory(x,y))
		return false;

	return true;
}

/// Straßenbau-Pathfinding
bool GameWorldViewer::FindRoadPath(const MapCoord x_start,const MapCoord y_start, const MapCoord x_dest, const MapCoord y_dest,std::vector<unsigned char>& route, const bool boat_road)
{
	Param_RoadPath prp = { boat_road };
	return FindFreePath(x_start,y_start,x_dest,y_dest,false,100,&route,NULL,NULL,IsPointOK_RoadPath,NULL, &prp,false);
}

/// Abbruch-Bedingungen für freien Pfad für Menschen
bool IsPointOK_HumanPath(const GameWorldBase& gwb, const MapCoord x, const MapCoord y, const unsigned char dir, const void *param)
{
	// Feld passierbar?
	noBase::BlockingManner bm = gwb.GetNO(x,y)->GetBM();
	if(bm != noBase::BM_NOTBLOCKING && bm != noBase::BM_TREE && bm != noBase::BM_FLAG)
		return false;

	return true;
}



/// Zusätzliche Abbruch-Bedingungen für freien Pfad für Menschen, die auch bei der letzen Kante
/// zum Ziel eingehalten werden müssen
bool IsPointToDestOK_HumanPath(const GameWorldBase& gwb, const MapCoord x, const MapCoord y, const unsigned char dir, const void *param)
{
	// Feld passierbar?
	// Nicht über Wasser, Lava, Sümpfe gehen
	if(!gwb.IsNodeToNodeForFigure(x,y,(dir+3)%6))
		return false;

	return true;
}

/// Abbruch-Bedingungen für freien Pfad für Schiffe
bool IsPointOK_ShipPath(const GameWorldBase& gwb, const MapCoord x, const MapCoord y, const unsigned char dir, const void *param)
{
	// Ein Meeresfeld?
	for(unsigned i = 0;i<6;++i)
	{
		if(gwb.GetTerrainAround(x,y,i) != TT_WATER)
			return false;
	}

	return true;
}

/// Zusätzliche Abbruch-Bedingungen für freien Pfad für Schiffe, die auch bei der letzen Kante
/// zum Ziel eingehalten werden müssen
bool IsPointToDestOK_ShipPath(const GameWorldBase& gwb, const MapCoord x, const MapCoord y, const unsigned char dir, const void *param)
{
	// Der Übergang muss immer aus Wasser sein zu beiden Seiten
	if(gwb.GetWalkingTerrain1(x,y,(dir+3)%6) == TT_WATER && gwb.GetWalkingTerrain2(x,y,(dir+3)%6) == TT_WATER)
		return true;
	else
		return false;
}

/// Findet einen Weg für Figuren
unsigned char GameWorldBase::FindHumanPath(const MapCoord x_start,const MapCoord y_start,
			const MapCoord x_dest, const MapCoord y_dest, const unsigned max_route, const bool random_route, unsigned *length, const bool record) const
{
	// Aus Replay lesen?
	if(GameClient::inst().ArePathfindingResultsAvailable() && !random_route)
	{
		unsigned char dir;
		if(GameClient::inst().ReadPathfindingResult(&dir,length,NULL))
			return dir;
	}
	
	unsigned char first_dir = 0xFF;
	FindFreePath(x_start,y_start,x_dest,y_dest,random_route,max_route,NULL,length,&first_dir,IsPointOK_HumanPath,
		IsPointToDestOK_HumanPath,NULL,record);
		
	if(!random_route)
	GameClient::inst().AddPathfindingResult(first_dir,length,NULL);	
	
	return first_dir;

}

/// Wegfindung für Menschen im Straßennetz
unsigned char GameWorldGame::FindHumanPathOnRoads(const noRoadNode * const start, const noRoadNode * const goal,unsigned * length, Point<MapCoord> * next_harbor, const RoadSegment * const forbidden)
{
	unsigned char first_dir = 0xFF;
	if(FindPathOnRoads(start, goal, false, length, &first_dir, next_harbor, forbidden))
		return first_dir;
	else
		return 0xFF;
}

/// Wegfindung für Waren im Straßennetz
unsigned char GameWorldGame::FindPathForWareOnRoads(const noRoadNode * const start, const noRoadNode * const goal,unsigned * length, Point<MapCoord> * next_harbor, unsigned max)
{
	unsigned char first_dir = 0xFF;
	if(FindPathOnRoads(start, goal, true, length, &first_dir, next_harbor, NULL, true, max))
		return first_dir;
	else
		return 0xFF;
}


/// Wegfindung für Schiffe auf dem Wasser
bool GameWorldBase::FindShipPath(const MapCoord x_start,const MapCoord y_start, const MapCoord x_dest,
								 const MapCoord y_dest, std::vector<unsigned char> * route, unsigned * length, const unsigned max_length,
								 GameWorldBase::CrossBorders * cb)
{
	return FindFreePath(x_start,y_start,x_dest,y_dest,true,400,route,length,NULL,IsPointOK_ShipPath,
		IsPointToDestOK_ShipPath,NULL,false);
}

/// Prüft, ob eine Schiffsroute noch Gültigkeit hat
bool GameWorldGame::CheckShipRoute(const MapCoord x_start,const MapCoord y_start, const std::vector<unsigned char>& route, const unsigned pos, 
		 MapCoord* x_dest,  MapCoord* y_dest)
{
	return CheckFreeRoute(x_start,y_start,route,pos,IsPointOK_ShipPath,
		IsPointToDestOK_ShipPath,x_dest,y_dest,NULL);
}


/// Abbruch-Bedingungen für freien Pfad für Menschen
bool IsPointOK_TradePath(const GameWorldBase& gwb, const MapCoord x, const MapCoord y, const unsigned char dir, const void *param)
{
	// Feld passierbar?
	noBase::BlockingManner bm = gwb.GetNO(x,y)->GetBM();
	if(bm != noBase::BM_NOTBLOCKING && bm != noBase::BM_TREE && bm != noBase::BM_FLAG)
		return false;

	
	unsigned char player = gwb.GetNode(x,y).owner;
	// Ally or no player? Then ok
	if(player == 0 || gwb.GetPlayer(*((unsigned char*)param))->IsAlly(player-1)) 
		return true;
	else
		return false;
}

bool IsPointToDestOK_TradePath(const GameWorldBase& gwb, const MapCoord x, const MapCoord y, const unsigned char dir, const void *param)
{
	// Feld passierbar?
	// Nicht über Wasser, Lava, Sümpfe gehen
	if(!gwb.IsNodeToNodeForFigure(x,y,(dir+3)%6))
		return false;

	// Not trough hostile territory?
	unsigned char old_player = gwb.GetNode(gwb.GetXA(x,y,(dir+3)%6),gwb.GetYA(x,y,(dir+3)%6)).owner,
		new_player = gwb.GetNode(x,y).owner;
	// Ally or no player? Then ok
	if(new_player == 0 || gwb.GetPlayer(*((unsigned char*)param))->IsAlly(new_player-1)) 
		return true;
	else
	{
		// Old player also evil?
		if(old_player != 0 && !gwb.GetPlayer(*((unsigned char*)param))->IsAlly(old_player-1)) 
			return true;
		else
			return false;
	}
}


/// Find a route for trade caravanes
unsigned char GameWorldGame::FindTradePath(const Point<MapCoord> start,
	const Point<MapCoord> dest, const unsigned char player, const unsigned max_route, const bool random_route, 
	 std::vector<unsigned char> * route, unsigned *length, 
	const bool record) const
{
	//unsigned tt = GetTickCount();
	//static unsigned cc = 0;
	//++cc;

	unsigned char pp = GetNode(dest.x,dest.y).owner;
	if(!(pp == 0 || GetPlayer(player)->IsAlly(pp-1))) 
		return 0xff;
	bool is_warehouse_at_goal = false;
	if(GetNO(dest.x,dest.y)->GetType() == NOP_BUILDING)
	{
		if(GetSpecObj<noBuilding>(dest.x,dest.y)->IsWarehouse()) 
			is_warehouse_at_goal = true;
	}

	if(!IsNodeForFigures(dest.x,dest.y) && !is_warehouse_at_goal )
		return 0xff;

	unsigned char first_dir = 0xFF;
	FindFreePath(start.x,start.y,dest.x,dest.y,random_route,max_route,route,length,&first_dir,IsPointOK_TradePath,
		IsPointToDestOK_TradePath,&player,record);

	//if(GetTickCount()-tt > 100)
	//	printf("%u: %u ms; (%u,%u) to (%u,%u)\n",cc,GetTickCount()-tt,start.x,start.y,dest.x,dest.y);
	
	return first_dir;
}

/// Check whether trade path is still valid
bool GameWorldGame::CheckTradeRoute(const Point<MapCoord> start, const std::vector<unsigned char>& route, 
									const unsigned pos, const unsigned char player,
									Point<MapCoord> * dest) const
{
	return CheckFreeRoute(start.x,start.y,route,pos,IsPointOK_TradePath,
		IsPointToDestOK_HumanPath,dest ? &dest->x : NULL,dest ? &dest->y : NULL,&player);
}
