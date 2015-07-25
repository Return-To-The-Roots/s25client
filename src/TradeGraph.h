#ifndef TRADE_GRAPH_H_
#define TRADE_GRAPH_H_

#include "gameTypes/MapTypes.h"
#include "gameData/GameConsts.h"
#include "Point.h"

#include <memory.h>
#include <vector>

class GameWorldGame;
class TradeGraph;
class GameClientPlayerList;
class SerializedGameData;

/// Size of such a TradeGraphNode
const MapCoord TGN_SIZE = 20;
/// Maximum route length for pathfinding
const MapCoord TG_PF_LENGTH = 2 * TGN_SIZE;

/// Constants used for Pathfinding
const unsigned char NO_PATH = 0xff;
const unsigned char REACHED_GOAL = 0xdd;
const MapCoord NO_EDGE = 0xffff;

struct TradeGraphNode
{
    /// Point of the node, representing the main node
    MapPoint main_pos;
    /// Possible 8 directions with way costs
    MapCoord dirs[8];
    /// Direction not possible, even in the future (water, lava, swamp etc.)
    bool not_possible_forever[8];
    /// Is the route running over any player territory?
    bool dont_run_over_player_territory[MAX_PLAYERS];

    TradeGraphNode() : main_pos(0xffff, 0xffff)
    {
        for(unsigned i = 0; i < 8; ++i) dirs[i] = NO_EDGE;
        memset(dont_run_over_player_territory, 0, MAX_PLAYERS * sizeof(bool));
    }

    void Deserialize(SerializedGameData* sgd);
    void Serialize(SerializedGameData* sgd) const;

    /// Converts map coords to TG coords
    static MapPoint ConverToTGCoords(const MapPoint pos)
    {
        MapPoint ret(pos.x / TGN_SIZE, pos.y / TGN_SIZE);
        return ret;
    }

    void operator=(const TradeGraphNode& tgn)
    {
        main_pos = tgn.main_pos;
        for(unsigned i = 0; i < 8; ++i)
        {
            dirs[i] = tgn.dirs[i];
            not_possible_forever[i] = tgn.not_possible_forever[i];
        }
    }
};

class TradeRoute
{
        /// Reference to the trade graph
        const TradeGraph* const tg;

        /// Start and goal, current posistion in usual map coordinates and TG coordinates
        MapPoint start, goal, current_pos, current_pos_tg;
        /// Current "global" route on the trade graph
        std::vector<unsigned char> global_route;
        unsigned global_pos;
        /// Current "local" route from one main point to another main point
        std::vector<unsigned char> local_route;
        unsigned local_pos;
    private:

        /// Recalc local route and returns next direction
        unsigned char RecalcLocalRoute();
        /// Recalc the whole route and returns next direction
        unsigned char RecalcGlobalRoute();

    public:

        TradeRoute(const TradeGraph* const tg, const MapPoint start, const MapPoint goal) :
            tg(tg), start(start), goal(goal), current_pos(start), global_pos(0), local_pos(0) { RecalcGlobalRoute(); }
        TradeRoute(SerializedGameData* sgd, const GameWorldGame* const gwg, const unsigned char player);

        void Serialize(SerializedGameData* sgd) const;


        /// Was a route found?
        bool IsValid() const
        { return local_route.size() > 0; }
        /// Gets the next direction the caravane has to take
        unsigned char GetNextDir();
        /// Assigns new start and goal positions and hence, a new route
        void AssignNewGoal(const MapPoint new_goal, const MapPoint current);


};

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

    private:

        /// Finds a main point for a speciefic node
        void FindMainPoint(const MapPoint tgn);


    public:

        TradeGraph(const unsigned char player, const GameWorldGame* const gwg);
        TradeGraph(SerializedGameData* sgd, const GameWorldGame* const gwg);

        void Serialize(SerializedGameData* sgd) const;

        /// Creates a new complete graph
        void Create();
        /// Creates the graph at the beginning of the game using the data of the graph of another player
        void CreateWithHelpOfAnotherPlayer(const TradeGraph& helper, const GameClientPlayerList& players);

        /// Returns a speciefic TradeGraphNode
        TradeGraphNode& GetNode(const MapPoint pos)
        { return trade_graph[pos.y * size.x + pos.x]; }
        const TradeGraphNode& GetNode(const MapPoint pos) const
        { return trade_graph[pos.y * size.x + pos.x]; }


        /// Returns to coordinate of the node around this node
        /// (Directions 1-8 (incl), 0 = no change)
        MapPoint GetNodeAround(const MapPoint pos, const unsigned char dir) const;


        /// Updates one speciefic edge
        void UpdateEdge(MapPoint pos, const unsigned char dir, const TradeGraph* const tg);
        /// Find a path on the Trade Graph
        bool FindPath(const MapPoint start, const MapPoint goal, std::vector<unsigned char>& route) const;
};


#endif
