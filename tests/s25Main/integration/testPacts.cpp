// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "GamePlayer.h"
#include "enum_cast.hpp"
#include "postSystem/DiplomacyPostQuestion.h"
#include "postSystem/PostBox.h"
#include "worldFixtures/WorldWithGCExecution.h"
#include "gameTypes/GameTypesOutput.h"
#include <s25util/boostTestHelpers.h>
#include <boost/test/unit_test.hpp>

namespace utf = boost::unit_test;

// LCOV_EXCL_START
static std::ostream& operator<<(std::ostream& out, const PactState e)
{
    return out << static_cast<unsigned>(rttr::enum_cast(e));
}
// LCOV_EXCL_STOP

BOOST_AUTO_TEST_SUITE(PactTestSuite)

BOOST_FIXTURE_TEST_CASE(InitialPactStates, WorldWithGCExecution3P)
{
    // No pacts at start
    for(unsigned i = 0; i < world.getNumPlayers(); i++)
    {
        const GamePlayer& player = world.getPlayer(i);
        for(unsigned j = 0; j < world.getNumPlayers(); j++)
        {
            BOOST_TEST_INFO_SCOPE(i << " vs " << j);
            // Self is always an ally and not attackable
            if(i == j)
            {
                BOOST_TEST(player.isAlly(j));
                BOOST_TEST(!player.isAttackable(j));
            } else
            {
                BOOST_TEST(!player.isAlly(j));
                BOOST_TEST(player.isAttackable(j));
            }
            for(const auto p : helpers::enumRange<PactType>())
            {
                BOOST_TEST_INFO_SCOPE("Pact " << p);
                BOOST_TEST(player.getPactState(p, j) == PactState::None);
                BOOST_TEST(player.getRemainingPactTime(p, j) == 0u);
            }
        }
    }
}

namespace {
/// Validate the state of a pact between to players. Also checks that the state for the other player is the same
void CheckPactState(const GameWorldBase& world, unsigned playerIdFrom, unsigned playerIdTo, PactType pact,
                    PactState expectedState)
{
    const GamePlayer& playerFrom = world.getPlayer(playerIdFrom);
    const GamePlayer& playerTo = world.getPlayer(playerIdTo);

    BOOST_TEST_REQUIRE(playerFrom.getPactState(pact, playerIdTo) == expectedState);
    // If pact was requested, to other player has no pact yet
    if(expectedState == PactState::InProgress)
        BOOST_TEST_REQUIRE(playerTo.getPactState(pact, playerIdFrom) == PactState::None);
    else
        BOOST_TEST_REQUIRE(playerTo.getPactState(pact, playerIdFrom) == expectedState);

    // If pact is accepted, we must have some time remaining, else not
    if(expectedState == PactState::Accepted)
        BOOST_TEST_REQUIRE(playerFrom.getRemainingPactTime(pact, playerIdTo) > 0u);
    else
        BOOST_TEST_REQUIRE(playerFrom.getRemainingPactTime(pact, playerIdTo) == 0u);

    // Remaining times must match
    BOOST_TEST_REQUIRE(playerFrom.getRemainingPactTime(pact, playerIdTo)
                       == playerFrom.getRemainingPactTime(pact, playerIdTo));

    // Attackable must match
    BOOST_TEST_REQUIRE(playerFrom.isAttackable(playerIdTo) == playerTo.isAttackable(playerIdFrom));
    // Ally must match
    BOOST_TEST_REQUIRE(playerFrom.isAlly(playerIdTo) == playerTo.isAlly(playerIdFrom));

    // Attackable only when non-agg. pact not accepted
    BOOST_TEST_REQUIRE(playerFrom.isAttackable(playerIdTo)
                       == (playerFrom.getPactState(PactType::NonAgressionPact, playerIdTo) != PactState::Accepted));
    // Ally when treaty of alliance accepted
    BOOST_TEST_REQUIRE(playerFrom.isAlly(playerIdTo)
                       == (playerFrom.getPactState(PactType::TreatyOfAlliance, playerIdTo) == PactState::Accepted));
}
} // namespace

