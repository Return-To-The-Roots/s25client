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
#include "GameWorldGame.h"
#include "TradeRoute.h"
#include "TradeGraph.h"
#include "SerializedGameData.h"
#include "gameData/GameConsts.h"

TradeRoute::TradeRoute(SerializedGameData& sgd, const GameWorldGame* const gwg, const unsigned char player) :
    tg(gwg->GetTradeGraph(player)),
    start(sgd.PopMapPoint()),
    goal(sgd.PopMapPoint()),
    current_pos(sgd.PopMapPoint()),
    current_pos_tg(sgd.PopMapPoint())
{
    global_route.resize(sgd.PopUnsignedInt());
    for(unsigned i = 0; i < global_route.size(); ++i)
        global_route[i] = sgd.PopUnsignedChar();
    global_pos = sgd.PopUnsignedInt();

    local_route.resize(sgd.PopUnsignedInt());
    for(unsigned i = 0; i < local_route.size(); ++i)
        local_route[i] = sgd.PopUnsignedChar();
    local_pos = sgd.PopUnsignedInt();
}

void TradeRoute::Serialize(SerializedGameData& sgd) const
{
    sgd.PushMapPoint(start);
    sgd.PushMapPoint(goal);
    sgd.PushMapPoint(current_pos);
    sgd.PushMapPoint(current_pos_tg);

    sgd.PushUnsignedInt(global_route.size());
    for(unsigned i = 0; i < global_route.size(); ++i)
        sgd.PushUnsignedChar(global_route[i]);
    sgd.PushUnsignedInt(global_pos);

    sgd.PushUnsignedInt(local_route.size());
    for(unsigned i = 0; i < local_route.size(); ++i)
        sgd.PushUnsignedChar(local_route[i]);
    sgd.PushUnsignedInt(local_pos);
}


/// Gets the next direction the caravane has to take
unsigned char TradeRoute::GetNextDir()
{
    if(local_route.empty())
        return INVALID_DIR;

    if(current_pos == tg->gwg->GetNeighbour(goal, 1))
    {
        return REACHED_GOAL;
    }

    // Test the route in the trade graph
    for(unsigned i = global_pos; i < global_route.size(); ++i)
    {
        unsigned char next_dir = global_route[i];
        MapPoint pos = TradeGraphNode::ConverToTGCoords(current_pos);
        // Next direction not possible?
        if(!tg->GetNode(pos).dirs[next_dir])
            return RecalcGlobalRoute();
    }

    // Check the current local route
    if(!tg->gwg->CheckTradeRoute(current_pos, local_route, local_pos, tg->player))
    {
        // Not valid, recalc it
        return RecalcLocalRoute();
    }

    unsigned char next_dir = local_route[local_pos];
    current_pos = tg->gwg->GetNeighbour(current_pos, next_dir);

    // Next step
    if(++local_pos >= local_route.size())
    {
        local_pos = 0;
        if(global_pos < global_route.size())
            current_pos_tg = tg->GetNodeAround(current_pos_tg, global_route[global_pos] + 1);
        ++global_pos;
        RecalcLocalRoute();
    }

    return next_dir;
}

/// Recalc local route and returns next direction
unsigned char TradeRoute::RecalcLocalRoute()
{
    /// Are we at the flag of the goal?
    if(current_pos == goal)
    {
        local_route.resize(1);
        local_route[0] = 1;
        return 1;
    }

    unsigned char next_dir;
    if(global_pos >= global_route.size() || tg->gwg->CalcDistance(current_pos, goal) < TGN_SIZE / 2)
    {
        // Global route over or are we near the goal? Then find a (real) path to our goal
        global_pos = global_route.size();
        next_dir = tg->gwg->FindTradePath(current_pos, goal, tg->player, TG_PF_LENGTH, false, &local_route);
        local_pos = 0;
    }
    else
    {
        next_dir = tg->gwg->FindTradePath(current_pos, tg->GetNode(current_pos_tg).main_pos, tg->player, TG_PF_LENGTH, false, &local_route);
        local_pos = 0;
    }
    return next_dir;
}

/// Recalc the whole route and returns next direction
unsigned char TradeRoute::RecalcGlobalRoute()
{
    local_pos = 0;
    global_pos = 0;
    // TG node where we start
    MapPoint start_tgn  = current_pos_tg = TradeGraphNode::ConverToTGCoords(start);
    // Try to calc paths to the main point and - if this doesn't work - to the mainpoints of the surrounded nodes
    unsigned char next_dir;
    for(unsigned char i = 0; i <= 8; ++i)
    {
        // Try to find path
        start_tgn = tg->GetNodeAround(current_pos_tg, i);
        next_dir = tg->gwg->FindTradePath(current_pos, tg->GetNode(start_tgn).main_pos, tg->player, TG_PF_LENGTH, false, &local_route);
        // Found a path? Then abort the loop
        if(next_dir != INVALID_DIR)
            break;
    }

    // Didn't find any paths? Then bye
    if(next_dir == INVALID_DIR)
        return INVALID_DIR;

    // The same for the last path main_point-->destination
    // TG node where we end
    MapPoint goal_tgn = TradeGraphNode::ConverToTGCoords(goal);
    MapPoint goal_tgn_tmp = goal_tgn;
    // Try to calc paths to the main point and - if this doesn't work - to the mainpoints of the surrounded nodes
    for(unsigned char i = 0; i <= 8; ++i)
    {
        // Try to find path
        goal_tgn = tg->GetNodeAround(goal_tgn_tmp, i);
        next_dir = tg->gwg->FindTradePath(tg->GetNode(goal_tgn).main_pos, goal, tg->player, TG_PF_LENGTH, false);
        // Found a path? Then abort the loop
        if(next_dir != INVALID_DIR)
            break;
    }
    // Didn't find any paths? Then bye
    if(next_dir == INVALID_DIR)
        return INVALID_DIR;


    // Pathfinding on the TradeGraph
    if(!tg->FindPath(start_tgn, goal_tgn, global_route))
    {
        local_route.clear();
        return INVALID_DIR;
    }

    return local_route[0];
}

/// Assigns new start and goal positions and hence, a new route
void TradeRoute::AssignNewGoal(const MapPoint new_goal, const MapPoint current)
{
    this->start = current;
    this->goal = new_goal;
    RecalcGlobalRoute();
}
