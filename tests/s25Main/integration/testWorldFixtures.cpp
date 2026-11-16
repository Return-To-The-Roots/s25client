// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "worldFixtures/CreateEmptyWorld.h"
#include "worldFixtures/WorldFixture.h"
#include <boost/test/unit_test.hpp>

namespace {
struct TestGO : GameObject
{
    // LCOV_EXCL_START
    void destroy() override final{};
    void Serialize(SerializedGameData&) const override final{};
    GO_Type getGOT() const override final { return GO_Type::Animal; }
    // LCOV_EXCL_STOP

    void handleEvent(unsigned) override final { handled = true; }
    bool handled = false;
};
} // namespace

using WorldFixtureEmpty = WorldFixture<CreateEmptyWorld, 0, 6, 6>;
BOOST_FIXTURE_TEST_CASE(SkipGFs_Works, WorldFixtureEmpty)
{
    // No events -> Nothing to do
    BOOST_TEST(em.getNumActiveEvents() == 0u);
    RTTR_SKIP_GFS(0);
    BOOST_TEST(em.getCurrentGF() == 0u);
    BOOST_TEST(em.executeNextEvent(0) == 0u);
    BOOST_TEST(em.getCurrentGF() == 0u);
    RTTR_SKIP_GFS(10);
    BOOST_TEST(em.getCurrentGF() == 10u);
    BOOST_TEST(em.executeNextEvent(5) == 0u);
    BOOST_TEST(em.executeNextEvent(10) == 0u);
    BOOST_TEST(em.getCurrentGF() == 10u);

    TestGO go;
    em.addEvent(&go, 10);
    RTTR_SKIP_GFS(5); // No event triggered
    BOOST_TEST(em.getCurrentGF() == 15u);
    BOOST_TEST(!go.handled);
    RTTR_SKIP_GFS(7); // event triggered
    BOOST_TEST(em.getCurrentGF() == 22u);
    BOOST_TEST(go.handled);

    go.handled = false;
    em.addEvent(&go, 8); // at +20

    unsigned numGF;
    RTTR_EXEC_TILL_CT_GF(100, go.handled, numGF);
    BOOST_TEST(em.getCurrentGF() == 30u);
    BOOST_TEST(numGF == 8u);
}
