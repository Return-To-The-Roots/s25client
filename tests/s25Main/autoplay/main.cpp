// Copyright (c) 2021 - 2021 Settlers Freaks (sf-team at siedler25.org)
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

BOOST_TEST_DONT_PRINT_LOG_VALUE(AsyncChecksum)

static void playReplay(const boost::filesystem::path& replayPath)
{
    Replay replay;
    BOOST_TEST_REQUIRE(replay.LoadHeader(replayPath, true));
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
    RANDOM.Init(replay.random_init);
    GameWorld& gameWorld = game.world_;

    for(unsigned i = 0; i < gameWorld.GetNumPlayers(); ++i)
        gameWorld.GetPlayer(i).MakeStartPacts();

    MapLoader loader(gameWorld);
    BOOST_TEST_REQUIRE(loader.Load(mapfile.filePath));
    gameWorld.SetupResources();
    gameWorld.InitAfterLoad();

    bool endOfReplay = false;
    unsigned nextGF;
    BOOST_TEST_REQUIRE(replay.ReadGF(&nextGF));

    const Timer timer(true);
    do
    {
        unsigned curGF = game.em_->GetCurrentGF();
        AsyncChecksum checksum;
        if(nextGF == curGF)
            checksum = AsyncChecksum::create(game);
        while(nextGF == curGF)
        {
            BOOST_TEST_INFO("Current GF: " << curGF);
            const ReplayCommand rc = replay.ReadRCType();

            if(rc == ReplayCommand::Chat)
            {
                uint8_t player, dest;
                std::string message;
                replay.ReadChatCommand(player, dest, message);
            } else if(rc == ReplayCommand::Game)
            {
                PlayerGameCommands msg;
                uint8_t gcPlayer;
                replay.ReadGameCommand(gcPlayer, msg);
                for(const gc::GameCommandPtr& gc : msg.gcs)
                    gc->Execute(game.world_, gcPlayer);
                AsyncChecksum& msgChecksum = msg.checksum;
                if(msgChecksum.randChecksum != 0)
                    BOOST_TEST_REQUIRE(msgChecksum == checksum);
            }
            if(!replay.ReadGF(&nextGF))
            {
                endOfReplay = true;
                break;
            } else
                BOOST_TEST_REQUIRE(nextGF <= replay.GetLastGF());
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
    // 200k GFs run (+ a bit)
    const boost::filesystem::path replayPath = rttr::test::rttrBaseDir / "tests" / "testData" / "200kGFs.rpl";
    playReplay(replayPath);
}

BOOST_AUTO_TEST_CASE(PlaySeaReplay)
{
    // Map: Island by Island
    // 2 x Hard KI + Player KI
    // No teams, Sea attacks enabled, ships fast
    // 300k GFs run (+ a bit)
    const boost::filesystem::path replayPath = rttr::test::rttrBaseDir / "tests" / "testData" / "SeaMap300kGfs.rpl";
    playReplay(replayPath);
}
