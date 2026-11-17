// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "world/TradePath.h"
#include "SerializedGameData.h"

TradePath::TradePath(SerializedGameData& sgd) : start(sgd.popMapPoint()), goal(sgd.popMapPoint())
{
    route.resize(sgd.popUnsignedInt());
    for(Direction& dir : route)
    {
        dir = sgd.pop<Direction>();
    }
}

void TradePath::serialize(SerializedGameData& sgd) const
{
    helpers::pushPoint(sgd, start);
    helpers::pushPoint(sgd, goal);
    sgd.pushUnsignedInt(route.size());
    for(const Direction& dir : route)
    {
        sgd.pushEnum<uint8_t>(dir);
    }
}
