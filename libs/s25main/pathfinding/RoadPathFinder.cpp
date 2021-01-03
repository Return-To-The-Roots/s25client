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

#include "RoadPathFinder.h"
#include "EventManager.h"
#include "RttrForeachPt.h"
#include "buildings/nobHarborBuilding.h"
#include "pathfinding/OpenListPrioQueue.h"
#include "pathfinding/OpenListVector.h"
#include "world/GameWorldBase.h"
#include "nodeObjs/noRoadNode.h"
#include "gameData/GameConsts.h"
#include "s25util/Log.h"

/// Comparison operator for road nodes that returns true if lhs > rhs (descending order)
struct RoadNodeComperatorGreater
{
    bool operator()(const noRoadNode* const lhs, const noRoadNode* const rhs) const
    {
        if(lhs->estimate == rhs->estimate)
        {
            // Wenn die Wegkosten gleich sind, vergleichen wir die Koordinaten, da wir für std::set eine streng
            // monoton steigende Folge brauchen
            return (lhs->GetObjId() > rhs->GetObjId());
        }

        return (lhs->estimate > rhs->estimate);
    }
};

using QueueImpl = OpenListPrioQueue<const noRoadNode*, RoadNodeComperatorGreater>;
using VecImpl = OpenListVector<const noRoadNode*>;
VecImpl todo;

// Namespace with all functors usable as additional cost functors
namespace AdditonalCosts {
struct None
{
    unsigned operator()(const noRoadNode&, const Direction) const { return 0; }
};

struct Carrier
{
    unsigned operator()(const noRoadNode& curNode, const Direction nextDir) const
    {
        // Add costs for busy carriers to allow alternative routes
        return curNode.GetPunishmentPoints(nextDir);
    }
};
} // namespace AdditonalCosts

// Namespace with all functors usable as segment constraint functors
namespace SegmentConstraints {
struct None
{
    bool operator()(const RoadSegment&) const { return true; }
};

/// Disallows a specific road segment
struct AvoidSegment
{
    const RoadSegment* const forbiddenSeg_;
    AvoidSegment(const RoadSegment* const forbiddenSeg) : forbiddenSeg_(forbiddenSeg) {}

    bool operator()(const RoadSegment& segment) const { return forbiddenSeg_ != &segment; }
};

/// Disallows a specific road type
template<RoadType T_roadType>
struct AvoidRoadType
{
    bool operator()(const RoadSegment& segment) const { return segment.GetRoadType() != T_roadType; }
};

/// Combines 2 functors by returning true only if both of them return true
/// Can be chained
template<class T_Func1, class T_Func2>
struct And : private T_Func1, private T_Func2
{
    using Func1 = T_Func1;
    using Func2 = T_Func2;

    And() : Func1(), Func2() {}

    template<typename T>
    And(const T& p1) : Func1(p1), Func2()
    {}

    template<typename T, typename U>
    And(const T& p1, const U& p2) : Func1(p1), Func2(p2)
    {}

    And(const Func2& f2) : Func1(), Func2(f2) {}

    bool operator()(const RoadSegment& segment) const
    {
        return Func1::operator()(segment) && Func2::operator()(segment);
    }
};
} // namespace SegmentConstraints

