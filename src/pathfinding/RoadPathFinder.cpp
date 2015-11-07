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
#include "Log.h"

#include <queue>

template<
    class _Ty, 
    class _Container = std::vector<_Ty>, 
    class _Pr = std::less<typename _Container::value_type>
>
class OpenListPrioQueue : public std::priority_queue<_Ty,   _Container,  _Pr>
{
    typedef std::priority_queue<_Ty,   _Container,  _Pr> Parent;
public:
    typedef typename std::vector<_Ty>::iterator iterator;

    OpenListPrioQueue(): Parent()
    {
        Parent::c.reserve(255);
    }

    void rearrange(const _Ty& target)
    {
        iterator it = std::find(Parent::c.begin(), Parent::c.end(), target);
        rearrange(it);
    }

    void rearrange(iterator it)
    {
        std::push_heap(Parent::c.begin(), it + 1, Parent::comp);
    }

    iterator find(const _Ty& target)
    {
        return std::find(Parent::c.begin(), Parent::c.end(), target);
    }

    void clear()
    {
        Parent::c.clear();
    }

    /// Removes and returns the first element
    _Ty pop()
    {
        _Ty result = Parent::top();
        Parent::pop();
        return result;
    }
};

template<class T>
class OpenListVector
{
    std::vector<T> elements;
public:
    typedef typename std::vector<T>::iterator iterator;

    OpenListVector()
    {
        elements.reserve(255);
    }

    T pop()
    {
        assert(!empty());
        const int size = static_cast<int>(elements.size());
        if (size == 1)
        {
            T best = elements.front();
            elements.clear();
            return best;
        }
        int bestIdx = 0;
        unsigned bestEstimate = elements.front()->estimate;
        for (int i = 1; i < size; i++)
        {
            // Note that this check does not consider nodes with the same value
            // However this is a) correct (same estimate = same quality so no preference from the algorithm)
            // and b) still fully deterministic as the entries are NOT sorted and the insertion-extraction-pattern
            // is completely pre-determined by the graph-structur
            const unsigned estimate = elements[i]->estimate;
            if (estimate < bestEstimate)
            {
                bestEstimate = estimate;
                bestIdx = i;
            }
        }
        T best = elements[bestIdx];
        elements[bestIdx] = elements[size - 1];
        elements.resize(size - 1);
        return best;
    }

    void clear()
    {
        elements.clear();
    }

    bool empty()
    {
        return elements.empty();
    }

    void push(T el)
    {
        elements.push_back(el);
    }

    size_t size() const
    {
        return elements.size();
    }

    void rearrange(const T& target) {}
};

/// Comparison operator for road nodes that returns true if lhs > rhs (descending order)
struct RoadNodeComperatorGreater
{
    bool operator()(const noRoadNode* const lhs, const noRoadNode* const rhs) const
    {
        if (lhs->estimate == rhs->estimate)
        {
            // Wenn die Wegkosten gleich sind,  vergleichen wir die Koordinaten,  da wir für std::set eine streng
            // monoton steigende Folge brauchen
            return (lhs->GetObjId() > rhs->GetObjId());
        }

        return (lhs->estimate > rhs->estimate);
    }
};

typedef OpenListPrioQueue<const noRoadNode*, std::vector<const noRoadNode*>, RoadNodeComperatorGreater> QueueImpl;
typedef OpenListVector<const noRoadNode*> VecImpl;
VecImpl todo;

