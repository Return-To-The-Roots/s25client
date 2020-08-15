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

#include "lua/GameDataLoader.h"
#include "mapGenerator/Islands.h"
#include "mapGenerator/TextureHelper.h"
#include <boost/test/unit_test.hpp>

using namespace rttr::mapGenerator;

BOOST_AUTO_TEST_SUITE(IslandsTests)
/*
template<class T_Test>
void RunTest(T_Test test);

template<class T_Test>
void RunTest(T_Test test)
{
    MapExtent size(16, 16);
    DescIdx<LandscapeDesc> landscape(1);
    WorldDescription worldDesc;
    loadGameData(worldDesc);

    TextureMap textures(worldDesc, landscape);
    textures.Resize(size);

    test(textures, size);
}
*/
BOOST_AUTO_TEST_SUITE_END()
