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
#include "RttrConfig.h"
#include "files.h"
#include "lua/GameDataLoader.h"
#include "gameData/EdgeDesc.h"
#include "gameData/TerrainData.h"
#include "gameData/TerrainDesc.h"
#include "gameData/WorldDescription.h"
#include "test/BQOutput.h"
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(GameData)

BOOST_AUTO_TEST_CASE(LoadGameData)
{
    boost::shared_ptr<WorldDescription> worldDesc(new WorldDescription());
    GameDataLoader loader(worldDesc, RTTRCONFIG.ExpandPath(FILE_PATHS[1]) + "/world");
    BOOST_REQUIRE(loader.Load());
    BOOST_REQUIRE_EQUAL(worldDesc->edges.size(), 5u);
    BOOST_REQUIRE_EQUAL(worldDesc->terrain.size(), NUM_TTS);
    for(unsigned i = 0; i < NUM_TTS; i++)
    {
        TerrainType t = TerrainType(i);
        const TerrainDesc& desc = worldDesc->terrain.get(DescIdx<TerrainDesc>(i));
        BOOST_REQUIRE_EQUAL(desc.s2Id, TerrainData::GetTextureIdentifier(t));
        BOOST_REQUIRE_EQUAL(!desc.edgeType ? 0 : desc.edgeType.value + 1, TerrainData::GetEdgeType(Landscape::GREENLAND, t));
        BOOST_REQUIRE_EQUAL(desc.GetBQ(), TerrainData::GetBuildingQuality(t));
        BOOST_REQUIRE_EQUAL(!desc.Is(ETerrain::Walkable), TerrainData::GetBuildingQuality(t) == TerrainBQ::NOTHING
                                                            || TerrainData::GetBuildingQuality(t) == TerrainBQ::DANGER);
        BOOST_REQUIRE_EQUAL(desc.Is(ETerrain::Mineable), TerrainData::GetBuildingQuality(t) == TerrainBQ::MINE);
        BOOST_REQUIRE_EQUAL(desc.Is(ETerrain::Buildable), TerrainData::GetBuildingQuality(t) == TerrainBQ::CASTLE);
        BOOST_REQUIRE_EQUAL(desc.Is(ETerrain::Shippable), TerrainData::IsUsableByShip(t));
        BOOST_REQUIRE_EQUAL(desc.IsUsableByAnimals(), TerrainData::IsUsableByAnimals(t));
        BOOST_REQUIRE_EQUAL(desc.IsVital(), TerrainData::IsVital(t));
    }
}

BOOST_AUTO_TEST_SUITE_END()
