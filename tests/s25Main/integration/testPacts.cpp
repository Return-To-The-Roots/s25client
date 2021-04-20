// Copyright (c) 2016 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "GamePlayer.h"
#include "enum_cast.hpp"
#include "postSystem/DiplomacyPostQuestion.h"
#include "postSystem/PostBox.h"
#include "worldFixtures/WorldWithGCExecution.h"
#include "gameTypes/GameTypesOutput.h"
#include <boost/test/unit_test.hpp>

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
    for(unsigned i = 0; i < world.GetNumPlayers(); i++)
    {
        const GamePlayer& player = world.GetPlayer(i);
        for(unsigned j = 0; j < world.GetNumPlayers(); j++)
        {
            // Self is always an ally and not attackable
            if(i == j)
            {
                BOOST_TEST_REQUIRE(player.IsAlly(j));
                BOOST_TEST_REQUIRE(!player.IsAttackable(j));
            } else
            {
                BOOST_TEST_REQUIRE(!player.IsAlly(j));
                BOOST_TEST_REQUIRE(player.IsAttackable(j));
            }
            for(const auto p : helpers::enumRange<PactType>())
            {
                BOOST_TEST_REQUIRE(player.GetPactState(p, j) == PactState::None);
                BOOST_TEST_REQUIRE(player.GetRemainingPactTime(p, j) == 0u);
            }
        }
    }
}

namespace {
/// Validate the state of a pact between to players. Also checks that the state for the other player is the same
void CheckPactState(const GameWorldBase& world, unsigned playerIdFrom, unsigned playerIdTo, PactType pact,
                    PactState expectedState)
{
    const GamePlayer& playerFrom = world.GetPlayer(playerIdFrom);
    const GamePlayer& playerTo = world.GetPlayer(playerIdTo);

    BOOST_TEST_REQUIRE(playerFrom.GetPactState(pact, playerIdTo) == expectedState);
    // If pact was requested, to other player has no pact yet
    if(expectedState == PactState::InProgress)
        BOOST_TEST_REQUIRE(playerTo.GetPactState(pact, playerIdFrom) == PactState::None);
    else
        BOOST_TEST_REQUIRE(playerTo.GetPactState(pact, playerIdFrom) == expectedState);

    // If pact is accepted, we must have some time remaining, else not
    if(expectedState == PactState::Accepted)
        BOOST_TEST_REQUIRE(playerFrom.GetRemainingPactTime(pact, playerIdTo) > 0u);
    else
        BOOST_TEST_REQUIRE(playerFrom.GetRemainingPactTime(pact, playerIdTo) == 0u);

    // Remaining times must match
    BOOST_TEST_REQUIRE(playerFrom.GetRemainingPactTime(pact, playerIdTo)
                       == playerFrom.GetRemainingPactTime(pact, playerIdTo));

    // Attackable must match
    BOOST_TEST_REQUIRE(playerFrom.IsAttackable(playerIdTo) == playerTo.IsAttackable(playerIdFrom));
    // Ally must match
    BOOST_TEST_REQUIRE(playerFrom.IsAlly(playerIdTo) == playerTo.IsAlly(playerIdFrom));

    // Attackable only when non-agg. pact not accepted
    BOOST_TEST_REQUIRE(playerFrom.IsAttackable(playerIdTo)
                       == (playerFrom.GetPactState(PactType::NonAgressionPact, playerIdTo) != PactState::Accepted));
    // Ally when treaty of alliance accepted
    BOOST_TEST_REQUIRE(playerFrom.IsAlly(playerIdTo)
                       == (playerFrom.GetPactState(PactType::TreatyOfAlliance, playerIdTo) == PactState::Accepted));
}
} // namespace

BOOST_FIXTURE_TEST_CASE(MakePactTest,
                        WorldWithGCExecution3P) //, *utf::depends_on("PactTestSuite/TestInitialPactStates"))
{
    for(unsigned i = 0; i < world.GetNumPlayers(); i++)
        world.GetPostMgr().AddPostBox(i);
    // Use middle player for off-by-one detection
    curPlayer = 1;
    GamePlayer& player1 = world.GetPlayer(1);
    PostBox& postbox2 = *world.GetPostMgr().GetPostBox(2);
    BOOST_TEST_REQUIRE(postbox2.GetNumMsgs() == 0u);

    // Invalid length
    this->SuggestPact(2, PactType::NonAgressionPact, 0u);
    CheckPactState(world, 1, 2, PactType::NonAgressionPact, PactState::None);
    // Own player
    this->SuggestPact(1, PactType::NonAgressionPact, 0u);
    CheckPactState(world, 1, 2, PactType::NonAgressionPact, PactState::None);
    BOOST_TEST_REQUIRE(postbox2.GetNumMsgs() == 0u);

    const unsigned duration = 10;
    this->SuggestPact(2, PactType::NonAgressionPact, duration);
    // Suggesting a pact must send a post message to the target player
    BOOST_TEST_REQUIRE(postbox2.GetNumMsgs() == 1u);
    const auto* msg = dynamic_cast<const DiplomacyPostQuestion*>(postbox2.GetMsg(0));
    BOOST_TEST_REQUIRE(msg);
    BOOST_TEST_REQUIRE(msg->GetPactType() == PactType::NonAgressionPact); //-V522
    BOOST_TEST_REQUIRE(msg->GetPlayerId() == curPlayer);
    BOOST_TEST_REQUIRE(msg->IsAccept() == true);
    // should be in progress for player1
    CheckPactState(world, 1, 2, PactType::NonAgressionPact, PactState::InProgress);

    // Same player sends accept -> Disallowed
    this->AcceptPact(msg->GetPactId(), PactType::NonAgressionPact, 2);
    CheckPactState(world, 1, 2, PactType::NonAgressionPact, PactState::InProgress);

    curPlayer = 2;
    // Wrong ID
    this->AcceptPact(msg->GetPactId() + 1, PactType::NonAgressionPact, msg->GetPlayerId());
    CheckPactState(world, 1, 2, PactType::NonAgressionPact, PactState::InProgress);
    // Wrong Pact
    this->AcceptPact(msg->GetPactId(), PactType::TreatyOfAlliance, msg->GetPlayerId());
    CheckPactState(world, 1, 2, PactType::NonAgressionPact, PactState::InProgress);
    // Correct
    this->AcceptPact(msg->GetPactId(), PactType::NonAgressionPact, msg->GetPlayerId());
    CheckPactState(world, 1, 2, PactType::NonAgressionPact, PactState::Accepted);
    // Other pact should not be affected
    CheckPactState(world, 1, 2, PactType::TreatyOfAlliance, PactState::None);

    // Check duration
    BOOST_TEST_REQUIRE(player1.GetRemainingPactTime(PactType::NonAgressionPact, 2) == duration);
}

