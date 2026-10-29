// Copyright (C) 2005 - 2025 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "PointOutput.h"
#include "TerrainBQOutput.h"
#include "files.h"
#include "helpers/Range.h"
#include "helpers/containerUtils.h"
#include "legacy/TerrainData.h"
#include "lua/GameDataLoader.h"
#include "gameData/EdgeDesc.h"
#include "gameData/TerrainDesc.h"
#include "gameData/WorldDescription.h"
#include "rttr/test/ConfigOverride.hpp"
#include "rttr/test/LogAccessor.hpp"
#include "rttr/test/TmpFolder.hpp"
#include <boost/filesystem.hpp>
#include <boost/nowide/fstream.hpp>
#include <boost/test/unit_test.hpp>
#include <sstream>

namespace bfs = boost::filesystem;
namespace bnw = boost::nowide;

BOOST_AUTO_TEST_SUITE(GameData)

BOOST_AUTO_TEST_CASE(BQ_Output)
{
    std::stringstream s;
    s << TerrainBQ::Nothing;
    BOOST_TEST(s.str() == "Nothing");
    s.str("");
    s << TerrainBQ::Mine;
    BOOST_TEST(s.str() == "Mine");
}

template<class Container, class Predicate>
static std::vector<std::string> getNames(const Container& items, Predicate&& predicate)
{
    std::vector<std::string> result;
    for(const auto& item : items)
    {
        if(predicate(item))
            result.push_back(item.name);
    }
    return result;
}

BOOST_AUTO_TEST_CASE(IndexingToDescriptionVector)
{
    struct name_t
    {};
    using idx_t = DescIdx<name_t>;
    {
        // Default construct index is false-ish
        idx_t idx;
        BOOST_TEST(!idx);
        // With a value it is true-ish
        idx = idx_t(5);
        BOOST_TEST(!!idx);
    }

    DescriptionVector<std::string, idx_t::index_type> vec;
    vec.push_back("0");
    vec.push_back("1");
    vec.push_back("2");
    BOOST_TEST(vec[idx_t(0)] == "0");
    BOOST_TEST(vec[idx_t(1)] == "1");
    BOOST_TEST(vec[idx_t(2)] == "2");

    std::vector<idx_t> indices;
    for(const auto i : vec.indices())
        indices.push_back(i);
    BOOST_TEST((indices == std::vector<idx_t>{idx_t(0), idx_t(1), idx_t(2)}));
}

BOOST_AUTO_TEST_CASE(LoadGameData)
{
    WorldDescription worldDesc;
    loadGameData(worldDesc);
    BOOST_TEST(worldDesc.edges.size() == 3u * 5u - 1u);
    BOOST_TEST(worldDesc.terrain.size() >= 3u * NUM_TTS);
    for(unsigned l = 0; l < NUM_LTS; l++)
    {
        // indexOf(name) in these arrays should be the original RTTR index as in TerrainData
        const std::vector<std::string> tNames = getNames(
          worldDesc.terrain, [l, &worldDesc](const TerrainDesc& t) { return worldDesc.get(t.landscape).s2Id == l; });
        const std::vector<std::string> eNames = getNames(
          worldDesc.edges, [l, &worldDesc](const EdgeDesc& e) { return worldDesc.get(e.landscape).s2Id == l; });
        const auto lt = Landscape(l);
        for(const unsigned i : helpers::range(NUM_TTS))
        {
            const auto t = TerrainType(i);
            const TerrainDesc& desc = worldDesc.terrain.get(worldDesc.terrain.getIndex(tNames[i]));
            BOOST_TEST(desc.s2Id == TerrainData::GetTextureIdentifier(t));
            const EdgeType newEdge =
              !desc.edgeType ? ET_NONE : EdgeType((helpers::indexOf(eNames, worldDesc.get(desc.edgeType).name) + 1));
            BOOST_TEST(newEdge == TerrainData::GetEdgeType(lt, t));
            BOOST_TEST(desc.GetBQ() == TerrainData::GetBuildingQuality(t));
            BOOST_TEST(!desc.Is(ETerrain::Walkable)
                       == (TerrainData::GetBuildingQuality(t) == TerrainBQ::Nothing
                           || TerrainData::GetBuildingQuality(t) == TerrainBQ::Danger));
            BOOST_TEST(desc.Is(ETerrain::Mineable) == (TerrainData::GetBuildingQuality(t) == TerrainBQ::Mine));
            BOOST_TEST(desc.Is(ETerrain::Buildable) == (TerrainData::GetBuildingQuality(t) == TerrainBQ::Castle));
            BOOST_TEST(desc.Is(ETerrain::Shippable) == TerrainData::IsUsableByShip(t));
            BOOST_TEST((desc.kind == TerrainKind::Snow) == TerrainData::IsSnow(lt, t));

            BOOST_TEST((desc.IsUsableByAnimals() || desc.kind == TerrainKind::Snow)
                       == (TerrainData::IsUsableByAnimals(t) || TerrainData::IsSnow(lt, t)));
            BOOST_TEST(desc.IsVital() == TerrainData::IsVital(t));
            BOOST_TEST(desc.minimapColor == TerrainData::GetColor(lt, t));
        }
    }
    // TerrainData::PrintEdgePrios();
}

