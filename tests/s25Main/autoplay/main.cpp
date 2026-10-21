// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#define BOOST_TEST_MODULE RTTR_AutoplayTest
#include "HeadlessReplay.h"
#include "Timer.h"
#include "helpers/chronoIO.h"
#include "ogl/glAllocator.h"
#include "random/Random.h"
#include "random/randomIO.h"
#include "test/testConfig.h"
#include "libsiedler2/libsiedler2.h"
#include <rttr/test/Fixture.hpp>
#include <s25util/boostTestHelpers.h>
#include <boost/test/unit_test.hpp>

#if RTTR_HAS_VLD
#    include <vld.h>
#endif

struct Fixture : rttr::test::Fixture
{
    Fixture() { libsiedler2::setAllocator(new GlAllocator); }
};
BOOST_GLOBAL_FIXTURE(Fixture);

static boost::test_tools::predicate_result verifyNoDesync(const std::optional<ReplayDesync>& desync)
{
    if(!desync)
        return true;
    // LCOV_EXCL_START
    boost::test_tools::predicate_result result(false);
    result.message() << "Desync at GF " << desync->gf << ":\n"
                     << desync->actual << " != \n"
                     << desync->expected << '\n';
    for(const auto& entry : RANDOM.GetAsyncLog())
        result.message() << entry << '\n';
    return result;
    // LCOV_EXCL_STOP
}

static void playReplay(const boost::filesystem::path& replayPath)
{
    HeadlessReplay replay(replayPath);

    const Timer timer(true);
    replay.Run();
    const auto duration = std::chrono::duration_cast<std::chrono::duration<float>>(timer.getElapsed());
    std::cout << "Replay " << replayPath.filename() << " took " << helpers::withUnit(duration) << std::endl;

    BOOST_TEST_REQUIRE(verifyNoDesync(replay.getDesync()));
}

BOOST_AUTO_TEST_CASE(Play200kReplay)
{
    // Map: Others/Big Slaughter v2
    // 7 x Hard KI
    // 2 KIs each in Teams 1-3, 1 in Team 4
    // Player KI without team ("WINTER" + F10)
    // Default addon settings
    // Save immediately, then load (so savegame is embedded instead of map)
    // 200k GFs run (+ a bit)
    const boost::filesystem::path replayPath = rttr::test::rttrBaseDir / "tests" / "testData" / "200kGFs.rpl";
    playReplay(replayPath);
}

BOOST_AUTO_TEST_CASE(PlaySeaReplay)
{
    // Map: Island by Island
    // 2 x Hard KI + Player KI ("WINTER" + F10)
    // No teams, Sea attacks enabled (harbors block), ships fast
    // 300k GFs run (+ a bit)
    const boost::filesystem::path replayPath = rttr::test::rttrBaseDir / "tests" / "testData" / "SeaMap300kGfs.rpl";
    playReplay(replayPath);
}
