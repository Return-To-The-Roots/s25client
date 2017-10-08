// Copyright (c) 2016 - 2017 Settlers Freaks (sf-team at siedler25.org)
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

#ifndef CreateSeaWorld_h__
#define CreateSeaWorld_h__

#include "gameTypes/MapCoordinates.h"
#include "gameTypes/Nation.h"
#include <vector>

class GameWorldGame;

struct SeaWorldDefault
{
    // Min size is 59
    BOOST_STATIC_CONSTEXPR unsigned width = 60;
    BOOST_STATIC_CONSTEXPR unsigned height = 62;
};

/// Creates a world for up to 4 players,
/// with a sea on the outside and a lake on the inside with each player having access to both
/// Harbors: 1: Top outside
///          2: Top inside
///          3: Left outside
///          4: Left inside
///          5: Right inside
///          6: Right outside
///          7: Bottom inside
///          8: Bottom outside
struct CreateSeaWorld
{
    CreateSeaWorld(const MapExtent& size, unsigned numPlayers);
    bool operator()(GameWorldGame& world) const;

private:
    MapExtent size_;
    std::vector<Nation> playerNations_;
};

/// World for 2 players with all water except a small patch of land with player 0 HQ in the middle and player 1 HQ in the bottom left
struct CreateWaterWorld
{
    CreateWaterWorld(const MapExtent& size, unsigned numPlayers);
    bool operator()(GameWorldGame& world) const;

private:
    MapExtent size_;
    std::vector<Nation> playerNations_;
};

#endif // CreateSeaWorld_h__
