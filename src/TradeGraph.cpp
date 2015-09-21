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
#include "TradeGraph.h"
#include "GameWorld.h"
#include "SerializedGameData.h"

TradeGraph::TradeGraph(const unsigned char player, const GameWorldGame* const gwg)
    : gwg(gwg), player(player), size(gwg->GetWidth() / TGN_SIZE, gwg->GetHeight() / TGN_SIZE)
{
    if(gwg->GetWidth() % TGN_SIZE > 0) ++size.x;
    if(gwg->GetHeight() % TGN_SIZE > 0) ++size.y;
    trade_graph.resize(size.x * size.y);
}

/// Creates a new complete graph
void TradeGraph::Create()
{
    for(MapPoint pt(0, 0); pt.y < size.y; ++pt.y)
        for(pt.x = 0; pt.x < size.x; ++pt.x)
            FindMainPoint(pt);

    for(MapPoint pt(0, 0); pt.y < size.y; ++pt.y)
        for(pt.x = 0; pt.x < size.x; ++pt.x)
        {
            for(unsigned d = 0; d < 8; ++d)
                UpdateEdge(pt, d, NULL);
        }
}

TradeGraph::TradeGraph(SerializedGameData* sgd, const GameWorldGame* const gwg) :
    gwg(gwg), player(sgd->PopUnsignedChar()), size(sgd->PopMapPoint()),
    trade_graph(size.x* size.y)
{
    for(unsigned i = 0; i < trade_graph.size(); ++i)
        trade_graph[i].Deserialize(sgd);
}

void TradeGraph::Serialize(SerializedGameData* sgd) const
{
    sgd->PushUnsignedChar(player);
    sgd->PushMapPoint(size);
    for(unsigned i = 0; i < trade_graph.size(); ++i)
        trade_graph[i].Serialize(sgd);
}

/// Creates the graph at the beginning of the game using the data of the graph of another player
void TradeGraph::CreateWithHelpOfAnotherPlayer(const TradeGraph& helper, const GameClientPlayerList& players)
{
    for(MapPoint pt(0, 0); pt.y < size.y; ++pt.y)
        for(pt.x = 0; pt.x < size.x; ++pt.x)
        {
            MapPoint p(pt);
            // Player hqs far away from this point?
            unsigned nearest_hq = std::numeric_limits<unsigned>::max();
            for(unsigned i = 0; i < players.getCount(); ++i)
            {
                unsigned new_distance = gwg->CalcDistance(helper.GetNode(p).main_pos, players[i].hqPos);

                if(new_distance < nearest_hq)
                    nearest_hq = new_distance;
            }

            if(nearest_hq >= TGN_SIZE * 2)
                GetNode(p) = helper.GetNode(p);
            else
                FindMainPoint(p);
        }
    for(MapPoint pt(0, 0); pt.y < size.y; ++pt.y)
        for(pt.x = 0; pt.x < size.x; ++pt.x)
        {
            MapPoint p(pt);
            // Player hqs far away from this point?
            unsigned nearest_hq = std::numeric_limits<unsigned>::max();
            for(unsigned i = 0; i < players.getCount(); ++i)
            {
                unsigned new_distance = gwg->CalcDistance(helper.GetNode(p).main_pos, players[i].hqPos);

                if(new_distance < nearest_hq)
                    nearest_hq = new_distance;
            }

            if(nearest_hq < TGN_SIZE * 2)
                for(unsigned d = 0; d < 8; ++d)
                    UpdateEdge(pt, d, &helper);
        }
}