/// Wegfinden ( A* ),  O(v lg v) --> Wegfindung auf Straßen
bool RoadPathFinder::FindPath(const noRoadNode& start, const noRoadNode& goal, 
                                    const bool ware_mode, unsigned* length, 
                                    unsigned char* first_dir, MapPoint* next_harbor, 
                                    const RoadSegment* const forbidden, const bool record, const unsigned max)
{
    assert(&start && &goal);
    if(&start == &goal)
    {
        // Path where start==goal should never happen
        assert(false);
        LOG.lprintf("WARNING: Bug detected (GF: %u). Please report this with the savegame and replay (Start==Goal in pathfinding %u,%u)\n", GAMECLIENT.GetGFNumber(), unsigned(start.GetX()), unsigned(start.GetY()));
        // But for now we assume it to be valid and return (kind of) correct values
        if(length)
            *length = 0;
        if(first_dir)
            *first_dir = 0xff;
        if(next_harbor)
            *next_harbor = start.GetPos();
        return true;
    }

    // Aus Replay lesen?
    if(record && GAMECLIENT.ArePathfindingResultsAvailable())
    {
        unsigned char dir;
        if(GAMECLIENT.ReadPathfindingResult(&dir,  length,  next_harbor))
        {
            if(first_dir)
                *first_dir = dir;
            return (dir != 0xff);
        }
    }

    // increase current_visit_on_roads,  so we don't have to clear the visited-states at every run
    currentVisit++;

    // if the counter reaches its maxium,  tidy up
    if (currentVisit == std::numeric_limits<unsigned>::max())
    {
        int w = gwb_.GetWidth();
        int h = gwb_.GetHeight();
        for(int y = 0; y < h; y++)
        {
            for(int x = 0; x < w; x++)
            {
                noRoadNode* const node = gwb_.GetSpecObj<noRoadNode>(MapPoint(x, y));
                if(node)
                    node->last_visit = 0;
            }
        }
        currentVisit = 1;
    }

    // Anfangsknoten einfügen
    todo.clear();

    start.targetDistance = gwb_.CalcDistance(start.GetPos(),  goal.GetPos());
    start.estimate = start.targetDistance;
    start.last_visit = currentVisit;
    start.prev = NULL;
    start.cost = 0;
    start.dir_ = 0;

    todo.push(&start);

    while (!todo.empty())
    {
        // Knoten mit den geringsten Wegkosten auswählen
        const noRoadNode* best = todo.pop();

        // Ziel erreicht?
        if (best == &goal)
        {
            // Jeweils die einzelnen Angaben zurückgeben,  falls gewünscht (Pointer übergeben)
            if (length)
                *length = best->cost;

            // Backtrace to get the last node that is not the start node (has a prev node) --> Next node from start on path
            const noRoadNode* last = best;
            while(best->prev)
            {
                last = best;
                best = best->prev;
            }

            if (first_dir)
                *first_dir = (unsigned char) last->dir_;

            if (next_harbor)
                *next_harbor = last->GetPos();

            if (record)
            {
                GAMECLIENT.AddPathfindingResult((unsigned char) last->dir_,  length,  next_harbor);
            }

            // Done, path found
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

            // No pathes over buildings
            if ((i == 1) && (neighbour != &goal))
            {
                // Flags and harbors are allowed
                const GO_Type got = neighbour->GetGOT();
                if(got != GOT_FLAG && got != GOT_NOB_HARBORBUILDING)
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

            // Was node already visited?
            if (neighbour->last_visit == currentVisit)
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
            }else
            {
                // Not visited yet -> Add to list
                neighbour->last_visit = currentVisit;
                neighbour->cost = cost;
                neighbour->dir_ = i;
                neighbour->prev = best;

                neighbour->targetDistance = gwb_.CalcDistance(neighbour->GetPos(),  goal.GetPos());
                neighbour->estimate = neighbour->targetDistance + cost;

                todo.push(neighbour);
            }
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

                noRoadNode& dest = *scs[i].dest;
                // Was node already visited?
                if (dest.last_visit == currentVisit)
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
                }else
                {
                    // Not visited yet -> Add to list
                    dest.last_visit = currentVisit;

                    dest.dir_ = 100;
                    dest.prev = best;
                    dest.cost = cost;

                    dest.targetDistance = gwb_.CalcDistance(dest.GetPos(),  goal.GetPos());
                    dest.estimate = dest.targetDistance + cost;

                    todo.push(&dest);
                }
            }
        }
    }

    // Liste leer und kein Ziel erreicht --> kein Weg
    if(record)
        GAMECLIENT.AddPathfindingResult(0xff,  length,  next_harbor);
    return false;
}