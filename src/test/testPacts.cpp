// Copyright (c) 2016 - 2017 Settlers Freaks (sf-team at siedler25.org)
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

#include "rttrDefines.h" // IWYU pragma: keep
#include "GamePlayer.h"
#include "WorldWithGCExecution.h"
#include "postSystem/DiplomacyPostQuestion.h"
#include "postSystem/PostBox.h"
#include <boost/test/unit_test.hpp>

namespace utf = boost::unit_test;

BOOST_AUTO_TEST_SUITE(PactTestSuite)

BOOST_FIXTURE_TEST_CASE(InitialPactStates, WorldWithGCExecution3P)
{
    // No pacts at start
    for(unsigned i = 0; i < world.GetNumPlayers(); i++)
    {
        const GamePlayer& player = world.GetPlayer(i);
        for(unsigned j = 0; j < world.GetNumPlayers(); j++)
        {
            // Self is always an ally and not attackable
            if(i == j)
            {
                BOOST_REQUIRE(player.IsAlly(j));
                BOOST_REQUIRE(!player.IsAttackable(j));
            } else
            {
                BOOST_REQUIRE(!player.IsAlly(j));
                BOOST_REQUIRE(player.IsAttackable(j));
            }
            for(unsigned p = 0; p < NUM_PACTS; p++)
            {
                BOOST_REQUIRE_EQUAL(player.GetPactState(PactType(p), j), GamePlayer::NO_PACT);
                BOOST_REQUIRE_EQUAL(player.GetRemainingPactTime(PactType(p), j), 0u);
            }
        }
    }
}

namespace {
/// Validate the state of a pact between to players. Also checks that the state for the other player is the same
void CheckPactState(const GameWorldBase& world, unsigned playerIdFrom, unsigned playerIdTo, PactType pact,
                    GamePlayer::PactState expectedState)
{
    const GamePlayer& playerFrom = world.GetPlayer(playerIdFrom);
    const GamePlayer& playerTo = world.GetPlayer(playerIdTo);

    BOOST_REQUIRE_EQUAL(playerFrom.GetPactState(pact, playerIdTo), expectedState);
    // If pact was requested, to other player has no pact yet
    if(expectedState == GamePlayer::IN_PROGRESS)
        BOOST_REQUIRE_EQUAL(playerTo.GetPactState(pact, playerIdFrom), GamePlayer::NO_PACT);
    else
        BOOST_REQUIRE_EQUAL(playerTo.GetPactState(pact, playerIdFrom), expectedState);

    // If pact is accepted, we must have some time remaining, else not
    if(expectedState == GamePlayer::ACCEPTED)
        BOOST_REQUIRE_GT(playerFrom.GetRemainingPactTime(pact, playerIdTo), 0u);
    else
        BOOST_REQUIRE_EQUAL(playerFrom.GetRemainingPactTime(pact, playerIdTo), 0u);

    // Remaining times must match
    BOOST_REQUIRE_EQUAL(playerFrom.GetRemainingPactTime(pact, playerIdTo), playerFrom.GetRemainingPactTime(pact, playerIdTo));

    // Attackable must match
    BOOST_REQUIRE_EQUAL(playerFrom.IsAttackable(playerIdTo), playerTo.IsAttackable(playerIdFrom));
    // Ally must match
    BOOST_REQUIRE_EQUAL(playerFrom.IsAlly(playerIdTo), playerTo.IsAlly(playerIdFrom));

    // Attackable only when non-agg. pact not accepted
    BOOST_REQUIRE_EQUAL(playerFrom.IsAttackable(playerIdTo),
                        (playerFrom.GetPactState(NON_AGGRESSION_PACT, playerIdTo) != GamePlayer::ACCEPTED));
    // Ally when treaty of alliance accepted
    BOOST_REQUIRE_EQUAL(playerFrom.IsAlly(playerIdTo), (playerFrom.GetPactState(TREATY_OF_ALLIANCE, playerIdTo) == GamePlayer::ACCEPTED));
}
} // namespace

