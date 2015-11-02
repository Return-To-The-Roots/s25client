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
#include "RoadPathFinder.h"
#include "GameClient.h"
#include "GameWorldBase.h"
#include "buildings/nobHarborBuilding.h"
#include "nodeObjs/noRoadNode.h"

#include <queue>

/// Vergleichsoperator für die Prioritätswarteschlange bzw. std::set beim straßengebundenen Wegfinden
class RoadNodeComperator
{
public:
    bool operator()(const noRoadNode* const rn1,  const noRoadNode* const rn2) const;
};

class RoadNodeComperatorInv
{
public:
    bool operator()(const noRoadNode* const rn1,  const noRoadNode* const rn2) const;
};

template<class _Ty, 
class _Container = std::vector<_Ty>, 
class _Pr = std::less<typename _Container::value_type> >
class openlist_container : public std::priority_queue<_Ty,   _Container,  _Pr>
{
public:
    openlist_container()
        : std::priority_queue<_Ty,   _Container,  _Pr>()
    {
        std::priority_queue<_Ty,     _Container,  _Pr>::c.reserve(255);
    }

    void rearrange(const _Ty& target)
    {
        typename std::vector<_Ty>::iterator it = std::find(std::priority_queue<_Ty,  _Container,  _Pr>::c.begin(),  std::priority_queue<_Ty,    _Container,  _Pr>::c.end(),  target);
        std::push_heap(std::priority_queue<_Ty,  _Container,  _Pr>::c.begin(),  it + 1,  std::priority_queue<_Ty,  _Container,  _Pr>::comp);
    }

    void clear()
    {
        std::priority_queue<_Ty,     _Container,  _Pr>::c.clear();
    }
};

// a 'second' current_visit for road pathfinding
unsigned current_visit_on_roads = 0;

// Vergleichsoperator für das straßengebundene Pathfinding,  wird genauso wie das freie Pathfinding
// gehandelt,  nur dass wir noRoadNodes statt direkt Points vergleichen
bool RoadNodeComperator::operator()(const noRoadNode* const rn1,  const noRoadNode* const rn2) const
{
    if (rn1->estimate == rn2->estimate)
    {
        // Wenn die Wegkosten gleich sind,  vergleichen wir die Koordinaten,  da wir für std::set eine streng
        // monoton steigende Folge brauchen
        return (rn1->coord_id < rn2->coord_id);
    }

    return (rn1->estimate < rn2->estimate);
}

bool RoadNodeComperatorInv::operator()(const noRoadNode* const rn1,  const noRoadNode* const rn2) const
{
    if (rn1->estimate == rn2->estimate)
    {
        // Wenn die Wegkosten gleich sind,  vergleichen wir die Koordinaten,  da wir für std::set eine streng
        // monoton steigende Folge brauchen
        return (rn1->coord_id > rn2->coord_id);
    }

    return (rn1->estimate > rn2->estimate);
}

openlist_container<const noRoadNode*,  std::vector<const noRoadNode*>,  RoadNodeComperatorInv> todo;

