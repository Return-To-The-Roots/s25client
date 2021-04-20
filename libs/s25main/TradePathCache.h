// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "world/TradePath.h"
#include <boost/container/static_vector.hpp>

class GameWorld;

class TradePathCache
{
    using PlayerIdx = unsigned char;

    struct Entry
    {
        PlayerIdx player;
        unsigned lastUse;
        TradePath path;
    };

    const GameWorld& world;
    boost::container::static_vector<Entry, 10> paths;
    int findEntry(MapPoint start, MapPoint goal, PlayerIdx player) const;

public:
    TradePathCache(const GameWorld& world) : world(world) {}

    void clear() { paths.clear(); }
    unsigned size() const { return paths.size(); }
    bool pathExists(MapPoint start, MapPoint goal, PlayerIdx player);
    void addEntry(TradePath path, PlayerIdx player);
};
