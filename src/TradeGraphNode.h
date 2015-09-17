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

#ifndef TradeGraphNode_h__
#define TradeGraphNode_h__

#include "gameData/PlayerConsts.h"
#include "gameTypes/MapTypes.h"
#include <boost/array.hpp>

class SerializedGameData;

const MapCoord NO_EDGE = 0xffff;
/// Size of such a TradeGraphNode
const MapCoord TGN_SIZE = 20;

struct TradeGraphNode
{
    /// Point of the node, representing the main node
    MapPoint main_pos;
    /// Possible 8 directions with way costs
    boost::array<MapCoord, 8> dirs;
    /// Direction not possible, even in the future (water, lava, swamp etc.)
    boost::array<bool, 8> not_possible_forever;
    /// Is the route running over any player territory?
    boost::array<bool, MAX_PLAYERS> dont_run_over_player_territory;

    TradeGraphNode();

    void Deserialize(SerializedGameData* sgd);
    void Serialize(SerializedGameData* sgd) const;

    /// Converts map coords to TG coords
    static MapPoint ConverToTGCoords(const MapPoint pos)
    {
        return pos / TGN_SIZE;
    }
};

#endif // TradeGraphNode_h__
