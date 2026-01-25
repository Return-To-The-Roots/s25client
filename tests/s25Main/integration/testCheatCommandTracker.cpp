// Copyright (C) 2024-2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "CheatCommandTracker.h"
#include "Cheats.h"
#include "GameCommands.h"
#include "GamePlayer.h"
#include "driver/KeyEvent.h"
#include "factories/GameCommandFactory.h"
#include "worldFixtures/CreateEmptyWorld.h"
#include "worldFixtures/WorldFixture.h"
#include <vector>

BOOST_AUTO_TEST_SUITE(CheatCommandTrackerTests)

namespace {
struct StoreGCFactory : public GameCommandFactory
{
    std::vector<gc::GameCommandPtr> gcs;

protected:
    bool AddGC(gc::GameCommandPtr gc) override
    {
        gcs.push_back(gc);
        return true;
    }
};
template<unsigned T_numPlayers>
struct CheatCommandTrackerFixture : WorldFixture<CreateEmptyWorld, T_numPlayers>
{
    StoreGCFactory gcFactory_;
    Cheats cheats_{this->world, gcFactory_};
    CheatCommandTracker tracker_{cheats_};

    void trackString(const std::string& str)
    {
        for(char32_t c : str)
            tracker_.onKeyEvent(KeyEvent(c));
    }
};
using CheatCommandTrackerFixture1P = CheatCommandTrackerFixture<1>;
using CheatCommandTrackerFixture2P = CheatCommandTrackerFixture<2>;
using CheatCommandTrackerFixture4P = CheatCommandTrackerFixture<4>;
} // namespace

BOOST_FIXTURE_TEST_CASE(CheatModeCanBeTurnedOnAndOffRepeatedly, CheatCommandTrackerFixture1P)
{
    BOOST_TEST_REQUIRE(cheats_.isCheatModeOn() == false); // initially off
    trackString("winter");
    BOOST_TEST_REQUIRE(cheats_.isCheatModeOn() == true);
    trackString("winter");
    BOOST_TEST_REQUIRE(cheats_.isCheatModeOn() == false);
    trackString("winter");
    BOOST_TEST_REQUIRE(cheats_.isCheatModeOn() == true);
    trackString("winter");
    BOOST_TEST_REQUIRE(cheats_.isCheatModeOn() == false);
}

BOOST_FIXTURE_TEST_CASE(CheatModeCannotBeTurnedOn_InMultiplayer, CheatCommandTrackerFixture2P)
{
    trackString("winter");
    BOOST_TEST_REQUIRE(cheats_.isCheatModeOn() == false);
    // Also no chat commands
    tracker_.onChatCommand("apocalypsis");
    BOOST_TEST(gcFactory_.gcs.empty());
}

BOOST_FIXTURE_TEST_CASE(CheatModeIsNotTurnedOn_IfTheCheatStringIsWrong, CheatCommandTrackerFixture1P)
{
    trackString("winte"); // incomplete
    BOOST_TEST_REQUIRE(cheats_.isCheatModeOn() == false);

    trackString("win");
    tracker_.onKeyEvent(KeyEvent(KeyType::F10)); // interrupted by another key type
    trackString("ter");
    BOOST_TEST_REQUIRE(cheats_.isCheatModeOn() == false);

    trackString("wainter"); // interrupted by another letter
    BOOST_TEST_REQUIRE(cheats_.isCheatModeOn() == false);

    trackString("rwinte"); // order of characters is wrong
    BOOST_TEST_REQUIRE(cheats_.isCheatModeOn() == false);

    trackString("winnter"); // character repeated
    BOOST_TEST_REQUIRE(cheats_.isCheatModeOn() == false);

    trackString("w\x82nter"); // Non-ASCII character
    BOOST_TEST_REQUIRE(cheats_.isCheatModeOn() == false);
}

BOOST_FIXTURE_TEST_CASE(CheatModeIsTurnedOn_WhenTheFirstCharacterIsRepeated, CheatCommandTrackerFixture1P)
{
    trackString("wwwinter");
    BOOST_TEST_REQUIRE(cheats_.isCheatModeOn() == true);
}

BOOST_FIXTURE_TEST_CASE(CheatModeIsTurnedOn_EvenWhenWrongInputsWereProvidedBefore, CheatCommandTrackerFixture1P)
{
    trackString("www");
    KeyEvent ke('1');
    ke.alt = true;
    tracker_.onKeyEvent(ke);
    trackString("interwitter");
    BOOST_TEST_REQUIRE(cheats_.isCheatModeOn() == false);
    trackString("winter");
    BOOST_TEST_REQUIRE(cheats_.isCheatModeOn() == true);
}