BOOST_FIXTURE_TEST_CASE(MakePactTest, WorldWithGCExecution3P) //, *utf::depends_on("PactTestSuite/TestInitialPactStates"))
{
    for(unsigned i = 0; i < world.GetNumPlayers(); i++)
        world.GetPostMgr().AddPostBox(i);
    // Use middle player for off-by-one detection
    curPlayer = 1;
    GamePlayer& player1 = world.GetPlayer(1);
    PostBox& postbox2 = *world.GetPostMgr().GetPostBox(2);
    BOOST_REQUIRE_EQUAL(postbox2.GetNumMsgs(), 0u);

    // Invalid length
    this->SuggestPact(2, NON_AGGRESSION_PACT, 0u);
    CheckPactState(world, 1, 2, NON_AGGRESSION_PACT, GamePlayer::NO_PACT);
    // Own player
    this->SuggestPact(1, NON_AGGRESSION_PACT, 0u);
    CheckPactState(world, 1, 2, NON_AGGRESSION_PACT, GamePlayer::NO_PACT);
    BOOST_REQUIRE_EQUAL(postbox2.GetNumMsgs(), 0u);

    const unsigned duration = 10;
    this->SuggestPact(2, NON_AGGRESSION_PACT, duration);
    // Suggesting a pact must send a post message to the target player
    BOOST_REQUIRE_EQUAL(postbox2.GetNumMsgs(), 1u);
    const DiplomacyPostQuestion* msg = dynamic_cast<const DiplomacyPostQuestion*>(postbox2.GetMsg(0));
    BOOST_REQUIRE(msg);
    BOOST_REQUIRE_EQUAL(msg->GetPactType(), NON_AGGRESSION_PACT); //-V522
    BOOST_REQUIRE_EQUAL(msg->GetPlayerId(), curPlayer);
    BOOST_REQUIRE_EQUAL(msg->IsAccept(), true);
    // should be in progress for player1
    CheckPactState(world, 1, 2, NON_AGGRESSION_PACT, GamePlayer::IN_PROGRESS);

    // Same player sends accept -> Disallowed
    this->AcceptPact(msg->GetPactId(), NON_AGGRESSION_PACT, 2);
    CheckPactState(world, 1, 2, NON_AGGRESSION_PACT, GamePlayer::IN_PROGRESS);

    curPlayer = 2;
    // Wrong ID
    this->AcceptPact(msg->GetPactId() + 1, NON_AGGRESSION_PACT, msg->GetPlayerId());
    CheckPactState(world, 1, 2, NON_AGGRESSION_PACT, GamePlayer::IN_PROGRESS);
    // Wrong Pact
    this->AcceptPact(msg->GetPactId(), TREATY_OF_ALLIANCE, msg->GetPlayerId());
    CheckPactState(world, 1, 2, NON_AGGRESSION_PACT, GamePlayer::IN_PROGRESS);
    // Correct
    this->AcceptPact(msg->GetPactId(), NON_AGGRESSION_PACT, msg->GetPlayerId());
    CheckPactState(world, 1, 2, NON_AGGRESSION_PACT, GamePlayer::ACCEPTED);
    // Other pact should not be affected
    CheckPactState(world, 1, 2, TREATY_OF_ALLIANCE, GamePlayer::NO_PACT);

    // Check duration
    BOOST_REQUIRE_EQUAL(player1.GetRemainingPactTime(NON_AGGRESSION_PACT, 2), duration);
}

// Creates a non-aggression pact between players 1 and 2
struct PactCreatedFixture : public WorldWithGCExecution3P
{
    BOOST_STATIC_CONSTEXPR unsigned duration = 10;
    const DiplomacyPostQuestion* msg;
    PactCreatedFixture()
    {
        for(unsigned i = 0; i < world.GetNumPlayers(); i++)
            world.GetPostMgr().AddPostBox(i);
        // Use middle player for off-by-one detection
        curPlayer = 1;
        PostBox& postbox2 = *world.GetPostMgr().GetPostBox(2);

        this->SuggestPact(2, NON_AGGRESSION_PACT, duration);
        msg = dynamic_cast<const DiplomacyPostQuestion*>(postbox2.GetMsg(0));
        BOOST_REQUIRE(msg);
        curPlayer = 2;
        this->AcceptPact(msg->GetPactId(), NON_AGGRESSION_PACT, msg->GetPlayerId());
        CheckPactState(world, 1, 2, NON_AGGRESSION_PACT, GamePlayer::ACCEPTED);
    }
};

BOOST_CONSTEXPR_OR_CONST unsigned PactCreatedFixture::duration;

