// Copyright (C) 2005 - 2022 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "PointOutput.h"
#include "worldFixtures/CreateEmptyWorld.h"
#include "worldFixtures/WorldFixture.h"
#include "world/GameWorldView.h"
#include "world/GameWorldViewer.h"
#include "gameData/MapConsts.h"
#include "rttr/test/random.hpp"
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(GameWorldViewTests)

namespace {
using EmptyWorldFixture1P = WorldFixture<CreateEmptyWorld, 1>;
} // namespace

BOOST_FIXTURE_TEST_CASE(HasCorrectDrawCoords, EmptyWorldFixture1P)
{
    GameWorldViewer gwv(0, world);
    const auto viewSize = rttr::test::randomPoint<Extent>(100, 1000);
    GameWorldView view(gwv, Position(0, 0), viewSize);
    BOOST_TEST(view.GetPos() == Position(0, 0));
    BOOST_TEST(view.GetSize() == viewSize);
    BOOST_TEST(view.GetFirstPt() == Position(-1, -1));
    BOOST_TEST(view.GetLastPt().x > static_cast<int>(viewSize.x / TR_W));
    BOOST_TEST(view.GetLastPt().y > static_cast<int>(viewSize.y / TR_H));

    const auto origFirstPt = view.GetFirstPt();
    const auto origLastPt = view.GetLastPt();
    auto newOffset = rttr::test::randomPoint<DrawPoint>(100, 200);
    BOOST_TEST_REQUIRE(view.GetOffset() == DrawPoint(0, 0));
    view.MoveTo(newOffset);
    BOOST_TEST_REQUIRE(view.GetOffset() == newOffset);
    const auto offsetFirstPt = view.GetFirstPt();
    const auto offsetLastPt = view.GetLastPt();
    BOOST_TEST(offsetFirstPt.x > origFirstPt.x);
    BOOST_TEST(offsetFirstPt.y > origFirstPt.y);
    BOOST_TEST(offsetLastPt.x > origLastPt.x);
    BOOST_TEST(offsetLastPt.y > origLastPt.y);
    // Relative move
    {
        const auto offset = rttr::test::randomPoint<Position>(-100, 70); // Smallish max offset to not exceed map size
        view.MoveBy(offset);
        BOOST_TEST_REQUIRE(view.GetOffset() == newOffset + offset);
    }

    // Don't move if moved by multiples of total size
    {
        const DrawPoint drawSize(world.GetSize() * DrawPoint(TR_W, TR_H));
        // Absolute
        view.MoveTo(newOffset + drawSize);
        BOOST_TEST_REQUIRE(view.GetOffset() == newOffset);
        // Relative +-
        view.MoveBy(drawSize);
        BOOST_TEST_REQUIRE(view.GetOffset() == newOffset);
        view.MoveBy(drawSize);
        BOOST_TEST_REQUIRE(view.GetOffset() == newOffset);
        // Absolute by multiples of the size
        const auto offset = rttr::test::randomPoint<Position>(1, 10);
        view.MoveTo(newOffset + (drawSize * rttr::test::randomPoint<Position>(2, 5)) + offset);
        BOOST_TEST_REQUIRE(view.GetOffset() == newOffset + offset);
    }

    // Move back to start restores first/last points
    view.MoveTo({0, 0});
    BOOST_TEST_REQUIRE(view.GetOffset() == DrawPoint(0, 0));
    BOOST_TEST(view.GetFirstPt() == origFirstPt);
    BOOST_TEST(view.GetLastPt() == origLastPt);
    // Relative move also updates first/last point
    view.MoveBy(newOffset);
    BOOST_TEST_REQUIRE(view.GetOffset() == newOffset);
    BOOST_TEST(view.GetFirstPt() == offsetFirstPt);
    BOOST_TEST(view.GetLastPt() == offsetLastPt);
}

BOOST_FIXTURE_TEST_CASE(GetsCorrectMaxHeight, EmptyWorldFixture1P)
{
    GameWorldViewer gwv(0, world);
    const auto viewSize = rttr::test::randomPoint<Extent>(100, 1000);
    GameWorldView view(gwv, Position(0, 0), viewSize);
    BOOST_TEST(gwv.getMaxNodeAltitude() == world.GetNode(MapPoint(0, 0)).altitude); // All same height

    {
        EmptyWorldFixture1P world2;
        const auto newMaxHeight = world2.world.GetNodeWriteable(MapPoint(0, 0)).altitude *= 2;
        GameWorldViewer gwv2(0, world2.world);
        BOOST_TEST(gwv2.getMaxNodeAltitude() == newMaxHeight);
        GameWorldView view2(gwv2, Position(0, 0), viewSize);
        BOOST_TEST(view2.GetFirstPt() == view.GetFirstPt());
        BOOST_TEST(view2.GetLastPt().x == view.GetLastPt().x);
        BOOST_TEST(view2.GetLastPt().y > view.GetLastPt().y); // Should have increased due to higher altitude

        // React on height changes, at least after moves
        world.ChangeAltitude(MapPoint(0, 0), newMaxHeight);
        view.MoveBy(DrawPoint(0, 0)); // no-op move
        BOOST_TEST(gwv.getMaxNodeAltitude() == newMaxHeight);
        BOOST_TEST(view.GetFirstPt() == view2.GetFirstPt());
        BOOST_TEST(view.GetLastPt() == view2.GetLastPt());
    }
}

BOOST_AUTO_TEST_SUITE_END()
