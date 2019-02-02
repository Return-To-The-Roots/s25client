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

#include "rttrDefines.h" // IWYU pragma: keep
#include "worldFixtures/CreateEmptyWorld.h"
#include "initGameRNG.hpp"
#include "lua/GameDataLoader.h"
#include "world/GameWorldGame.h"
#include "world/MapLoader.h"
#include "gameData/TerrainDesc.h"
#include <cmath>
#include <stdexcept>

CreateEmptyWorld::CreateEmptyWorld(const MapExtent& size) : size_(size) {}

bool CreateEmptyWorld::operator()(GameWorldGame& world) const
{
    // For consistent results
    doInitGameRNG(0);

    loadGameData(world.GetDescriptionWriteable());
    world.Init(size_);
    // Set everything to buildable land
    DescIdx<TerrainDesc> t(0);
    const WorldDescription& desc = world.GetDescription();
    for(; t.value < desc.terrain.size(); t.value++)
    {
        if(desc.get(t).Is(ETerrain::Buildable) && desc.get(t).kind == TerrainKind::LAND)
            break;
    }
    RTTR_FOREACH_PT(MapPoint, size_)
    {
        MapNode& node = world.GetNodeWriteable(pt);
        node.t1 = node.t2 = t;
    }
    unsigned numPlayers = world.GetNumPlayers();
    if(numPlayers > 0)
    {
        // Distribute player HQs evenly across map
        Point<unsigned> numPlayersPerDim;
        numPlayersPerDim.x = static_cast<unsigned>(ceil(sqrt(numPlayers)));
        numPlayersPerDim.y = static_cast<unsigned>(ceil(float(numPlayers) / numPlayersPerDim.x));
        // Distance between HQs
        Point<unsigned> playerDist = size_ / numPlayersPerDim;
        // Start with a little offset so we don't place them at the map border
        MapPoint curPt(playerDist / 2u);
        std::vector<MapPoint> hqPositions;
        for(unsigned y = 0; y < numPlayersPerDim.y; y++)
        {
            numPlayersPerDim.x = min<unsigned>(numPlayersPerDim.x, numPlayers - hqPositions.size());
            playerDist.x = size_.x / numPlayersPerDim.x;
            curPt.x = playerDist.x / 2;
            for(unsigned x = 0; x < numPlayersPerDim.x; x++)
            {
                hqPositions.push_back(curPt);
                curPt.x += playerDist.x;
            }
            curPt.y += playerDist.y;
        }
        if(!MapLoader::PlaceHQs(world, hqPositions, false))
            return false;
    }
    world.InitAfterLoad();
    return true;
}

CreateUninitWorld::CreateUninitWorld(const MapExtent& size) : size_(size) {}

bool CreateUninitWorld::operator()(GameWorldGame& world) const
{
    if(world.GetNumPlayers() > 0)
        throw std::logic_error("Cannot have players for uninitialized world");
    // For consistent results
    doInitGameRNG(0);

    loadGameData(world.GetDescriptionWriteable());
    world.Init(size_);
    return true;
}

void setRightTerrain(GameWorldGame& world, const MapPoint& pt, Direction dir, DescIdx<TerrainDesc> t)
{
    switch(dir.native_value())
    {
        case Direction::WEST: world.GetNodeWriteable(world.GetNeighbour(pt, Direction::NORTHWEST)).t1 = t; break;
        case Direction::NORTHWEST: world.GetNodeWriteable(world.GetNeighbour(pt, Direction::NORTHWEST)).t2 = t; break;
        case Direction::NORTHEAST: world.GetNodeWriteable(world.GetNeighbour(pt, Direction::NORTHEAST)).t1 = t; break;
        case Direction::EAST: world.GetNodeWriteable(pt).t2 = t; break;
        case Direction::SOUTHEAST: world.GetNodeWriteable(pt).t1 = t; break;
        case Direction::SOUTHWEST: world.GetNodeWriteable(world.GetNeighbour(pt, Direction::WEST)).t2 = t; break;
    }
}

void setLeftTerrain(GameWorldGame& world, const MapPoint& pt, Direction dir, DescIdx<TerrainDesc> t)
{
    setRightTerrain(world, pt, Direction(dir.toUInt() + 6 - 1), t);
}