/// Returns to coordinate of the node around this node
/// (Directions 1-8 (incl), 0 = no change)
MapPoint TradeGraph::GetNodeAround(const MapPoint pos, const unsigned char dir) const
{
    Point<int> cpos = Point<int>(pos.x, pos.y);
    Point<int> new_pos(cpos);
    switch(dir)
    {
        case 1: new_pos = Point<int>(cpos.x-1, cpos.y); break;
        case 2: new_pos = Point<int>(cpos.x-1, cpos.y-1); break;
        case 3: new_pos = Point<int>(cpos.x, cpos.y-1); break;
        case 4: new_pos = Point<int>(cpos.x+1, cpos.y-1); break;
        case 5: new_pos = Point<int>(cpos.x+1, cpos.y); break;
        case 6: new_pos = Point<int>(cpos.x+1, cpos.y+1); break;
        case 7: new_pos = Point<int>(cpos.x, cpos.y+1); break;
        case 8: new_pos = Point<int>(cpos.x-1, cpos.y+1); break;
        default: break;
    }

    // Consider map borders
    if(new_pos.x < 0) new_pos.x += size.x;
    if(new_pos.x >= size.x) new_pos.x -= size.x;
    if(new_pos.y < 0) new_pos.y += size.y;
    if(new_pos.y >= size.y) new_pos.y -= size.y;

    return MapPoint(new_pos.x, new_pos.y);
}

struct TGN
{
    unsigned route_length; /// Length of the route consisting of "turn" directions
    unsigned real_length; /// Length of the route in real steps on the map
    unsigned char dir;
    bool visited;
    bool in_list;

    TGN() : route_length(std::numeric_limits<unsigned>::max()), real_length(std::numeric_limits<unsigned>::max()), visited(false), in_list(false) {}
};



/// Find a path on the Trade Graph
bool TradeGraph::FindPath(const MapPoint start, const MapPoint goal, std::vector<unsigned char>& route) const
{
    // Todo list
    std::list< MapPoint > todo;
    todo.push_back(start);

    std::vector<TGN> nodes(size.x * size.y);

    nodes[start.y * size.x + start.x].route_length = 0;
    nodes[start.y * size.x + start.x].real_length = 0;

    while(!todo.empty())
    {
        unsigned shortest_route = 0xFFFFFFFF;

        std::list<MapPoint >::iterator best_it;

        for(std::list<MapPoint >::iterator it = todo.begin(); it != todo.end(); ++it)
        {
            unsigned new_way = nodes[it->y * size.x + it->x].real_length + TGN_SIZE;
            if(new_way < shortest_route)
            {
                shortest_route = new_way;
                best_it = it;
            }
        }

        nodes[best_it->y * size.x + best_it->x].visited = true;


        for(unsigned i = 0; i < 8; ++i)
        {
            if(GetNode(*best_it).dirs[i] == NO_EDGE)
                continue;

            MapPoint new_pos(GetNodeAround(*best_it, i + 1));

            if(nodes[new_pos.y * size.x + new_pos.x].visited)
                continue;

            if(new_pos == goal)
            {
                // Reached goal
                route.resize(nodes[best_it->y * size.x + best_it->x].route_length + 1);
                route[route.size() - 1] = i;

                MapPoint pos = *best_it;

                for(int z = route.size() - 2; z >= 0; --z, pos = GetNodeAround(pos, (nodes[pos.y * size.x + pos.x].dir + 4) % 8 + 1))
                    route[z] = nodes[pos.y * size.x + pos.x].dir;

                return true;
            }

            unsigned new_length = nodes[best_it->y * size.x + best_it->x].real_length + GetNode(*best_it).dirs[i];

            if(new_length < nodes[new_pos.y * size.x + new_pos.x].real_length)
            {
                nodes[new_pos.y * size.x + new_pos.x].real_length = new_length;
                nodes[new_pos.y * size.x + new_pos.x].dir = i;
                nodes[new_pos.y * size.x + new_pos.x].route_length = nodes[best_it->y * size.x + best_it->x].route_length + 1;
                if(!nodes[new_pos.y * size.x + new_pos.x].in_list)
                {
                    nodes[new_pos.y * size.x + new_pos.x].in_list = true;
                    todo.push_back(new_pos);
                }
            }
        }

        // Knoten behandelt --> raus aus der todo Liste
        todo.erase(best_it);

    }

    return false;
}


