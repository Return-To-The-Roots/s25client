// Copyright (C) 2016 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "gameTypes/Direction.h"
#include "gameTypes/MapCoordinates.h"
#include "gameData/DescIdx.h"

class GameWorld;
struct TerrainDesc;

/// Creates an empty world, with meadow terrain and the given number of players
struct CreateEmptyWorld
{
    explicit CreateEmptyWorld(const MapExtent& size);
    bool operator()(GameWorld& world) const;

private:
    MapExtent size_;
};

void setRightTerrain(GameWorld& world, const MapPoint& pt, Direction dir, DescIdx<TerrainDesc> t);
void setLeftTerrain(GameWorld& world, const MapPoint& pt, Direction dir, DescIdx<TerrainDesc> t);
