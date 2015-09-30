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

#ifndef TRADE_GRAPH_H_
#define TRADE_GRAPH_H_

#include "gameTypes/MapTypes.h"
#include "gameData/GameConsts.h"
#include "TradeGraphNode.h"
#include <vector>

class GameWorldGame;
class SerializedGameData;
class GameClientPlayerList;
class TradeRoute;

/// Maximum route length for pathfinding
const MapCoord TG_PF_LENGTH = 2 * TGN_SIZE;

class TradeGraph
{
        friend class TradeRoute;

        // Reference to the game world
        const GameWorldGame* const gwg;
        /// Player which uses the graph
        const unsigned char player;
        /// Size of the graph
        MapPoint size;
        /// The trade graph consisting of big squares
        std::vector<TradeGraphNode> trade_graph;

        /// Finds a main point for a speciefic node
        void FindMainPoint(const MapPoint tgn);

    public:

        TradeGraph(const unsigned char player, const GameWorldGame* const gwg);
        TradeGraph(SerializedGameData& sgd, const GameWorldGame* const gwg);

        void Serialize(SerializedGameData& sgd) const;

        /// Creates a new complete graph
        void Create();
        /// Creates the graph at the beginning of the game using the data of the graph of another player
        void CreateWithHelpOfAnotherPlayer(const TradeGraph& helper, const GameClientPlayerList& players);

        /// Returns a speciefic TradeGraphNode
        TradeGraphNode& GetNode(const MapPoint pos) { return trade_graph[pos.y * size.x + pos.x]; }
        const TradeGraphNode& GetNode(const MapPoint pos) const { return trade_graph[pos.y * size.x + pos.x]; }

        /// Returns to coordinate of the node around this node
        /// (Directions 1-8 (incl), 0 = no change)
        MapPoint GetNodeAround(const MapPoint pos, const unsigned char dir) const;

        /// Updates one speciefic edge
        void UpdateEdge(MapPoint pos, const unsigned char dir, const TradeGraph* const tg);
        /// Find a path on the Trade Graph
        bool FindPath(const MapPoint start, const MapPoint goal, std::vector<unsigned char>& route) const;
};


#endif
