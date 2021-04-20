// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "world/TradePath.h"
#include "SerializedGameData.h"

TradePath::TradePath(SerializedGameData& sgd) : start(sgd.PopMapPoint()), goal(sgd.PopMapPoint())
{
    route.resize(sgd.PopUnsignedInt());
    for(Direction& dir : route)
    {
        dir = sgd.Pop<Direction>();
    }
}

void TradePath::Serialize(SerializedGameData& sgd) const
{
    helpers::pushPoint(sgd, start);
    helpers::pushPoint(sgd, goal);
    sgd.PushUnsignedInt(route.size());
    for(const Direction& dir : route)
    {
        sgd.PushEnum<uint8_t>(dir);
    }
}