/// Wegfinden ( A* ), O(v lg v) --> Wegfindung auf Stra�en
template<class T_AdditionalCosts, class T_SegmentConstraints>
bool RoadPathFinder::FindPathImpl(const noRoadNode& start, const noRoadNode& goal, const unsigned max,
                                  const T_AdditionalCosts addCosts, const T_SegmentConstraints isSegmentAllowed,
                                  unsigned* const length, RoadPathDirection* const firstDir,
                                  MapPoint* const firstNodePos)
{
    if(&start == &goal)
    {
        // Path where start==goal should never happen
        RTTR_Assert(false);
        LOG.write("WARNING: Bug detected (GF: %u). Please report this with the savegame and replay (Start==Goal in "
                  "pathfinding %u,%u)\n")
          % gwb_.GetEvMgr().GetCurrentGF() % unsigned(start.GetX()) % unsigned(start.GetY());
        // But for now we assume it to be valid and return (kind of) correct values
        if(length)
            *length = 0;
        if(firstDir)
            *firstDir = RoadPathDirection::None;
        if(firstNodePos)
            *firstNodePos = start.GetPos();
        return true;
    }

    // increase current_visit_on_roads, so we don't have to clear the visited-states at every run
    currentVisit++;

    // if the counter reaches its maximum, tidy up
    if(currentVisit == std::numeric_limits<unsigned>::max())
    {
        RTTR_FOREACH_PT(MapPoint, gwb_.GetSize())
        {
            auto* const node = gwb_.GetSpecObj<noRoadNode>(pt);
            if(node)
                node->last_visit = 0;
        }
        currentVisit = 1;
    }

    // Anfangsknoten einf�gen
    todo.clear();

    start.targetDistance = gwb_.CalcDistance(start.GetPos(), goal.GetPos());
    start.estimate = start.targetDistance;
    start.last_visit = currentVisit;
    start.prev = nullptr;
    start.cost = 0;
    start.dir_ = RoadPathDirection::None;

    todo.push(&start);

    while(!todo.empty())
    {
        // Knoten mit den geringsten Wegkosten ausw�hlen
        const noRoadNode& best = *todo.pop();

        // Ziel erreicht?
        if(&best == &goal)
        {
            // Jeweils die einzelnen Angaben zur�ckgeben, falls gew�nscht (Pointer �bergeben)
            if(length)
                *length = best.cost;

            // Backtrace to get the last node that is not the start node (has a prev node) --> Next node from start on
            // path
            const noRoadNode* firstNode = &best;
            while(firstNode->prev != &start)
            {
                firstNode = firstNode->prev;
            }

            if(firstDir)
                *firstDir = firstNode->dir_;

            if(firstNodePos)
                *firstNodePos = firstNode->GetPos();

            // Done, path found
            return true;
        }

        // Nachbarflagge bzw. Wege in allen 6 Richtungen verfolgen
        for(const auto dir : helpers::EnumRange<Direction>{})
        {
            // Gibt es auch einen solchen Weg bzw. Nachbarflagge?
            noRoadNode* neighbour = best.GetNeighbour(dir);

            // Wenn nicht, brauchen wir mit dieser Richtung gar nicht weiter zu machen
            if(!neighbour)
                continue;

            // this eliminates 1/6 of all nodes and avoids cost calculation and further checks,
            // therefore - and because the profiler says so - it is more efficient that way
            if(neighbour == best.prev)
                continue;

            // No pathes over buildings
            if((dir == Direction::NorthWest) && (neighbour != &goal))
            {
                // Flags and harbors are allowed
                const GO_Type got = neighbour->GetGOT();
                if(got != GO_Type::Flag && got != GO_Type::NobHarborbuilding)
                    continue;
            }

            // evtl verboten?
            if(!isSegmentAllowed(*best.GetRoute(dir)))
                continue;

            // Neuer Weg für diesen neuen Knoten berechnen
            unsigned cost = best.cost + best.GetRoute(dir)->GetLength();
            cost += addCosts(best, dir);

            if(cost > max)
                continue;

            // Was node already visited?
            if(neighbour->last_visit == currentVisit)
            {
                // Dann nur ggf. Weg und Vorg�nger korrigieren, falls der Weg k�rzer ist
                if(cost < neighbour->cost)
                {
                    neighbour->cost = cost;
                    neighbour->prev = &best;
                    neighbour->estimate = neighbour->targetDistance + cost;
                    todo.rearrange(neighbour);
                    neighbour->dir_ = toRoadPathDirection(dir);
                }
            } else
            {
                // Not visited yet -> Add to list
                neighbour->last_visit = currentVisit;
                neighbour->cost = cost;
                neighbour->dir_ = toRoadPathDirection(dir);
                neighbour->prev = &best;

                neighbour->targetDistance = gwb_.CalcDistance(neighbour->GetPos(), goal.GetPos());
                neighbour->estimate = neighbour->targetDistance + cost;

                todo.push(neighbour);
            }
        }

        // Stehen wir hier auf einem Hafenplatz
        if(best.GetGOT() == GO_Type::NobHarborbuilding)
        {
            std::vector<nobHarborBuilding::ShipConnection> scs =
              static_cast<const nobHarborBuilding&>(best).GetShipConnections();

            for(auto& sc : scs)
            {
                // Neuer Weg für diesen neuen Knoten berechnen
                unsigned cost = best.cost + sc.way_costs;

                if(cost > max)
                    continue;

                noRoadNode& dest = *sc.dest;
                // Was node already visited?
                if(dest.last_visit == currentVisit)
                {
                    // Dann nur ggf. Weg und Vorg�nger korrigieren, falls der Weg k�rzer ist
                    if(cost < dest.cost)
                    {
                        dest.dir_ = RoadPathDirection::Ship;
                        dest.cost = cost;
                        dest.prev = &best;
                        dest.estimate = dest.targetDistance + cost;
                        todo.rearrange(&dest);
                    }
                } else
                {
                    // Not visited yet -> Add to list
                    dest.last_visit = currentVisit;

                    dest.dir_ = RoadPathDirection::Ship;
                    dest.prev = &best;
                    dest.cost = cost;

                    dest.targetDistance = gwb_.CalcDistance(dest.GetPos(), goal.GetPos());
                    dest.estimate = dest.targetDistance + cost;

                    todo.push(&dest);
                }
            }
        }
    }

    // Liste leer und kein Ziel erreicht --> kein Weg
    return false;
}

