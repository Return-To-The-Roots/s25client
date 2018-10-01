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

#ifndef CreateEmptyWorld_h__
#define CreateEmptyWorld_h__

#include "gameTypes/Direction.h"
#include "gameTypes/MapCoordinates.h"
#include "gameTypes/MapTypes.h"
#include "gameTypes/Nation.h"
#include "gameData/DescIdx.h"
#include <vector>

class GameWorldGame;
struct TerrainDesc;

/// Creates an empty world, with meadow terrain and the given number of players
struct CreateEmptyWorld
{
    CreateEmptyWorld(const MapExtent& size, unsigned numPlayers);
    bool operator()(GameWorldGame& world) const;

private:
    MapExtent size_;
    std::vector<Nation> playerNations_;
};

/// Create an uninitalized world (terrain, BQ etc not set. only nodes and size)
struct CreateUninitWorld
{
    CreateUninitWorld(const MapExtent& size, unsigned numPlayers);
    bool operator()(GameWorldGame& world) const;

private:
    MapExtent size_;
};

void setRightTerrain(GameWorldGame& world, const MapPoint& pt, Direction dir, DescIdx<TerrainDesc> t);
void setLeftTerrain(GameWorldGame& world, const MapPoint& pt, Direction dir, DescIdx<TerrainDesc> t);

#endif // CreateEmptyWorld_h__
