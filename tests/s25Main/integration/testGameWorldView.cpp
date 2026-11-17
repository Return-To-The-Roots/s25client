// Copyright (C) 2005 - 2022 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "PointOutput.h"
#include "uiHelper/uiHelpers.hpp"
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
    uiHelper::initGUITests(); // Required for GameWorldView

    GameWorldViewer gwv(0, world);
    const auto viewSize = rttr::test::randomPoint<Extent>(100, 1000);
    GameWorldView view(gwv, Position(0, 0), viewSize);
    BOOST_TEST(view.getPos() == Position(0, 0));
    BOOST_TEST(view.getSize() == viewSize);
    BOOST_TEST(view.getFirstPt() == Position(-1, -1));
    BOOST_TEST(view.getLastPt().x > static_cast<int>(viewSize.x / TR_W));
    BOOST_TEST(view.getLastPt().y > static_cast<int>(viewSize.y / TR_H));

    const auto origFirstPt = view.getFirstPt();
    const auto origLastPt = view.getLastPt();
    auto newOffset = rttr::test::randomPoint<DrawPoint>(100, 200);
    BOOST_TEST_REQUIRE(view.getOffset() == DrawPoint(0, 0));
    view.moveTo(newOffset);
    BOOST_TEST_REQUIRE(view.getOffset() == newOffset);
    const auto offsetFirstPt = view.getFirstPt();
    const auto offsetLastPt = view.getLastPt();
    BOOST_TEST(offsetFirstPt.x > origFirstPt.x);
    BOOST_TEST(offsetFirstPt.y > origFirstPt.y);
    BOOST_TEST(offsetLastPt.x > origLastPt.x);
    BOOST_TEST(offsetLastPt.y > origLastPt.y);
    // Relative move
    {
        const auto offset = rttr::test::randomPoint<Position>(-100, 70); // Smallish max offset to not exceed map size
        view.moveBy(offset);
        BOOST_TEST_REQUIRE(view.getOffset() == newOffset + offset);
    }

    // Don't move if moved by multiples of total size
    {
        const DrawPoint drawSize(world.getSize() * DrawPoint(TR_W, TR_H));
        // Absolute
        view.moveTo(newOffset + drawSize);
        BOOST_TEST_REQUIRE(view.getOffset() == newOffset);
        // Relative +-
        view.moveBy(drawSize);
        BOOST_TEST_REQUIRE(view.getOffset() == newOffset);
        view.moveBy(drawSize);
        BOOST_TEST_REQUIRE(view.getOffset() == newOffset);
        // Absolute by multiples of the size
        const auto offset = rttr::test::randomPoint<Position>(1, 10);
        view.moveTo(newOffset + (drawSize * rttr::test::randomPoint<Position>(2, 5)) + offset);
        BOOST_TEST_REQUIRE(view.getOffset() == newOffset + offset);
    }

    // Move back to start restores first/last points
    view.moveTo({0, 0});
    BOOST_TEST_REQUIRE(view.getOffset() == DrawPoint(0, 0));
    BOOST_TEST(view.getFirstPt() == origFirstPt);
    BOOST_TEST(view.getLastPt() == origLastPt);
    // Relative move also updates first/last point
    view.moveBy(newOffset);
    BOOST_TEST_REQUIRE(view.getOffset() == newOffset);
    BOOST_TEST(view.getFirstPt() == offsetFirstPt);
    BOOST_TEST(view.getLastPt() == offsetLastPt);
}

BOOST_FIXTURE_TEST_CASE(GetsCorrectMaxHeight, EmptyWorldFixture1P)
{
    uiHelper::initGUITests(); // Required for GameWorldView

    GameWorldViewer gwv(0, world);
    const auto viewSize = rttr::test::randomPoint<Extent>(100, 1000);
    GameWorldView view(gwv, Position(0, 0), viewSize);
    BOOST_TEST(gwv.getMaxNodeAltitude() == world.getNode(MapPoint(0, 0)).altitude); // All same height

    {
        EmptyWorldFixture1P world2;
        const auto newMaxHeight = world2.world.getNodeWriteable(MapPoint(0, 0)).altitude *= 2;
        GameWorldViewer gwv2(0, world2.world);
        BOOST_TEST(gwv2.getMaxNodeAltitude() == newMaxHeight);
        GameWorldView view2(gwv2, Position(0, 0), viewSize);
        BOOST_TEST(view2.getFirstPt() == view.getFirstPt());
        BOOST_TEST(view2.getLastPt().x == view.getLastPt().x);
        BOOST_TEST(view2.getLastPt().y > view.getLastPt().y); // Should have increased due to higher altitude

        // React on height changes, at least after moves
        world.changeAltitude(MapPoint(0, 0), newMaxHeight);
        view.moveBy(DrawPoint(0, 0)); // no-op move
        BOOST_TEST(gwv.getMaxNodeAltitude() == newMaxHeight);
        BOOST_TEST(view.getFirstPt() == view2.getFirstPt());
        BOOST_TEST(view.getLastPt() == view2.getLastPt());
    }
}

BOOST_AUTO_TEST_SUITE_END()