BOOST_AUTO_TEST_CASE(DetectError)
{
    rttr::test::TmpFolder tmp;
    rttr::test::ConfigOverride _("RTTR", tmp);
    auto worldPath = RTTRCONFIG.ExpandPath(s25::folders::gamedata) / "world";
    create_directories(worldPath);
    {
        bnw::ofstream file(worldPath / "default.lua");
        BOOST_TEST_REQUIRE(!!(file << "empty = true"));
    }
    WorldDescription desc;
    BOOST_REQUIRE_NO_THROW(loadGameData(desc));

    {
        bnw::ofstream file(worldPath / "default.lua");
        BOOST_TEST_REQUIRE(!!(file << "syntax{-error"));
    }
    GameDataLoader loader(desc, tmp);
    rttr::test::LogAccessor logAcc;
    BOOST_TEST(!loader.Load());
    BOOST_CHECK_THROW(loadGameData(desc), std::runtime_error);
}

BOOST_AUTO_TEST_CASE(DetectRecursion)
{
    rttr::test::TmpFolder tmp;
    {
        bnw::ofstream file(tmp / "default.lua");
        file << "include(\"foo.lua\")\n";
        bnw::ofstream file2(tmp / "foo.lua");
        file2 << "include(\"default.lua\")\n";
    }
    WorldDescription desc;
    GameDataLoader loader(desc, tmp);
    rttr::test::LogAccessor logAcc;
    BOOST_TEST(!loader.Load());
    RTTR_REQUIRE_LOG_CONTAINS("Maximum include depth", false);
}

BOOST_AUTO_TEST_CASE(DetectInvalidFilenames)
{
    rttr::test::TmpFolder tmp;
    {
        bnw::ofstream file(tmp / "default.lua");
        file << "include(\"foo(=.lua\")\n";
    }
    WorldDescription desc;
    GameDataLoader loader(desc, tmp);
    rttr::test::LogAccessor logAcc;
    BOOST_TEST(!loader.Load());
    RTTR_REQUIRE_LOG_CONTAINS("disallowed chars", false);
}

BOOST_AUTO_TEST_CASE(DetectNonexistingFile)
{
    rttr::test::TmpFolder tmp;
    {
        bnw::ofstream file(tmp / "default.lua");
        file << "include(\"foo.lua\")\n";
    }
    WorldDescription desc;
    GameDataLoader loader(desc, tmp);
    rttr::test::LogAccessor logAcc;
    BOOST_TEST(!loader.Load());
    RTTR_REQUIRE_LOG_CONTAINS("File not found", false);
}

BOOST_AUTO_TEST_CASE(DetectWrongExtension)
{
    rttr::test::TmpFolder tmp;
    {
        bnw::ofstream file(tmp / "default.lua");
        file << "include(\"foo.txt\")\n";
        bnw::ofstream file2(tmp / "foo.txt");
    }
    WorldDescription desc;
    GameDataLoader loader(desc, tmp);
    rttr::test::LogAccessor logAcc;
    BOOST_TEST(!loader.Load());
    RTTR_REQUIRE_LOG_CONTAINS("File must have .lua as the extension", false);
}

BOOST_AUTO_TEST_CASE(DetectFolderEscape)
{
    rttr::test::TmpFolder tmp;
    bfs::path basePath(tmp / "gameData");
    create_directories(basePath);
    {
        bnw::ofstream file(basePath / "default.lua");
        file << "include(\"../foo.lua\")\n";
        bnw::ofstream file2(tmp / "foo.lua");
    }
    WorldDescription desc;
    GameDataLoader loader(desc, basePath);
    rttr::test::LogAccessor logAcc;
    BOOST_TEST(!loader.Load());
    RTTR_REQUIRE_LOG_CONTAINS("outside the lua data directory", false);
}

