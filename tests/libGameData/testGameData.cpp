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
#include "PointOutput.h"
#include "TerrainBQOutput.h"
#include "helpers/containerUtils.h"
#include "legacy/TerrainData.h"
#include "lua/GameDataLoader.h"
#include "gameData/EdgeDesc.h"
#include "gameData/TerrainDesc.h"
#include "gameData/WorldDescription.h"
#include <boost/filesystem.hpp>
#include <boost/nowide/fstream.hpp>
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(GameData)

BOOST_AUTO_TEST_CASE(LoadGameData)
{
    WorldDescription worldDesc;
    loadGameData(worldDesc);
    BOOST_REQUIRE_EQUAL(worldDesc.edges.size(), 3u * 5u - 1u);
    BOOST_REQUIRE_GE(worldDesc.terrain.size(), 3u * NUM_TTS);
    for(unsigned l = 0; l < NUM_LTS; l++)
    {
        Landscape lt = Landscape(l);
        // indexOf(name) in these arrays should be the original RTTR index as in TerrainData
        std::vector<std::string> tNames, eNames;
        for(DescIdx<TerrainDesc> i(0); i.value < worldDesc.terrain.size(); i.value++)
        {
            if(worldDesc.get(worldDesc.get(i).landscape).s2Id == l)
                tNames.push_back(worldDesc.get(i).name);
        }
        for(DescIdx<EdgeDesc> i(0); i.value < worldDesc.edges.size(); i.value++)
        {
            if(worldDesc.get(worldDesc.get(i).landscape).s2Id == l)
                eNames.push_back(worldDesc.get(i).name);
        }
        for(unsigned i = 0; i < NUM_TTS; i++)
        {
            TerrainType t = TerrainType(i);
            const TerrainDesc& desc = worldDesc.terrain.get(worldDesc.terrain.getIndex(tNames[i]));
            BOOST_REQUIRE_EQUAL(desc.s2Id, TerrainData::GetTextureIdentifier(t));
            EdgeType newEdge = !desc.edgeType ? ET_NONE : EdgeType((helpers::indexOf(eNames, worldDesc.get(desc.edgeType).name) + 1));
            BOOST_REQUIRE_EQUAL(newEdge, TerrainData::GetEdgeType(lt, t));
            BOOST_REQUIRE_EQUAL(desc.GetBQ(), TerrainData::GetBuildingQuality(t));
            BOOST_REQUIRE_EQUAL(!desc.Is(ETerrain::Walkable), TerrainData::GetBuildingQuality(t) == TerrainBQ::NOTHING
                                                                || TerrainData::GetBuildingQuality(t) == TerrainBQ::DANGER);
            BOOST_REQUIRE_EQUAL(desc.Is(ETerrain::Mineable), TerrainData::GetBuildingQuality(t) == TerrainBQ::MINE);
            BOOST_REQUIRE_EQUAL(desc.Is(ETerrain::Buildable), TerrainData::GetBuildingQuality(t) == TerrainBQ::CASTLE);
            BOOST_REQUIRE_EQUAL(desc.Is(ETerrain::Shippable), TerrainData::IsUsableByShip(t));
            BOOST_REQUIRE_EQUAL(desc.kind == TerrainKind::SNOW, TerrainData::IsSnow(lt, t));
            BOOST_REQUIRE_EQUAL(desc.IsUsableByAnimals() || desc.kind == TerrainKind::SNOW,
                                TerrainData::IsUsableByAnimals(t) || TerrainData::IsSnow(lt, t));
            BOOST_REQUIRE_EQUAL(desc.IsVital(), TerrainData::IsVital(t));
            BOOST_REQUIRE_EQUAL(desc.minimapColor, TerrainData::GetColor(lt, t));
        }
    }
    // TerrainData::PrintEdgePrios();
}

