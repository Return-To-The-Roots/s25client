// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "worldFixtures/CreateEmptyWorld.h"
#include "GameEvent.h"
#include "GamePlayer.h"
#include "RttrForeachPt.h"
#include "buildings/nobHQ.h"
#include "figures/noFigure.h"
#include "initGameRNG.hpp"
#include "lua/GameDataLoader.h"
#include "worldFixtures/WorldFixture.h"
#include "worldFixtures/terrainHelpers.h"
#include "world/GameWorld.h"
#include "world/MapLoader.h"
#include <boost/test/unit_test.hpp>
#include <cmath>
#include <stdexcept>

void WorldFixtureBase::addStartResources()
{
    for(unsigned i = 0; i < world.getNumPlayers(); i++)
        addStartResources(i);
}

void WorldFixtureBase::addStartResources(unsigned playerIdx)
{
    world.getPlayer(playerIdx).getHQ()->addStartWares();
}

/// Reschedule the walk event of the obj to be executed in numGFs GFs
void WorldFixtureBase::rescheduleWalkEvent(noMovable& obj, unsigned numGFs)
{
    std::vector<const GameEvent*> evts = em.getObjEvents(obj);
    for(const GameEvent* ev : evts)
    {
        if(ev->id == 0)
        {
            em.rescheduleEvent(ev, em.getCurrentGF() + numGFs);
            return;
        }
    }
    BOOST_TEST_FAIL("Event not found"); // LCOV_EXCL_LINE
}

/// Move the object next to the given point. The next walk event will make it reach that point
// LCOV_EXCL_START
void WorldFixtureBase::moveObjTo(noFigure& obj, const MapPoint pos)
{
    std::unique_ptr<noFigure> ownedObj;
    if(world.hasFigureAt(obj.getPos(), obj))
        ownedObj = world.removeFigure(obj.getPos(), obj);
    else
        ownedObj = world.removeFigure(world.getNeighbour(obj.getPos(), obj.getCurMoveDir() + 3u), obj);
    obj.setPos(world.getNeighbour(pos, Direction::West));
    world.addFigure(obj.getPos(), std::move(ownedObj));
    if(obj.isMoving())
        obj.faceDir(Direction::East);
    else
        obj.startWalking(Direction::East);
}
// LCOV_EXCL_STOP

CreateEmptyWorld::CreateEmptyWorld(const MapExtent& size) : size_(size) {}

bool CreateEmptyWorld::operator()(GameWorld& world) const
{
    // For consistent results
    initGameRNG(0);

    loadGameData(world.getDescriptionWriteable());
    world.init(size_);
    // Set everything to buildable land
    DescIdx<TerrainDesc> t = getLandTerrain(world.getDescription(), ETerrain::Buildable);
    RTTR_FOREACH_PT(MapPoint, size_)
    {
        MapNode& node = world.getNodeWriteable(pt);
        node.t1 = node.t2 = t;
    }
    unsigned numPlayers = world.getNumPlayers();
    if(numPlayers > 0)
    {
        // Distribute player HQs evenly across map
        Point<unsigned> numPlayersPerDim;
        numPlayersPerDim.x = static_cast<unsigned>(std::ceil(std::sqrt(numPlayers)));
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
        BOOST_TEST_REQUIRE(MapLoader::placeHQs(world, hqPositions, false));
    }
    world.initAfterLoad();
    return true;
}

void setRightTerrain(GameWorld& world, const MapPoint& pt, Direction dir, DescIdx<TerrainDesc> t)
{
    switch(dir)
    {
        case Direction::West: world.getNodeWriteable(world.getNeighbour(pt, Direction::NorthWest)).t1 = t; break;
        case Direction::NorthWest: world.getNodeWriteable(world.getNeighbour(pt, Direction::NorthWest)).t2 = t; break;
        case Direction::NorthEast: world.getNodeWriteable(world.getNeighbour(pt, Direction::NorthEast)).t1 = t; break;
        case Direction::East: world.getNodeWriteable(pt).t2 = t; break;
        case Direction::SouthEast: world.getNodeWriteable(pt).t1 = t; break;
        case Direction::SouthWest: world.getNodeWriteable(world.getNeighbour(pt, Direction::West)).t2 = t; break;
    }
}

void setLeftTerrain(GameWorld& world, const MapPoint& pt, Direction dir, DescIdx<TerrainDesc> t)
{
    setRightTerrain(world, pt, dir - 1u, t);
}
