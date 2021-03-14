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
