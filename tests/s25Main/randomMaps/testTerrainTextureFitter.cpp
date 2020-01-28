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
#include "randomMaps/terrain/TextureFitter.h"

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(TextureFitterTests)

BOOST_AUTO_TEST_CASE(FlipsTrianglesToAvoidSharpCorners)
{
    std::vector<TextureType> rsu = {
        Water,  Lava /**/,Water,  Water,  Water,  Water,  Water,  Water,
        Water,  Water,    Water,  Water,  Water,  Water,  Water,  Water,
        Water,  Water,    Water,  Water,  Water,  Water,  Water,  Water,
        Coast,  Grass1,   Grass1, Grass1, Grass1, Grass1, Grass1, Coast,
        Coast,  Grass1,   Grass1, Grass1, Grass1, Grass1, Grass1, Coast,
        Coast,  Grass1,   Grass1, Grass1, Grass1, Grass1, Grass1, Coast,
        Coast,  Coast,    Coast,  Coast,  Coast,  Coast,  Coast,  Coast,
        Lava,   Lava,     Lava,   Lava,   Lava,   Lava,   Lava,   Lava
    };
    
    /**/
    //should be replaced by water

    std::vector<TextureType> lsd = {
        Water,  Water,  Water,  Water,  Water,  Water,  Water,  Water,
        Water,  Water,  Water,  Water,  Water,  Water,  Water,  Water,
        Water,  Water,  Water,  Water,  Water,  Water,  Water,  Water,
        Coast,  Grass1, Grass1, Grass1, Grass1, Grass1, Grass1, Coast,
        Coast,  Grass1, Grass1, Grass1, Grass1, Grass1, Grass1, Coast,
        Coast,  Grass1, Grass1, Grass1, Grass1, Grass1, Grass1, Coast,
        Coast,  Coast,  Coast,  Coast,  Coast,  Coast,  Coast,  Coast,
        Lava,   Lava,   Lava,   Lava,   Lava,   Lava,   Lava,   Lava
    };

    MapExtent size(8,8);
    
    TextureFitter::SmoothTextures(rsu, lsd, size);
    
    BOOST_REQUIRE(rsu[1]  == Water);
    BOOST_REQUIRE(lsd[34] == Grass1);
}

BOOST_AUTO_TEST_CASE(RemovesMountainTransitionTexturesWithoutMountains)
{
    std::vector<TextureType> rsu = {
        GrassFlower,GrassFlower,GrassFlower,GrassFlower,
        GrassToMountain,GrassToMountain,GrassToMountain,GrassToMountain,
        GrassToMountain,GrassToMountain,GrassToMountain,GrassToMountain,
        GrassToMountain,GrassToMountain,GrassToMountain,GrassToMountain
    };
    std::vector<TextureType> lsd(rsu);
    
    MapExtent size(4,4);
    
    TextureFitter::SmoothMountains(rsu, lsd, size);
    
    for (int i = 0; i < 16; i++)
    {
        BOOST_REQUIRE(rsu[i] == GrassFlower);
        BOOST_REQUIRE(lsd[i] == GrassFlower);
    }
}

BOOST_AUTO_TEST_CASE(DoesNotRemoveMountainTransitionWithMountainsNearby)
{
    std::vector<TextureType> rsu = {
        GrassToMountain,GrassToMountain,GrassToMountain,GrassToMountain,
        GrassToMountain,Mountain2,Mountain2,GrassToMountain,
        GrassToMountain,Mountain1,Mountain1,GrassToMountain,
        GrassToMountain,GrassToMountain,GrassToMountain,GrassToMountain
    };
    std::vector<TextureType> lsd(rsu);
    
    MapExtent size(4,4);
    
    TextureFitter::SmoothMountains(rsu, lsd, size);

    std::vector<int> expectedTransitions = {0,1,2,3,4,7,8,11,12,13,14,15};
    
    for (int i: expectedTransitions)
    {
        BOOST_REQUIRE(rsu[i] == GrassToMountain);
        BOOST_REQUIRE(lsd[i] == GrassToMountain);
    }
}

BOOST_AUTO_TEST_SUITE_END()
