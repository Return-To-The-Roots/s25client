#include "GCExecutor.h"
#include "pathfinding/FindPathForRoad.h"
#include "world/GameWorld.h"
#include "gameTypes/GameTypesOutput.h"

void GCExecutor::buildRoadForBlds(const MapPoint bldPosFrom, const MapPoint bldPosTo)
{
    auto& world = getWorld();
    const MapPoint start = world.getNeighbour(bldPosFrom, Direction::SouthEast);
    const MapPoint end = world.getNeighbour(bldPosTo, Direction::SouthEast);
    std::vector<Direction> road = FindPathForRoad(world, start, end, false);
    BOOST_TEST_REQUIRE(!road.empty());
    this->BuildRoad(start, false, road);
    BOOST_TEST_REQUIRE(world.getPointRoad(start, road.front()) == PointRoad::Normal);
}