#ifndef BOOST_WINDOWS_API
BOOST_AUTO_TEST_CASE(DetectAbsolute)
{
    rttr::test::TmpFolder tmp;
    bfs::path basePath(tmp / "gameData");
    create_directories(basePath);
    {
        bnw::ofstream file(basePath / "default.lua");
        file << "include(\"/tmp/foo.lua\")\n";
    }
    WorldDescription desc;
    GameDataLoader loader(desc, basePath);
    rttr::test::LogAccessor logAcc;
    BOOST_TEST(!loader.Load());
    RTTR_REQUIRE_LOG_CONTAINS("relative", false);
}
#endif

BOOST_AUTO_TEST_CASE(TextureCoords)
{
    rttr::test::TmpFolder tmp;
    {
        bnw::ofstream file(tmp / "default.lua");
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
            pos = { 10, 20, 31, 33 }, texType = \"stacked\" }";
        file << "rttr:AddTerrain{\
            name = \"terrain3\",\
            landscape = \"testland\", edgeType = \"edge\", texture = \"<RTTR_GAME>/foo\", color = 0, \
            pos = { 10, 20, 53, 54 }, texType = \"rotated\" }";
        file << "rttr:AddTerrain{\
            name = \"terrain4\",\
            landscape = \"testland\", edgeType = \"edge\", texture = \"<RTTR_GAME>/foo\", color = 0, \
            pos = { 10, 20, 64, 62 }, texType = \"overlapped\" }";
    }
    WorldDescription desc;
    GameDataLoader loader(desc, tmp);
    BOOST_TEST_REQUIRE(loader.Load());

    const auto checkTriangle = [](const TerrainDesc::Triangle& triangle, const TerrainDesc::Triangle& expected) {
        const PointF origin(10, 20);
        for(const auto& [pt, expectedPt] :
            {std::pair(triangle.tip, expected.tip), std::pair(triangle.left, expected.left),
             std::pair(triangle.right, expected.right)})
        {
            BOOST_TEST((pt - origin).x == expectedPt.x, boost::test_tools::tolerance(0.001f));
            BOOST_TEST((pt - origin).y == expectedPt.y, boost::test_tools::tolerance(0.001f));
        }
    };
    // Points are where S2 samples the texture, relative to the rectangle.
    // Overlapped uses the full rectangle, about half a texel further right than the pixel centers
    const TerrainDesc* terrain = desc.terrain.tryGet("terrain1");
    checkTriangle(terrain->GetRSUTriangle(), {{16.549f, 0.594f}, {1.162f, 30.340f}, {31.934f, 30.340f}});
    checkTriangle(terrain->GetUSDTriangle(), {{16.549f, 30.340f}, {1.162f, 0.594f}, {31.934f, 0.594f}});

    // Stacked has RSU over USD
    terrain = desc.terrain.tryGet("terrain2");
    checkTriangle(terrain->GetRSUTriangle(), {{15.515f, 1.310f}, {1.093f, 15.190f}, {29.934f, 15.190f}});
    checkTriangle(terrain->GetUSDTriangle(), {{15.515f, 31.190f}, {1.093f, 17.310f}, {29.934f, 17.310f}});

    // Rotated is the upper (RSU) and lower (USD) half of a diamond
    terrain = desc.terrain.tryGet("terrain3");
    checkTriangle(terrain->GetRSUTriangle(), {{0.698f, 25.831f}, {49.650f, 26.250f}, {24.546f, 1.146f}});
    checkTriangle(terrain->GetUSDTriangle(), {{50.098f, 27.698f}, {26.250f, 52.383f}, {1.146f, 27.279f}});

    // Other sizes are scaled
    terrain = desc.terrain.tryGet("terrain4");
    checkTriangle(terrain->GetRSUTriangle(), {{33.098f, 1.188f}, {2.324f, 60.680f}, {63.868f, 60.680f}});
    checkTriangle(terrain->GetUSDTriangle(), {{33.098f, 60.680f}, {2.324f, 1.188f}, {63.868f, 1.188f}});
}

BOOST_AUTO_TEST_SUITE_END()