BOOST_FIXTURE_TEST_CASE(PactDurationTest, PactCreatedFixture) //, *utf::depends_on("PactTestSuite/MakePactTest"))
{
    GamePlayer& player1 = world.GetPlayer(1);

    // Check duration
    BOOST_REQUIRE_EQUAL(player1.GetRemainingPactTime(NON_AGGRESSION_PACT, 2), duration);
    // Execute the GFs during which the pact is valid, starting in the GF in which the pact was accepted
    for(unsigned i = 0; i < duration; i++)
    {
        // Pact still valid
        CheckPactState(world, 1, 2, NON_AGGRESSION_PACT, GamePlayer::ACCEPTED);
        BOOST_REQUIRE_EQUAL(player1.GetRemainingPactTime(NON_AGGRESSION_PACT, 2), duration - i);
        // Double accept -> Do not change
        this->AcceptPact(msg->GetPactId(), NON_AGGRESSION_PACT, msg->GetPlayerId());
        BOOST_REQUIRE_EQUAL(player1.GetRemainingPactTime(NON_AGGRESSION_PACT, 2), duration - i);
        // Advance 1 GF and test pacts
        em.ExecuteNextGF();
        player1.TestPacts();
    }
    // On last GF the pact expired
    BOOST_REQUIRE_EQUAL(player1.GetRemainingPactTime(NON_AGGRESSION_PACT, 2), 0u);
    CheckPactState(world, 1, 2, NON_AGGRESSION_PACT, GamePlayer::NO_PACT);
}

BOOST_FIXTURE_TEST_CASE(PactCanceling, PactCreatedFixture) //, *utf::depends_on("PactTestSuite/MakePactTest"))
{
    PostBox& postbox1 = *world.GetPostMgr().GetPostBox(1);
    PostBox& postbox2 = *world.GetPostMgr().GetPostBox(2);
    postbox1.Clear();
    postbox2.Clear();

    curPlayer = 2;
    this->CancelPact(NON_AGGRESSION_PACT, 1);
    // Pact still alive
    CheckPactState(world, 1, 2, NON_AGGRESSION_PACT, GamePlayer::ACCEPTED);
    // But player1 should have received a question
    BOOST_REQUIRE_EQUAL(postbox1.GetNumMsgs(), 1u);
    msg = dynamic_cast<const DiplomacyPostQuestion*>(postbox1.GetMsg(0));
    BOOST_REQUIRE(msg);
    BOOST_REQUIRE_EQUAL(msg->GetPlayerId(), 2u);
    BOOST_REQUIRE_EQUAL(msg->GetPactType(), NON_AGGRESSION_PACT);
    BOOST_REQUIRE_EQUAL(msg->IsAccept(), false);

    // Same player tries to cancel again
    this->CancelPact(NON_AGGRESSION_PACT, 1);
    // .. or with self
    this->CancelPact(NON_AGGRESSION_PACT, 2);
    // Pact still alive
    CheckPactState(world, 1, 2, NON_AGGRESSION_PACT, GamePlayer::ACCEPTED);

    curPlayer = 1;
    // other player tries self cancel
    this->CancelPact(NON_AGGRESSION_PACT, 1);
    // Pact still alive
    CheckPactState(world, 1, 2, NON_AGGRESSION_PACT, GamePlayer::ACCEPTED);

    // And finally do it. Each player should receive a message
    BOOST_REQUIRE_EQUAL(postbox1.GetNumMsgs(), 2u); // 2 cancel messages
    BOOST_REQUIRE_EQUAL(postbox2.GetNumMsgs(), 0u);
    this->CancelPact(NON_AGGRESSION_PACT, 2);
    CheckPactState(world, 1, 2, NON_AGGRESSION_PACT, GamePlayer::NO_PACT);
    BOOST_REQUIRE_EQUAL(postbox1.GetNumMsgs(), 3u);
    BOOST_REQUIRE_EQUAL(postbox2.GetNumMsgs(), 1u);
    // The new message should not be a question
    BOOST_REQUIRE(!dynamic_cast<const DiplomacyPostQuestion*>(postbox1.GetMsg(postbox1.GetNumMsgs() - 1)));
    BOOST_REQUIRE(!dynamic_cast<const DiplomacyPostQuestion*>(postbox2.GetMsg(postbox2.GetNumMsgs() - 1)));
}

BOOST_AUTO_TEST_SUITE_END()
