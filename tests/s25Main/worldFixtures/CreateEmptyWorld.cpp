// Copyright (c) 2016 - 2020 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "worldFixtures/CreateEmptyWorld.h"
#include "RttrForeachPt.h"
#include "initGameRNG.hpp"
#include "lua/GameDataLoader.h"
#include "world/GameWorld.h"
#include "world/MapLoader.h"
#include "gameData/TerrainDesc.h"
#include <cmath>
#include <stdexcept>

CreateEmptyWorld::CreateEmptyWorld(const MapExtent& size) : size_(size) {}

bool CreateEmptyWorld::operator()(GameWorld& world) const
{
    // For consistent results
    initGameRNG(0);

    loadGameData(world.GetDescriptionWriteable());
    world.Init(size_);
    // Set everything to buildable land
    DescIdx<TerrainDesc> t(0);
    const WorldDescription& desc = world.GetDescription();
    for(; t.value < desc.terrain.size(); t.value++)
    {
        if(desc.get(t).Is(ETerrain::Buildable) && desc.get(t).kind == TerrainKind::Land)
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
        numPlayersPerDim.y = static_cast<unsigned>(std::ceil(float(numPlayers) / numPlayersPerDim.x));
        // Distance between HQs
        Point<unsigned> playerDist = size_ / numPlayersPerDim;
        // Start with a little offset so we don't place them at the map border
        MapPoint curPt(playerDist / 2u);
        std::vector<MapPoint> hqPositions;
        for(unsigned y = 0; y < numPlayersPerDim.y; y++)
        {
            numPlayersPerDim.x = std::min<unsigned>(numPlayersPerDim.x, numPlayers - hqPositions.size());
            playerDist.x = size_.x / std::max(1u, numPlayersPerDim.x);
            curPt.x = playerDist.x / 2;
            for(unsigned x = 0; x < numPlayersPerDim.x; x++)
            {
                hqPositions.push_back(curPt);
                curPt.x += playerDist.x;
            }
            curPt.y += playerDist.y;
        }
        if(!MapLoader::PlaceHQs(world, hqPositions, false))
            return false; // LCOV_EXCL_LINE
    }
    world.InitAfterLoad();
    return true;
}

void setRightTerrain(GameWorld& world, const MapPoint& pt, Direction dir, DescIdx<TerrainDesc> t)
{
    switch(dir)
    {
        case Direction::West: world.GetNodeWriteable(world.GetNeighbour(pt, Direction::NorthWest)).t1 = t; break;
        case Direction::NorthWest: world.GetNodeWriteable(world.GetNeighbour(pt, Direction::NorthWest)).t2 = t; break;
        case Direction::NorthEast: world.GetNodeWriteable(world.GetNeighbour(pt, Direction::NorthEast)).t1 = t; break;
        case Direction::East: world.GetNodeWriteable(pt).t2 = t; break;
        case Direction::SouthEast: world.GetNodeWriteable(pt).t1 = t; break;
        case Direction::SouthWest: world.GetNodeWriteable(world.GetNeighbour(pt, Direction::West)).t2 = t; break;
    }
}

void setLeftTerrain(GameWorld& world, const MapPoint& pt, Direction dir, DescIdx<TerrainDesc> t)
{
    setRightTerrain(world, pt, dir - 1u, t);
}
