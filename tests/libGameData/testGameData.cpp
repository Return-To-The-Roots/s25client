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
#include <cmath>
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

    using Triangle = TerrainDesc::Triangle;
    const auto getTriangles = [&desc](const std::string& name) {
        // Relative to the rectangle
        const PointF origin(10, 20);
        const TerrainDesc& terrain = *desc.terrain.tryGet(name);
        const Triangle rsu = terrain.GetRSUTriangle(), usd = terrain.GetUSDTriangle();
        return std::pair(Triangle{rsu.tip - origin, rsu.left - origin, rsu.right - origin},
                         Triangle{usd.tip - origin, usd.left - origin, usd.right - origin});
    };
    // Never sample outside the rectangle (black lines, #896)
    const auto checkInside = [](const Triangle& triangle, const PointF& size) {
        for(const PointF& pt : {triangle.tip, triangle.left, triangle.right})
        {
            BOOST_TEST(pt.x > 0.f);
            BOOST_TEST(pt.y > 0.f);
            BOOST_TEST(pt.x < size.x);
            BOOST_TEST(pt.y < size.y);
        }
    };
    const auto tolerance = boost::test_tools::tolerance(0.001f);
    const auto checkEqual = [tolerance](const PointF& pt, const PointF& expected) {
        BOOST_TEST(pt.x == expected.x, tolerance);
        BOOST_TEST(pt.y == expected.y, tolerance);
    };

    // Overlapped uses the full rectangle
    const auto [rsuO, usdO] = getTriangles("terrain1");
    checkInside(rsuO, PointF(32, 31));
    checkInside(usdO, PointF(32, 31));
    // Tip is centered between the other corners
    BOOST_TEST(rsuO.tip.x == (rsuO.left.x + rsuO.right.x) / 2, tolerance);
    BOOST_TEST(rsuO.left.y == rsuO.right.y, tolerance);
    // Pixel centers sample where S2 does, i.e. about 1 pixel further right (31 texels over 56 pixels)
    // than the middle of the rectangle. Vertically they stay close to the pixel centers of the rows.
    BOOST_TEST(rsuO.tip.x == 32 / 2.f + 31 / 56.f, tolerance);
    BOOST_TEST(std::abs(rsuO.tip.y - 0.5f) < 0.2f);
    BOOST_TEST(std::abs(rsuO.left.y - (31 - 0.5f)) < 0.2f);
    // USD is RSU upside down
    checkEqual(usdO.tip, PointF(rsuO.tip.x, rsuO.left.y));
    checkEqual(usdO.left, PointF(rsuO.left.x, rsuO.tip.y));
    checkEqual(usdO.right, PointF(rsuO.right.x, rsuO.tip.y));

    // Stacked has RSU over USD
    const auto [rsuS, usdS] = getTriangles("terrain2");
    checkInside(rsuS, PointF(31, 33));
    checkInside(usdS, PointF(31, 33));
    BOOST_TEST(rsuS.tip.x == (rsuS.left.x + rsuS.right.x) / 2, tolerance);
    BOOST_TEST(rsuS.left.y == rsuS.right.y, tolerance);
    BOOST_TEST(usdS.left.y == usdS.right.y, tolerance);
    BOOST_TEST(rsuS.left.y < usdS.left.y);
    BOOST_TEST(usdS.tip.x == rsuS.tip.x, tolerance);
    BOOST_TEST(usdS.left.x == rsuS.left.x, tolerance);
    BOOST_TEST(usdS.right.x == rsuS.right.x, tolerance);
    BOOST_TEST(usdS.tip.y - usdS.left.y == rsuS.left.y - rsuS.tip.y, tolerance);

    // Rotated is the upper (RSU) and lower (USD) half of a diamond with the tips at the left (RSU) and right (USD)
    const auto [rsuR, usdR] = getTriangles("terrain3");
    checkInside(rsuR, PointF(53, 54));
    checkInside(usdR, PointF(53, 54));
    BOOST_TEST(rsuR.tip.x < rsuR.right.x);
    BOOST_TEST(rsuR.right.x < rsuR.left.x);
    BOOST_TEST(rsuR.right.y < rsuR.tip.y);
    BOOST_TEST(usdR.right.x < usdR.left.x);
    BOOST_TEST(usdR.left.x < usdR.tip.x);
    BOOST_TEST(usdR.left.y > usdR.tip.y);
    BOOST_TEST(rsuR.tip.y < usdR.right.y);

    // Other sizes are scaled
    const auto [rsuO2, usdO2] = getTriangles("terrain4");
    for(const auto& [triangle, expected] : {std::pair(rsuO2, rsuO), std::pair(usdO2, usdO)})
    {
        checkEqual(triangle.tip, expected.tip * 2.f);
        checkEqual(triangle.left, expected.left * 2.f);
        checkEqual(triangle.right, expected.right * 2.f);
    }
}

BOOST_AUTO_TEST_SUITE_END()
