// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#define BOOST_TEST_MODULE RTTR_AutoplayTest
#include "EventManager.h"
#include "Game.h"
#include "GamePlayer.h"
#include "Replay.h"
#include "Timer.h"
#include "helpers/chronoIO.h"
#include "network/PlayerGameCommands.h"
#include "ogl/glAllocator.h"
#include "random/Random.h"
#include "random/randomIO.h"
#include "variant.h"
#include "world/GameWorld.h"
#include "world/MapLoader.h"
#include "gameTypes/MapInfo.h"
#include "test/testConfig.h"
#include "libsiedler2/libsiedler2.h"
#include "s25util/tmpFile.h"
#include <rttr/test/Fixture.hpp>
#include <boost/test/unit_test.hpp>

#if RTTR_HAS_VLD
#    include <vld.h>
#endif

struct Fixture : rttr::test::Fixture
{
    Fixture() { libsiedler2::setAllocator(new GlAllocator); }
};
BOOST_GLOBAL_FIXTURE(Fixture);

static boost::test_tools::predicate_result verifyChecksum(const AsyncChecksum& actual, const AsyncChecksum& expected,
                                                          const bool fail = false)
{
    if(!fail && (expected.randChecksum == 0 || actual == expected))
        return true;
    // LCOV_EXCL_START
    boost::test_tools::predicate_result result(false);
    result.message() << '\n' << actual << " != \n" << expected << '\n';
    for(const auto& entry : RANDOM.GetAsyncLog())
        result.message() << entry << '\n';
    return result;
    // LCOV_EXCL_STOP
}

static void playReplay(const boost::filesystem::path& replayPath)
{
    Replay replay;
    BOOST_TEST_REQUIRE(replay.LoadHeader(replayPath));
    MapInfo mapInfo;
    BOOST_TEST_REQUIRE(replay.LoadGameData(mapInfo));
    BOOST_TEST_REQUIRE(!mapInfo.savegame); // Must be from start
    TmpFile mapfile;
    mapfile.close();
    BOOST_TEST_REQUIRE(mapInfo.mapData.DecompressToFile(mapfile.filePath));

    std::vector<PlayerInfo> players;
    for(unsigned i = 0; i < replay.GetNumPlayers(); i++)
        players.emplace_back(replay.GetPlayer(i));
    Game game(replay.ggs, /*startGF*/ 0, players);
    RANDOM.Init(replay.getSeed());
    GameWorld& gameWorld = game.world_;

    for(unsigned i = 0; i < gameWorld.GetNumPlayers(); ++i)
        gameWorld.GetPlayer(i).MakeStartPacts();

    MapLoader loader(gameWorld);
    BOOST_TEST_REQUIRE(loader.Load(mapfile.filePath));
    gameWorld.SetupResources();
    gameWorld.InitAfterLoad();

    bool endOfReplay = false;
    auto nextGF = replay.ReadGF();
    BOOST_TEST_REQUIRE(nextGF.has_value());

    const Timer timer(true);
    do
    {
        const unsigned curGF = game.em_->GetCurrentGF();
        AsyncChecksum checksum;
        if(*nextGF == curGF)
            checksum = AsyncChecksum::create(game);
        while(*nextGF == curGF)
        {
            BOOST_TEST_INFO("Current GF: " << curGF);
            const auto cmd = replay.ReadCommand();
            visit(composeVisitor([](const Replay::ChatCommand&) {},
                                 [&](const Replay::GameCommand& cmd) {
                                     for(const gc::GameCommandPtr& gc : cmd.cmds.gcs)
                                         gc->Execute(game.world_, cmd.player);
                                     BOOST_TEST_REQUIRE(verifyChecksum(checksum, cmd.cmds.checksum));
                                 }),
                  cmd);
            nextGF = replay.ReadGF();
            if(!nextGF)
            {
                endOfReplay = true;
                break;
            } else
                BOOST_TEST_REQUIRE(*nextGF <= replay.GetLastGF());
        }
        game.RunGF();
    } while(!endOfReplay);
    const auto duration = std::chrono::duration_cast<std::chrono::duration<float>>(timer.getElapsed());
    std::cout << "Replay " << replayPath.filename() << " took " << helpers::withUnit(duration) << std::endl;
}

BOOST_AUTO_TEST_CASE(Play200kReplay)
{
    // Map: Big Slaughter v2
    // 7 x Hard KI
    // 2 KIs each in Teams 1-3, 1 in Team 4
    // Player KI without team ("WINTER" + F10)
    // Default addon settings
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