BOOST_FIXTURE_TEST_CASE(MakePactTest, WorldWithGCExecution3P, *utf::depends_on("PactTestSuite/InitialPactStates"))
{
    for(unsigned i = 0; i < world.getNumPlayers(); i++)
        world.getPostMgr().AddPostBox(i);
    // Use middle player for off-by-one detection
    curPlayer = 1;
    GamePlayer& player1 = world.getPlayer(1);
    PostBox& postbox2 = *world.getPostMgr().getPostBox(2);
    BOOST_TEST_REQUIRE(postbox2.getNumMsgs() == 0u);

    // Invalid length
    this->SuggestPact(2, PactType::NonAgressionPact, 0u);
    CheckPactState(world, 1, 2, PactType::NonAgressionPact, PactState::None);
    // Own player
    this->SuggestPact(1, PactType::NonAgressionPact, 0u);
    CheckPactState(world, 1, 2, PactType::NonAgressionPact, PactState::None);
    BOOST_TEST_REQUIRE(postbox2.getNumMsgs() == 0u);

    const unsigned duration = 10;
    this->SuggestPact(2, PactType::NonAgressionPact, duration);
    // Suggesting a pact must send a post message to the target player
    BOOST_TEST_REQUIRE(postbox2.getNumMsgs() == 1u);
    const auto* msg = dynamic_cast<const DiplomacyPostQuestion*>(postbox2.getMsg(0));
    BOOST_TEST_REQUIRE(msg);
    BOOST_TEST(msg->getPactType() == PactType::NonAgressionPact); //-V522
    BOOST_TEST(msg->getPlayerId() == curPlayer);
    BOOST_TEST(msg->isAccept() == true);
    // should be in progress for player1
    CheckPactState(world, 1, 2, PactType::NonAgressionPact, PactState::InProgress);

    // Same player sends accept -> Disallowed
    this->AcceptPact(msg->getPactId(), PactType::NonAgressionPact, 2);
    CheckPactState(world, 1, 2, PactType::NonAgressionPact, PactState::InProgress);

    curPlayer = 2;
    // Wrong ID
    this->AcceptPact(msg->getPactId() + 1, PactType::NonAgressionPact, msg->getPlayerId());
    CheckPactState(world, 1, 2, PactType::NonAgressionPact, PactState::InProgress);
    // Wrong Pact
    this->AcceptPact(msg->getPactId(), PactType::TreatyOfAlliance, msg->getPlayerId());
    CheckPactState(world, 1, 2, PactType::NonAgressionPact, PactState::InProgress);
    // Correct
    this->AcceptPact(msg->getPactId(), PactType::NonAgressionPact, msg->getPlayerId());
    CheckPactState(world, 1, 2, PactType::NonAgressionPact, PactState::Accepted);
    // Other pact should not be affected
    CheckPactState(world, 1, 2, PactType::TreatyOfAlliance, PactState::None);

    // Check duration
    BOOST_TEST(player1.getRemainingPactTime(PactType::NonAgressionPact, 2) == duration);
}

// Creates a non-aggression pact between players 1 and 2
struct PactCreatedFixture : public WorldWithGCExecution3P
{
    static constexpr unsigned duration = 10;
    const DiplomacyPostQuestion* msg;
    PactCreatedFixture()
    {
        for(unsigned i = 0; i < world.getNumPlayers(); i++)
            world.getPostMgr().AddPostBox(i);
        // Use middle player for off-by-one detection
        curPlayer = 1;
        PostBox& postbox2 = *world.getPostMgr().getPostBox(2);

        this->SuggestPact(2, PactType::NonAgressionPact, duration);
        msg = dynamic_cast<const DiplomacyPostQuestion*>(postbox2.getMsg(0));
        BOOST_TEST_REQUIRE(msg);
        curPlayer = 2;
        this->AcceptPact(msg->getPactId(), PactType::NonAgressionPact, msg->getPlayerId());
        CheckPactState(world, 1, 2, PactType::NonAgressionPact, PactState::Accepted);
    }
};

BOOST_FIXTURE_TEST_CASE(PactDurationTest, PactCreatedFixture, *utf::depends_on("PactTestSuite/MakePactTest"))
{
    GamePlayer& player1 = world.getPlayer(1);

    // Check duration
    BOOST_TEST_REQUIRE(player1.getRemainingPactTime(PactType::NonAgressionPact, 2) == duration);
    // Execute the GFs during which the pact is valid, starting in the GF in which the pact was accepted
    for(unsigned i = 0; i < duration; i++)
    {
        // Pact still valid
        CheckPactState(world, 1, 2, PactType::NonAgressionPact, PactState::Accepted);
        BOOST_TEST_REQUIRE(player1.getRemainingPactTime(PactType::NonAgressionPact, 2) == duration - i);
        // Double accept -> Do not change
        this->AcceptPact(msg->getPactId(), PactType::NonAgressionPact, msg->getPlayerId());
        BOOST_TEST_REQUIRE(player1.getRemainingPactTime(PactType::NonAgressionPact, 2) == duration - i);
        // Advance 1 GF and test pacts
        em.ExecuteNextGF();
        player1.TestPacts();
    }
    // On last GF the pact expired
    BOOST_TEST(player1.getRemainingPactTime(PactType::NonAgressionPact, 2) == 0u);
    CheckPactState(world, 1, 2, PactType::NonAgressionPact, PactState::None);
}

