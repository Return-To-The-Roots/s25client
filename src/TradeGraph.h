#ifndef TRADE_GRAPH_H_
#define TRADE_GRAPH_H_

#include "MapConsts.h"

class GameWorldGame;
class TradeGraph;

/// Size of such a TradeGraphNode
const MapCoord TGN_SIZE = 20;

struct TradeGraphNode
{
	/// Point of the node, representing the main node 
	Point<MapCoord> main_pos;
	/// Possible 8 directions with way costs
	unsigned dirs[8];
	/// Direction not possible, even in the future (water, lava, swamp etc.)
	bool not_possible_forever[8];

	/// Converts map coords to TG coords
	static Point<MapCoord> ConverToTGCoords(const Point<MapCoord> pos)
	{
		Point<MapCoord> ret(pos.x / TGN_SIZE, pos.y / TGN_SIZE);
		return ret;
	}

	
};

class TradeRoute
{
	/// Reference to the trade graph
	const TradeGraph * const tg;

	/// Start and goal, current posistion
	Point<MapCoord> start,goal,current_pos;
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

	TradeRoute(const TradeGraph * const tg ) : tg(tg) {}

	/// Gets the next direction the caravane has to take
	unsigned char GetNextDir();
	

};

class TradeGraph
{
	friend class TradeRoute;

	// Reference to the game world
	const GameWorldGame * const gwg;
	/// Player which uses the graph
	const unsigned player;
	/// Size of the graph
	Point<MapCoord> size;
	/// The trade graph consisting of big squares
	std::vector<TradeGraphNode> trade_graph;

public:

	TradeGraph(const unsigned char player,const GameWorldGame * const gwg) : player(player), gwg(gwg) {}

	/// Returns a speciefic TradeGraphNode
	TradeGraphNode& GetNode(const Point<MapCoord> pos)
	{ return trade_graph[pos.y*size.x+pos.x]; }
	const TradeGraphNode& GetNode(const Point<MapCoord> pos) const
	{ return trade_graph[pos.y*size.x+pos.x]; }

	/// Deletes the old graph and creates a new one
	void Create();
	/// Updates one speciefic edge
	void UpdateEdge(const Point<MapCoord> pos, const unsigned char dir);
	/// Finds a path from one point to another point and returns a TradeRoute 
	bool FindPath(const Point<MapCoord> start, const Point<MapCoord> goal, TradeRoute * &tr);
};


#endif