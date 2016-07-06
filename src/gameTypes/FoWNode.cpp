// Copyright (c) 2005 - 2016 Settlers Freaks (sf-team at siedler25.org)
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

#include "defines.h" // IWYU pragma: keep
#include "gameTypes/FoWNode.h"
#include "SerializedGameData.h"
#include <algorithm>

FoWNode::FoWNode(): 
    last_update_time(0), visibility(VIS_INVISIBLE), object(NULL), owner(0)
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
        for(unsigned r = 0; r < roads.size(); ++r)
            sgd.PushUnsignedChar(roads[r]);
        sgd.PushUnsignedChar(owner);
        for(unsigned b = 0; b < boundary_stones.size(); ++b)
            sgd.PushUnsignedChar(boundary_stones[b]);
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
        for(unsigned r = 0; r < roads.size(); ++r)
            roads[r] = sgd.PopUnsignedChar();
        owner = sgd.PopUnsignedChar();
        for(unsigned b = 0; b < boundary_stones.size(); ++b)
            boundary_stones[b] = sgd.PopUnsignedChar();
    }
    else
    {
        last_update_time = 0;
        object = NULL;
        for(unsigned r = 0; r < roads.size(); ++r)
            roads[r] = 0;
        owner = 0;
        for(unsigned b = 0; b < boundary_stones.size(); ++b)
            boundary_stones[b] = 0;
    }
}