// Creates a non-aggression pact between players 1 and 2
struct PactCreatedFixture : public WorldWithGCExecution3P
{
    static constexpr unsigned duration = 10;
    const DiplomacyPostQuestion* msg;
    PactCreatedFixture()
    {
        for(unsigned i = 0; i < world.GetNumPlayers(); i++)
            world.GetPostMgr().AddPostBox(i);
        // Use middle player for off-by-one detection
        curPlayer = 1;
        PostBox& postbox2 = *world.GetPostMgr().GetPostBox(2);

        this->SuggestPact(2, PactType::NonAgressionPact, duration);
        msg = dynamic_cast<const DiplomacyPostQuestion*>(postbox2.GetMsg(0));
        BOOST_TEST_REQUIRE(msg);
        curPlayer = 2;
        this->AcceptPact(msg->GetPactId(), PactType::NonAgressionPact, msg->GetPlayerId());
        CheckPactState(world, 1, 2, PactType::NonAgressionPact, PactState::Accepted);
    }
};

constexpr unsigned PactCreatedFixture::duration;

BOOST_FIXTURE_TEST_CASE(PactDurationTest, PactCreatedFixture) //, *utf::depends_on("PactTestSuite/MakePactTest"))
{
    GamePlayer& player1 = world.GetPlayer(1);

    // Check duration
    BOOST_TEST_REQUIRE(player1.GetRemainingPactTime(PactType::NonAgressionPact, 2) == duration);
    // Execute the GFs during which the pact is valid, starting in the GF in which the pact was accepted
    for(unsigned i = 0; i < duration; i++)
    {
        // Pact still valid
        CheckPactState(world, 1, 2, PactType::NonAgressionPact, PactState::Accepted);
        BOOST_TEST_REQUIRE(player1.GetRemainingPactTime(PactType::NonAgressionPact, 2) == duration - i);
        // Double accept -> Do not change
        this->AcceptPact(msg->GetPactId(), PactType::NonAgressionPact, msg->GetPlayerId());
        BOOST_TEST_REQUIRE(player1.GetRemainingPactTime(PactType::NonAgressionPact, 2) == duration - i);
        // Advance 1 GF and test pacts
        em.ExecuteNextGF();
        player1.TestPacts();
    }
    // On last GF the pact expired
    BOOST_TEST_REQUIRE(player1.GetRemainingPactTime(PactType::NonAgressionPact, 2) == 0u);
    CheckPactState(world, 1, 2, PactType::NonAgressionPact, PactState::None);
}

BOOST_FIXTURE_TEST_CASE(PactCanceling, PactCreatedFixture) //, *utf::depends_on("PactTestSuite/MakePactTest"))
{
    PostBox& postbox1 = *world.GetPostMgr().GetPostBox(1);
    PostBox& postbox2 = *world.GetPostMgr().GetPostBox(2);
    postbox1.Clear();
    postbox2.Clear();

    curPlayer = 2;
    this->CancelPact(PactType::NonAgressionPact, 1);
    // Pact still alive
    CheckPactState(world, 1, 2, PactType::NonAgressionPact, PactState::Accepted);
    // But player1 should have received a question
    BOOST_TEST_REQUIRE(postbox1.GetNumMsgs() == 1u);
    msg = dynamic_cast<const DiplomacyPostQuestion*>(postbox1.GetMsg(0));
    BOOST_TEST_REQUIRE(msg);
    BOOST_TEST_REQUIRE(msg->GetPlayerId() == 2u);
    BOOST_TEST_REQUIRE(msg->GetPactType() == PactType::NonAgressionPact);
    BOOST_TEST_REQUIRE(msg->IsAccept() == false);

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
    BOOST_TEST_REQUIRE(postbox1.GetNumMsgs() == 2u); // 2 cancel messages
    BOOST_TEST_REQUIRE(postbox2.GetNumMsgs() == 0u);
    this->CancelPact(PactType::NonAgressionPact, 2);
    CheckPactState(world, 1, 2, PactType::NonAgressionPact, PactState::None);
    BOOST_TEST_REQUIRE(postbox1.GetNumMsgs() == 3u);
    BOOST_TEST_REQUIRE(postbox2.GetNumMsgs() == 1u);
    // The new message should not be a question
    BOOST_TEST_REQUIRE(!dynamic_cast<const DiplomacyPostQuestion*>(postbox1.GetMsg(postbox1.GetNumMsgs() - 1)));
    BOOST_TEST_REQUIRE(!dynamic_cast<const DiplomacyPostQuestion*>(postbox2.GetMsg(postbox2.GetNumMsgs() - 1)));
}

BOOST_AUTO_TEST_SUITE_END()