/// Finds a main point for a speciefic node
void TradeGraph::FindMainPoint(const MapPoint tgn)
{
    /// Calc size of this node rectangle
    MapCoord width, height;
    if(tgn.x == gwg->GetWidth() / TGN_SIZE || gwg->GetWidth() % TGN_SIZE == 0)
        width = gwg->GetWidth() % TGN_SIZE;
    else
        width = TGN_SIZE;
    if(tgn.y == gwg->GetHeight() / TGN_SIZE || gwg->GetHeight() % TGN_SIZE == 0)
        height = gwg->GetHeight() % TGN_SIZE;
    else
        height = TGN_SIZE;

    const unsigned POTENTIAL_MAIN_POINTS = 5;


    // We consider the following points as main points
    MapPoint ps[POTENTIAL_MAIN_POINTS] =
    {
        MapPoint(tgn.x* TGN_SIZE + width / 2, tgn.y* TGN_SIZE + height / 2),

        MapPoint(tgn.x* TGN_SIZE + width / 4, tgn.y* TGN_SIZE + height / 4),
        MapPoint(tgn.x* TGN_SIZE + width * 3 / 4, tgn.y* TGN_SIZE + height / 4),
        MapPoint(tgn.x* TGN_SIZE + width * 3 / 4, tgn.y* TGN_SIZE + height * 3 / 4),
        MapPoint(tgn.x* TGN_SIZE + width / 4, tgn.y* TGN_SIZE + height * 3 / 4)
    };

    bool good_points[POTENTIAL_MAIN_POINTS];

    for(unsigned i = 0; i < POTENTIAL_MAIN_POINTS; ++i)
    {
        MapPoint p = ps[i];
        good_points[i] = gwg->IsNodeForFigures(p);

        // Valid point? Otherwise choose one around this one
        if(!gwg->IsNodeForFigures(p))
        {
            for(unsigned d = 0; d < 6; ++d)
            {
                MapPoint pt(gwg->GetNeighbour(p, d));
                if(gwg->IsNodeForFigures(pt))
                {
                    ps[i] = pt;
                    good_points[i] = true;
                    break;
                }
            }
        }
    }

    // Try to find paths to the other points if we reach at least 3/4 of the other points, choose
    // this point at once, otherwise choose the point with most connections
    unsigned best_connections = 0, best_id = 0;
    for(unsigned i = 0; i < POTENTIAL_MAIN_POINTS; ++i)
    {
        MapPoint p = ps[i];
        unsigned connections = 0;
        for(unsigned j = 0; j < POTENTIAL_MAIN_POINTS; ++j)
        {
            if(i == j || !good_points[j]) continue;

            unsigned char next_dir = gwg->FindTradePath(p, ps[j], player, TG_PF_LENGTH, false, NULL, NULL, false);
            if(next_dir != 0xff) ++connections;
        }

        if(connections >= (POTENTIAL_MAIN_POINTS - 1) * 3 / 4)
        {
            best_id = i;
            break;
        }

        if(connections >= best_connections)
        {
            best_id = i;
            best_connections = connections;
        }
    }

    GetNode(tgn).main_pos = ps[best_id];

}


/// Updates one speciefic edge
void TradeGraph::UpdateEdge(MapPoint pos, const unsigned char dir, const TradeGraph* const tg)
{
    if(tg)
        if(tg->GetNode(pos).dont_run_over_player_territory[dir])
        {
            GetNode(pos).dont_run_over_player_territory[dir] = true;
            GetNode(pos).dirs[dir] = tg->GetNode(pos).dirs[dir];
            return;
        }
    MapPoint other = GetNodeAround(pos, dir + 1);
    unsigned char other_dir = (dir + 4) % 8;
    if(GetNode(other).dont_run_over_player_territory[other_dir])
    {
        GetNode(pos).dont_run_over_player_territory[dir] = true;
        GetNode(pos).dirs[dir] = GetNode(other).dirs[other_dir];
        return;
    }


    unsigned length;
    std::vector<unsigned char> route;
    MapPoint mpos(GetNode(pos).main_pos);
    // Simply try to find a path from one main point to another
    if(gwg->FindTradePath(mpos, GetNode(other).main_pos,
                          player, TG_PF_LENGTH, false, &route, &length) == 0xff)
        length = NO_EDGE;
    GetNode(pos).dirs[dir] = static_cast<MapCoord>(length);

    bool hasOwner = false;
    for(unsigned i = 0; i < route.size(); ++i)
    {
        mpos = gwg->GetNeighbour(mpos, route[i]);
        if(gwg->GetNode(mpos).owner != 0)
            hasOwner = true;
    }

    if(!hasOwner)
        GetNode(pos).dont_run_over_player_territory[dir] = true;

}