BOOST_FIXTURE_TEST_CASE(CheatCommands, CheatCommandTrackerFixture1P)
{
    tracker_.onChatCommand("apocalypsis");
    BOOST_TEST(gcFactory_.gcs.empty());
    tracker_.onChatCommand("impulse9");
    BOOST_TEST(cheats_.areAllBuildingsEnabled() == false);
    tracker_.onChatCommand("spies");
    BOOST_TEST(cheats_.shouldShowEnemyProductivityOverlay() == false);
    tracker_.onKeyEvent(KeyEvent(KeyType::F7));
    BOOST_TEST(cheats_.isAllVisible() == false);

    // Activate cheats, still everything disabled
    cheats_.toggleCheatMode();
    BOOST_TEST(gcFactory_.gcs.empty());
    BOOST_TEST(cheats_.areAllBuildingsEnabled() == false);
    BOOST_TEST(cheats_.shouldShowEnemyProductivityOverlay() == false);
    BOOST_TEST(cheats_.isAllVisible() == false);

    tracker_.onChatCommand("apocalypsis");
    BOOST_TEST_REQUIRE(!gcFactory_.gcs.empty());
    BOOST_TEST(dynamic_cast<gc::CheatArmageddon*>(gcFactory_.gcs.back().get()));
    tracker_.onChatCommand("impulse9");
    BOOST_TEST(cheats_.areAllBuildingsEnabled() == true);
    tracker_.onChatCommand("spies");
    BOOST_TEST(cheats_.shouldShowEnemyProductivityOverlay() == true);
    tracker_.onKeyEvent(KeyEvent(KeyType::F7));
    BOOST_TEST(cheats_.isAllVisible() == true);
}

BOOST_FIXTURE_TEST_CASE(CheatKeys, CheatCommandTrackerFixture4P)
{
    using RRM = Cheats::ResourceRevealMode;
    auto& p1 = world.GetPlayer(0);
    auto& p2 = world.GetPlayer(1);
    auto& p3 = world.GetPlayer(2);
    auto& p4 = world.GetPlayer(3);
    p2.ps = p3.ps = p4.ps = PlayerState::AI;

    const KeyEvent evKeyVisible(KeyType::F7);
    KeyEvent evKeyResourceReveal(KeyType::F7);
    evKeyResourceReveal.alt = true;

    tracker_.onKeyEvent(evKeyVisible);
    BOOST_TEST(cheats_.isAllVisible() == false);
    tracker_.onKeyEvent(evKeyResourceReveal);
    BOOST_CHECK(cheats_.getResourceRevealMode() == RRM::Nothing);

    // Activate cheats, still everything disabled
    cheats_.toggleCheatMode();
    BOOST_TEST(gcFactory_.gcs.empty());
    BOOST_TEST(cheats_.isAllVisible() == false);

    tracker_.onKeyEvent(evKeyVisible);
    BOOST_TEST(cheats_.isAllVisible() == true);
    tracker_.onKeyEvent(evKeyResourceReveal);
    BOOST_CHECK(cheats_.getResourceRevealMode() != RRM::Nothing);

    // CTRL+SHIFT+F1-F8 destroy buildings of players 1-8
    KeyEvent evDestroy(KeyType::F3);
    tracker_.onKeyEvent(evDestroy);
    BOOST_TEST_REQUIRE(!p3.IsDefeated());
    evDestroy.ctrl = true;
    tracker_.onKeyEvent(evDestroy);
    BOOST_TEST_REQUIRE(!p3.IsDefeated());
    evDestroy.ctrl = false;
    evDestroy.shift = true;
    tracker_.onKeyEvent(evDestroy);
    BOOST_TEST_REQUIRE(!p3.IsDefeated());
    evDestroy.ctrl = evDestroy.shift = true;
    tracker_.onKeyEvent(evDestroy);
    BOOST_TEST(p3.IsDefeated());

    // Destroy AI players with CTRL+SHIFT+F9
    evDestroy.kt = KeyType::F9;
    evDestroy.ctrl = evDestroy.shift = true;
    BOOST_TEST_REQUIRE(!p2.IsDefeated());
    BOOST_TEST_REQUIRE(!p4.IsDefeated());
    tracker_.onKeyEvent(evDestroy);
    BOOST_TEST(!p1.IsDefeated());
    BOOST_TEST(p2.IsDefeated());
    BOOST_TEST(p4.IsDefeated());
}

BOOST_AUTO_TEST_SUITE_END()