BOOST_AUTO_TEST_CASE(TextureCoords)
{
    bfs::path basePath("testGameData");
    bfs::create_directories(basePath);
    {
        bnw::ofstream file(basePath / "default.lua");
        file << "rttr:AddLandscape{\
            name = \"testland\",\
            mapGfx = \"<RTTR_GAME>/DATA/MAP_0_Z.LST\",\
            roads = {\
                normal = { texture = \"<RTTR_GAME>/foo\"},\
                upgraded = { texture = \"<RTTR_GAME>/foo\"},\
                boat = { texture = \"<RTTR_GAME>/foo\"},\
                mountain = { texture = \"<RTTR_GAME>/foo\"}\
            }}";
        file << "rttr:AddTerrainEdge{ name = \"edge\", landscape = \"testland\", texture = \"<RTTR_GAME>/foo\" }";
        file << "rttr:AddTerrain{\
            name = \"terrain1\",\
            landscape = \"testland\", edgeType = \"edge\", texture = \"<RTTR_GAME>/foo\", color = 0, \
            pos = { 10, 20, 32, 31 }, texType = \"overlapped\" }";
        file << "rttr:AddTerrain{\
            name = \"terrain2\",\
            landscape = \"testland\", edgeType = \"edge\", texture = \"<RTTR_GAME>/foo\", color = 0, \
            pos = { 10, 20, 32, 31 }, texType = \"stacked\" }";
        file << "rttr:AddTerrain{\
            name = \"terrain3\",\
            landscape = \"testland\", edgeType = \"edge\", texture = \"<RTTR_GAME>/foo\", color = 0, \
            pos = { 10, 20, 32, 31 }, texType = \"rotated\" }";
    }
    WorldDescription desc;
    GameDataLoader loader(desc, basePath.string());
    BOOST_REQUIRE(loader.Load());
    typedef TerrainDesc::PointF PointF;
    // Border points are inset by half a pixel for OpenGL (sample middle of pixel!)
    // Overlapped uses the full rectangle
    const TerrainDesc::Triangle rsuO = desc.terrain.tryGet("terrain1")->GetRSUTriangle();
    BOOST_REQUIRE_EQUAL(rsuO.tip, PointF(10 + 32 / 2, 20 + 0.5f));
    BOOST_REQUIRE_EQUAL(rsuO.left, PointF(10 + 0.5f, 20 + 31 - 0.5f));
    BOOST_REQUIRE_EQUAL(rsuO.right, PointF(10 + 32 - 0.5f, 20 + 31 - 0.5f));
    const TerrainDesc::Triangle usdO = desc.terrain.tryGet("terrain1")->GetUSDTriangle();
    BOOST_REQUIRE_EQUAL(usdO.tip, PointF(10 + 32 / 2, 20 + 31 - 0.5f));
    BOOST_REQUIRE_EQUAL(usdO.left, PointF(10 + 0.5f, 20 + 0.5f));
    BOOST_REQUIRE_EQUAL(usdO.right, PointF(10 + 32 - 0.5f, 20 + 0.5f));

    // Stacked has RSU over USD
    const TerrainDesc::Triangle rsuS = desc.terrain.tryGet("terrain2")->GetRSUTriangle();
    BOOST_REQUIRE_EQUAL(rsuS.tip, rsuO.tip);
    BOOST_REQUIRE_EQUAL(rsuS.left, PointF(10 + 0.5f, 20 + 31 / 2.f));
    BOOST_REQUIRE_EQUAL(rsuS.right, PointF(10 + 32 - 0.5f, 20 + 31 / 2.f));
    const TerrainDesc::Triangle usdS = desc.terrain.tryGet("terrain2")->GetUSDTriangle();
    BOOST_REQUIRE_EQUAL(usdS.tip, usdO.tip);
    BOOST_REQUIRE_EQUAL(usdS.left, PointF(10 + 0.5f, 20 + 31 / 2.f));
    BOOST_REQUIRE_EQUAL(usdS.right, PointF(10 + 32 - 0.5f, 20 + 31 / 2.f));

    // Rotated is stacked sideways (note that order stays the same)
    const TerrainDesc::Triangle rsuR = desc.terrain.tryGet("terrain3")->GetRSUTriangle();
    BOOST_REQUIRE_EQUAL(rsuR.tip, rsuS.left);
    BOOST_REQUIRE_EQUAL(rsuR.left, rsuS.right);
    BOOST_REQUIRE_EQUAL(rsuR.right, rsuS.tip);
    const TerrainDesc::Triangle usdR = desc.terrain.tryGet("terrain3")->GetUSDTriangle();
    BOOST_REQUIRE_EQUAL(usdR.tip, usdS.right);
    BOOST_REQUIRE_EQUAL(usdR.left, usdS.tip);
    BOOST_REQUIRE_EQUAL(usdR.right, usdS.left);
}

BOOST_AUTO_TEST_SUITE_END()
