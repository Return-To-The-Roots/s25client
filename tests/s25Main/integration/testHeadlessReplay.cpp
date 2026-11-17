// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "HeadlessReplay.h"
#include "PlayerInfo.h"
#include "Replay.h"
#include "network/PlayerGameCommands.h"
#include "gameTypes/MapInfo.h"
#include "test/testConfig.h"
#include "s25util/tmpFile.h"
#include <boost/filesystem/operations.hpp>
#include <boost/test/unit_test.hpp>
#include <stdexcept>

BOOST_AUTO_TEST_SUITE(HeadlessReplaySuite)

BOOST_AUTO_TEST_CASE(ReplayWithCommandOutOfOrderIsRejected)
{
    MapInfo mapInfo;
    mapInfo.type = MapType::OldMap;
    mapInfo.filepath = rttr::test::rttrBaseDir / "tests" / "testData" / "maps" / "LuaFunctions.SWD";
    BOOST_TEST_REQUIRE(mapInfo.mapData.compressFromFile(mapInfo.filepath));
    PlayerInfo player;
    player.ps = PlayerState::AI;

    Replay replay;
    replay.addPlayer(player);
    TmpFile replayFile(".rpl");
    BOOST_TEST_REQUIRE(replayFile.isValid());
    replayFile.close();
    boost::filesystem::remove(replayFile.filePath); // Recording refuses to overwrite
    BOOST_TEST_REQUIRE(replay.startRecording(replayFile.filePath, mapInfo, 42));
    const PlayerGameCommands noCmds;
    replay.addGameCommand(10, 0, noCmds);
    replay.addGameCommand(5, 0, noCmds);
    replay.updateLastGF(20);
    BOOST_TEST_REQUIRE(replay.stopRecording());

    HeadlessReplay headlessReplay(replayFile.filePath);
    BOOST_CHECK_THROW(headlessReplay.run(), std::runtime_error);
    BOOST_TEST(headlessReplay.getCurrentGF() == 11u);
}

BOOST_AUTO_TEST_SUITE_END()
