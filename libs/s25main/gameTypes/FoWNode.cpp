// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
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

#include "gameTypes/FoWNode.h"
#include "SerializedGameData.h"
#include <algorithm>

FoWNode::FoWNode() : last_update_time(0), visibility(VIS_INVISIBLE), object(nullptr), owner(0)
{
    std::fill(roads.begin(), roads.end(), 0);
    std::fill(boundary_stones.begin(), boundary_stones.end(), 0);
}

void FoWNode::Serialize(SerializedGameData& sgd) const
{
    sgd.PushUnsignedChar(static_cast<unsigned char>(visibility));
    // Only in FoW can be FoW objects
    if(visibility == VIS_FOW)
    {
        sgd.PushUnsignedInt(last_update_time);
        sgd.PushFOWObject(object);
        for(unsigned char road : roads)
            sgd.PushUnsignedChar(road);
        sgd.PushUnsignedChar(owner);
        for(unsigned char boundary_stone : boundary_stones)
            sgd.PushUnsignedChar(boundary_stone);
    }
}

void FoWNode::Deserialize(SerializedGameData& sgd)
{
    visibility = Visibility(sgd.PopUnsignedChar());
    // Only in FoW can be FoW objects
    if(visibility == VIS_FOW)
    {
        last_update_time = sgd.PopUnsignedInt();
        object = sgd.PopFOWObject();
        for(unsigned char& road : roads)
            road = sgd.PopUnsignedChar();
        owner = sgd.PopUnsignedChar();
        for(unsigned char& boundary_stone : boundary_stones)
            boundary_stone = sgd.PopUnsignedChar();
    } else
    {
        last_update_time = 0;
        object = nullptr;
        for(unsigned char& road : roads)
            road = 0;
        owner = 0;
        for(unsigned char& boundary_stone : boundary_stones)
            boundary_stone = 0;
    }
}
