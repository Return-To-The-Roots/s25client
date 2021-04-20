// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "gameTypes/FoWNode.h"
#include "SerializedGameData.h"
#include "enum_cast.hpp"
#include <algorithm>

FoWNode::FoWNode() : last_update_time(0), visibility(Visibility::Invisible), owner(0)
{
    std::fill(roads.begin(), roads.end(), PointRoad::None);
    std::fill(boundary_stones.begin(), boundary_stones.end(), 0);
}

void FoWNode::Serialize(SerializedGameData& sgd) const
{
    sgd.PushEnum<uint8_t>(visibility);
    // Only in FoW can be FoW objects
    if(visibility == Visibility::FogOfWar)
    {
        sgd.PushUnsignedInt(last_update_time);
        sgd.PushFOWObject(object.get());
        helpers::pushContainer(sgd, roads);
        sgd.PushUnsignedChar(owner);
        helpers::pushContainer(sgd, boundary_stones);
    }
}

void FoWNode::Deserialize(SerializedGameData& sgd)
{
    visibility = sgd.Pop<Visibility>();
    // Only in FoW can be FoW objects
    if(visibility == Visibility::FogOfWar)
    {
        last_update_time = sgd.PopUnsignedInt();
        object = sgd.PopFOWObject();
        helpers::popContainer(sgd, roads);
        owner = sgd.PopUnsignedChar();
        helpers::popContainer(sgd, boundary_stones);
    } else
    {
        last_update_time = 0;
        object.reset();
        std::fill(roads.begin(), roads.end(), PointRoad::None);
        owner = 0;
        std::fill(boundary_stones.begin(), boundary_stones.end(), 0);
    }
}