BOOST_FIXTURE_TEST_CASE(PactCanceling, PactCreatedFixture, *utf::depends_on("PactTestSuite/MakePactTest"))
{
    PostBox& postbox1 = *world.getPostMgr().getPostBox(1);
    PostBox& postbox2 = *world.getPostMgr().getPostBox(2);
    postbox1.Clear();
    postbox2.Clear();

    curPlayer = 2;
    this->CancelPact(PactType::NonAgressionPact, 1);
    // Pact still alive
    CheckPactState(world, 1, 2, PactType::NonAgressionPact, PactState::Accepted);
    // But player1 should have received a question
    BOOST_TEST_REQUIRE(postbox1.getNumMsgs() == 1u);
    msg = dynamic_cast<const DiplomacyPostQuestion*>(postbox1.getMsg(0));
    BOOST_TEST_REQUIRE(msg);
    BOOST_TEST(msg->getPlayerId() == 2u);
    BOOST_TEST(msg->getPactType() == PactType::NonAgressionPact);
    BOOST_TEST(msg->isAccept() == false);

    // Same player tries to cancel again
    this->CancelPact(PactType::NonAgressionPact, 1);
    // .. or with self
    this->CancelPact(PactType::NonAgressionPact, 2);
    // Pact still alive
    CheckPactState(world, 1, 2, PactType::NonAgressionPact, PactState::Accepted);

    curPlayer = 1;
    // other player tries self cancel
    this->CancelPact(PactType::NonAgressionPact, 1);
    // Pact still alive
    CheckPactState(world, 1, 2, PactType::NonAgressionPact, PactState::Accepted);

    // And finally do it. Each player should receive a message
    BOOST_TEST_REQUIRE(postbox1.getNumMsgs() == 2u); // 2 cancel messages
    BOOST_TEST_REQUIRE(postbox2.getNumMsgs() == 0u);
    this->CancelPact(PactType::NonAgressionPact, 2);
    CheckPactState(world, 1, 2, PactType::NonAgressionPact, PactState::None);
    BOOST_TEST_REQUIRE(postbox1.getNumMsgs() == 3u);
    BOOST_TEST_REQUIRE(postbox2.getNumMsgs() == 1u);
    // The new message should not be a question
    BOOST_TEST(!dynamic_cast<const DiplomacyPostQuestion*>(postbox1.getMsg(postbox1.getNumMsgs() - 1)));
    BOOST_TEST(!dynamic_cast<const DiplomacyPostQuestion*>(postbox2.getMsg(postbox2.getNumMsgs() - 1)));
}

BOOST_FIXTURE_TEST_CASE(SuggestNewPactAfterExpiration, PactCreatedFixture,
                        *utf::depends_on("PactTestSuite/MakePactTest"))
{
    GamePlayer& player1 = world.getPlayer(1);
    PostBox& postbox2 = *world.getPostMgr().getPostBox(2);
    postbox2.Clear();

    // Execute the GFs until the pact is expired
    for(unsigned i = 0; i < duration; i++)
    {
        em.ExecuteNextGF();
        player1.TestPacts();
    }

    // On last GF the pact expired. Suggest new pact.
    CheckPactState(world, 1, 2, PactType::NonAgressionPact, PactState::None);
    curPlayer = 1;
    this->SuggestPact(2, PactType::NonAgressionPact, duration);

    // Test if other player has received the suggestion
    BOOST_TEST_REQUIRE(postbox2.getNumMsgs() == 1u);
    msg = dynamic_cast<const DiplomacyPostQuestion*>(postbox2.getMsg(0));
    BOOST_TEST_REQUIRE(msg);
    // Other player accepts
    curPlayer = 2;
    this->AcceptPact(msg->getPactId(), PactType::NonAgressionPact, msg->getPlayerId());
    // pact state must be accepted
    CheckPactState(world, 1, 2, PactType::NonAgressionPact, PactState::Accepted);
}

BOOST_AUTO_TEST_SUITE_END()
