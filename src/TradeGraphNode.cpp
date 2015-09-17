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
#include "TradeGraphNode.h"
#include "SerializedGameData.h"
#include <algorithm>

TradeGraphNode::TradeGraphNode(): main_pos(MapPoint::Invalid())
{
    std::fill(dirs.begin(), dirs.end(), NO_EDGE);
    std::fill(not_possible_forever.begin(), not_possible_forever.end(), false);
    std::fill(dont_run_over_player_territory.begin(), dont_run_over_player_territory.end(), false);
}

void TradeGraphNode::Deserialize(SerializedGameData* sgd)
{
    main_pos = sgd->PopMapPoint();
    for(unsigned i = 0; i < 8; ++i)
    {
        dirs[i] = sgd->PopUnsignedShort();
        not_possible_forever[i] = sgd->PopBool();
        dont_run_over_player_territory[i] = sgd->PopBool();
    }
}

void TradeGraphNode::Serialize(SerializedGameData* sgd) const
{
    sgd->PushMapPoint(main_pos);
    for(unsigned i = 0; i < 8; ++i)
    {
        sgd->PushUnsignedShort(dirs[i]);
        sgd->PushBool(not_possible_forever[i]);
        sgd->PushBool(dont_run_over_player_territory[i]);
    }
}