bool RoadPathFinder::FindPath(const noRoadNode& start, const noRoadNode& goal, const bool wareMode, const unsigned max,
                              const RoadSegment* const forbidden, unsigned* const length,
                              RoadPathDirection* const firstDir, MapPoint* const firstNodePos)
{
    RTTR_Assert(length || firstDir || firstNodePos); // If none of them is set use the \ref PathExist function!

    if(wareMode)
    {
        if(forbidden)
            return FindPathImpl(start, goal, max, AdditonalCosts::Carrier(),
                                SegmentConstraints::AvoidSegment(forbidden), length, firstDir, firstNodePos);
        else
            return FindPathImpl(start, goal, max, AdditonalCosts::Carrier(), SegmentConstraints::None(), length,
                                firstDir, firstNodePos);
    } else
    {
        if(forbidden)
            return FindPathImpl(start, goal, max, AdditonalCosts::None(),
                                SegmentConstraints::And<SegmentConstraints::AvoidSegment,
                                                        SegmentConstraints::AvoidRoadType<RoadType::Water>>(forbidden),
                                length, firstDir, firstNodePos);
        else
            return FindPathImpl(start, goal, max, AdditonalCosts::None(),
                                SegmentConstraints::AvoidRoadType<RoadType::Water>(), length, firstDir, firstNodePos);
    }
}

bool RoadPathFinder::PathExists(const noRoadNode& start, const noRoadNode& goal, const bool allowWaterRoads,
                                const unsigned max, const RoadSegment* const forbidden)
{
    if(allowWaterRoads)
    {
        if(forbidden)
            return FindPathImpl(start, goal, max, AdditonalCosts::None(), SegmentConstraints::AvoidSegment(forbidden));
        else
            return FindPathImpl(start, goal, max, AdditonalCosts::None(), SegmentConstraints::None());
    } else
    {
        if(forbidden)
            return FindPathImpl(start, goal, max, AdditonalCosts::None(),
                                SegmentConstraints::And<SegmentConstraints::AvoidSegment,
                                                        SegmentConstraints::AvoidRoadType<RoadType::Water>>(forbidden));
        else
            return FindPathImpl(start, goal, max, AdditonalCosts::None(),
                                SegmentConstraints::AvoidRoadType<RoadType::Water>());
    }
}
