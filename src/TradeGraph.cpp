#include <stdafx.h>
#include "main.h"
#include "TradeGraph.h"
#include "GameWorld.h"



/// Gets the next direction the caravane has to take
unsigned char TradeRoute::GetNextDir()
{
	// Test the route in the trade graph
	for(unsigned i = global_pos;i<global_route.size();++i)
	{
		unsigned char next_dir = global_route[i];
		Point<MapCoord> pos = TradeGraphNode::ConverToTGCoords(current_pos);
		// Next direction not possible?
		if(!tg->GetNode(pos).dirs[next_dir])
			return RecalcGlobalRoute();
	}

	// Check the current local route
	if(!tg->gwg->CheckTradeRoute(current_pos,local_route,local_pos,tg->player))
	{
		// Not valid, recalc it
		return RecalcLocalRoute();
	}

	unsigned char next_dir = local_route[local_pos];

	// Next step
	if(++local_pos >= local_route.size())
	{
		local_pos = 0;
		++global_pos;
		RecalcLocalRoute();
	}

	return next_dir;

}

/// Recalc local route and returns next direction
unsigned char TradeRoute::RecalcLocalRoute()
{
	return 0;
}

/// Recalc the whole route and returns next direction
unsigned char TradeRoute::RecalcGlobalRoute()
{
	return 0;
}