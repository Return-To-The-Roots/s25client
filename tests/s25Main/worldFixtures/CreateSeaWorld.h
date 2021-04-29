// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "gameTypes/MapCoordinates.h"

class GameWorld;

struct SeaWorldDefault
{
    // Min size is 59
    static constexpr unsigned width = 60;
    static constexpr unsigned height = 62;
};

template<unsigned T_numPlayers>
struct SmallSeaWorldDefault
{
    // Min size for 2 players
    static constexpr unsigned width = (8 * 2 + 4) * 2;
    static constexpr unsigned height = width + 2;
};
template<>
struct SmallSeaWorldDefault<1>
{
    // Min size for 1 player
    static constexpr unsigned width = 8 * 2 + 4;
    static constexpr unsigned height = width + 2;
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
    explicit CreateSeaWorld(const MapExtent& size);
    bool operator()(GameWorld& world) const;

private:
    MapExtent size_;
};

/// World for <=2 players with all water except 2 small patches of land at opposite sides of the map with 2 harbor spots
/// each
struct CreateWaterWorld
{
    explicit CreateWaterWorld(const MapExtent& size);
    bool operator()(GameWorld& world) const;

private:
    MapExtent size_;
};
