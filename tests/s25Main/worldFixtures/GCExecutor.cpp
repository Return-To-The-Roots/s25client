#include "GCExecutor.h"
#include "pathfinding/FindPathForRoad.h"
#include "world/GameWorld.h"
#include "gameTypes/GameTypesOutput.h"

void GCExecutor::BuildRoadForBlds(const MapPoint bldPosFrom, const MapPoint bldPosTo)
{
    auto& world = GetWorld();
    const MapPoint start = world.GetNeighbour(bldPosFrom, Direction::SouthEast);
    const MapPoint end = world.GetNeighbour(bldPosTo, Direction::SouthEast);
    std::vector<Direction> road = FindPathForRoad(world, start, end, false);
    BOOST_TEST_REQUIRE(!road.empty());
    this->BuildRoad(start, false, road);
    BOOST_TEST_REQUIRE(world.GetPointRoad(start, road.front()) == PointRoad::Normal);
}
