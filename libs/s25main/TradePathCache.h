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
#include "s25util/Singleton.h"
#include <array>

class GameWorldGame;

class TradePathCache : public Singleton<TradePathCache>
{
    struct Entry
    {
        unsigned char player;
        unsigned lastUse;
        TradePath path;
    };

    std::array<Entry, 10> pathes; //-V730_NOINIT
    unsigned curSize;

    unsigned FindEntry(const GameWorldGame& gwg, const MapPoint& start, const MapPoint& goal,
                       unsigned char player) const;

public:
    TradePathCache() : curSize(0) {}

    void Clear() { curSize = 0; }
    bool PathExists(const GameWorldGame& gwg, const MapPoint& start, const MapPoint& goal, unsigned char player);
    void AddEntry(const GameWorldGame& gwg, const TradePath& path, unsigned char player);
};
