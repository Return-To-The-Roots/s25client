// Copyright (c) 2017 - 2017 Settlers Freaks (sf-team at siedler25.org)
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
#include "randomMaps/algorithm/Tile.h"

#include <boost/test/unit_test.hpp>
#include <set>

BOOST_AUTO_TEST_SUITE(TileTests)

BOOST_AUTO_TEST_CASE(TilesWithNegativeCoordinatesAreInvalid)
{
    MapExtent size(16,16);

    BOOST_REQUIRE(!Tile(Position(0,0), Position(-1,0)).IsValid(size));
    BOOST_REQUIRE(!Tile(Position(0,0), Position(0,-1)).IsValid(size));
    BOOST_REQUIRE(!Tile(Position(-1,0), Position(0,0)).IsValid(size));
    BOOST_REQUIRE(!Tile(Position(0,-1), Position(0,0)).IsValid(size));
}

BOOST_AUTO_TEST_CASE(TilesWithNonNeighboringTextureCoordinatesAreInvalid)
{
    MapExtent size(16,16);

    BOOST_REQUIRE(!Tile(Position(0,1), Position(0,0)).IsValid(size));
}

BOOST_AUTO_TEST_CASE(TilesWithNeighboringTextureCoordinatesAreValid)
{
    MapExtent size(16,16);
    std::vector<Tile> validTiles = {
        Tile(Position(0,0), Position(0,0)),
        Tile(Position(2,2), Position(1,3)),
        Tile(Position(1,0), Position(0,0)),
        Tile(Position(3,1), Position(3,2)),
        Tile(Position(2,0), Position(1,1))
    };
    
    for (auto tile = validTiles.begin(); tile != validTiles.end(); tile++)
    {
        BOOST_REQUIRE(tile->IsValid(size));
    }
}

BOOST_AUTO_TEST_CASE(InvalidTileDoesNotHaveAnyNeighbors)
{
    MapExtent size(16,16);
    Tile invalidTile(Position(0,1), Position(0,0));
    BOOST_REQUIRE(invalidTile.Neighbors(size).empty());
}

BOOST_AUTO_TEST_CASE(ValidTilesHaveEightNeighbors)
{
    MapExtent size(16,16);
    std::vector<Tile> validTiles = {
        Tile(Position(0,0), Position(0,0)),
        Tile(Position(2,2), Position(1,3)),
        Tile(Position(1,0), Position(0,0)),
        Tile(Position(3,1), Position(3,2)),
        Tile(Position(2,0), Position(1,1)),
        Tile(Position(10,1), Position(9,1))
    };
    
    for (auto tile = validTiles.begin(); tile != validTiles.end(); tile++)
    {
        BOOST_REQUIRE(tile->Neighbors(size).size() == 8u);
    }
}

BOOST_AUTO_TEST_CASE(NeighborsOfValidTilesAreValid)
{
    MapExtent size(16,16);
    std::vector<Tile> validTiles = {
        Tile(Position(0,0), Position(0,0)),
        Tile(Position(2,2), Position(1,3)),
        Tile(Position(1,0), Position(0,0)),
        Tile(Position(3,1), Position(3,2)),
        Tile(Position(2,0), Position(1,1)),
        Tile(Position(10,1), Position(9,1))
    };
    
    for (auto tile = validTiles.begin(); tile != validTiles.end(); tile++)
    {
        auto neighbors = tile->Neighbors(size);
        
        for (auto neighbor = neighbors.begin(); neighbor != neighbors.end(); neighbor++)
        {
            BOOST_REQUIRE(neighbor->IsValid(size));
        }
    }
}

BOOST_AUTO_TEST_CASE(TilesWithEqualRsuAndLsdPositionsAreConsideredEqual)
{
    TileCompare compare;
    Tile t1(Position(1,2), Position(2,3));
    Tile t2(Position(1,2), Position(2,3));

    BOOST_REQUIRE(!compare.operator()(t1, t2) && !compare.operator()(t2, t1));
}

BOOST_AUTO_TEST_CASE(TilesAreSortedCorrectlyByComparator)
{
    TileCompare compare;
    Tile t_less(Position(1,2), Position(2,3));
    Tile t_greater(Position(1,3), Position(2,3));

    BOOST_REQUIRE(compare.operator()(t_less, t_greater));
    BOOST_REQUIRE(!compare.operator()(t_greater, t_less));
}

BOOST_AUTO_TEST_CASE(SetOfTilesDoesNotInsertEqualTilesTwice)
{
    std::set<Tile, TileCompare> tiles;
    
    auto insert1 = tiles.insert(Tile(Position(1,2), Position(2,3)));
    
    BOOST_REQUIRE(insert1.second);
    BOOST_REQUIRE(tiles.size() == 1);
    
    auto insert2 = tiles.insert(Tile(Position(1,2), Position(2,3)));
    
    BOOST_REQUIRE(!insert2.second);
    BOOST_REQUIRE(tiles.size() == 1);

    auto insert3 = tiles.insert(Tile(Position(2,2), Position(2,3)));
    
    BOOST_REQUIRE(insert3.second);
    BOOST_REQUIRE(tiles.size() == 2);
}

BOOST_AUTO_TEST_SUITE_END()