/// Wegfinden ( A* ),  O(v lg v) --> Wegfindung auf Straßen
bool GameWorldBase::FindPathOnRoads(const noRoadNode* const start,  const noRoadNode* const goal, 
                                    const bool ware_mode,  unsigned* length, 
                                    unsigned char* first_dir,   MapPoint* next_harbor, 
                                    const RoadSegment* const forbidden,  const bool record,  unsigned max) const
{
    // Aus Replay lesen?
    if(record && GAMECLIENT.ArePathfindingResultsAvailable())
    {
        unsigned char dir;
        if(GAMECLIENT.ReadPathfindingResult(&dir,  length,  next_harbor))
        {
            if(first_dir) *first_dir = dir;
            return (dir != 0xff);
        }
    }

    // Irgendwelche Null-Anfänge oder Ziele? --> Kein Weg
    if(!start || !goal)
    {
        if(record)
            GAMECLIENT.AddPathfindingResult(0xff,  length,  next_harbor);
        return false;
    }

    // increase current_visit_on_roads,  so we don't have to clear the visited-states at every run
    current_visit_on_roads++;

    // if the counter reaches its maxium,  tidy up
    if (current_visit_on_roads == std::numeric_limits<unsigned>::max())
    {
        for (int idx = width_ * height_; idx >= 0; --idx)
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

    start->targetDistance = start->estimate = CalcDistance(start->GetPos(),  goal->GetPos());
    start->last_visit = current_visit_on_roads;
    start->prev = NULL;
    start->cost = start->dir_ = 0;

    todo.push(start);

    while (!todo.empty())
    {
        // Knoten mit den geringsten Wegkosten auswählen
        const noRoadNode* best = todo.top();

        // Knoten behandelt --> raus aus der todo Liste
        todo.pop();

        // Ziel erreicht,  allerdings keine Nullwege erlauben?
        if (best == goal && best->cost)
        {
            // Jeweils die einzelnen Angaben zurückgeben,  falls gewünscht (Pointer übergeben)
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
                *first_dir = (unsigned char) last->dir_;
            }

            if (next_harbor)
            {
                next_harbor->x = last->GetX();
                next_harbor->y = last->GetY();
            }

            // Fertig,  es wurde ein Pfad gefunden
            if (record)
            {
                GAMECLIENT.AddPathfindingResult((unsigned char) last->dir_,  length,  next_harbor);
            }

            return true;
        }

        // Nachbarflagge bzw. Wege in allen 6 Richtungen verfolgen
        for (unsigned i = 0; i < 6; ++i)
        {
            // Gibt es auch einen solchen Weg bzw. Nachbarflagge?
            noRoadNode* neighbour = best->GetNeighbour(i);

            // Wenn nicht,  brauchen wir mit dieser Richtung gar nicht weiter zu machen
            if (!neighbour)
                continue;

            // this eliminates 1/6 of all nodes and avoids cost calculation and further checks, 
            // therefore - and because the profiler says so - it is more efficient that way
            if (neighbour == best->prev)
                continue;

            // evtl verboten?
            if (best->routes[i] == forbidden)
                continue;

            // Keine Umwege über Gebäude,  ausgenommen Häfen und Ziele
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
                // Dann nur ggf. Weg und Vorgänger korrigieren,  falls der Weg kürzer ist
                if (cost < neighbour->cost)
                {
                    neighbour->cost = cost;
                    neighbour->prev = best;
                    neighbour->estimate = neighbour->targetDistance + cost;
                    todo.rearrange(neighbour);
                    neighbour->dir_ = i;
                }

                continue;
            }

            // Alles in Ordnung,  Knoten kann gebildet werden
            neighbour->last_visit = current_visit_on_roads;
            neighbour->cost = cost;
            neighbour->dir_ = i;
            neighbour->prev = best;

            neighbour->targetDistance = CalcDistance(neighbour->GetPos(),  goal->GetPos());
            neighbour->estimate = neighbour->targetDistance + cost;

            todo.push(neighbour);
        }

        // Stehen wir hier auf einem Hafenplatz
        if (best->GetGOT() == GOT_NOB_HARBORBUILDING)
        {
            std::vector<nobHarborBuilding::ShipConnection> scs = static_cast<const nobHarborBuilding*>(best)->GetShipConnections();

            for (unsigned i = 0; i < scs.size(); ++i)
            {
                // Neuer Weg für diesen neuen Knoten berechnen
                unsigned cost = best->cost + scs[i].way_costs;

                if (cost > max)
                    continue;

                // Knoten schon auf dem Feld gebildet?
                noRoadNode& dest = *scs[i].dest;
                if (dest.last_visit == current_visit_on_roads)
                {
                    // Dann nur ggf. Weg und Vorgänger korrigieren,  falls der Weg kürzer ist
                    if (cost < dest.cost)
                    {
                        dest.dir_ = 100;
                        dest.cost = cost;
                        dest.prev = best;
                        dest.estimate = dest.targetDistance + cost;
                        todo.rearrange(&dest);
                    }

                    continue;
                }

                // Alles in Ordnung,  Knoten kann gebildet werden
                dest.last_visit = current_visit_on_roads;

                dest.dir_ = 100;
                dest.prev = best;
                dest.cost = cost;

                dest.targetDistance = CalcDistance(dest.GetPos(),  goal->GetPos());
                dest.estimate = dest.targetDistance + cost;

                todo.push(&dest);
            }
        }
    }

    // Liste leer und kein Ziel erreicht --> kein Weg
    if(record)
        GAMECLIENT.AddPathfindingResult(0xff,  length,  next_harbor);
    return false;
}