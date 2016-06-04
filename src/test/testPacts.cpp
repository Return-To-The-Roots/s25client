// Copyright (c) 2005 - 2016 Settlers Freaks (sf-team at siedler25.org)
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

#include "defines.h" // IWYU pragma: keep
#include "WorldWithGCExecution.h"
#include "GamePlayer.h"
#include "postSystem/PostBox.h"
#include "postSystem/DiplomacyPostQuestion.h"
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(PactTestSuite)

BOOST_FIXTURE_TEST_CASE(MakePactTest, WorldWithGCExecution3P)
{
    for(unsigned i = 0; i < world.GetPlayerCount(); i++)
        world.GetPostMgr().AddPostBox(i);
    // Use middle player for off-by-one detection
    curPlayer = 1;
    GamePlayer& player1 = world.GetPlayer(1);
    // No pacts at start
    for(unsigned i = 0; i < world.GetPlayerCount(); i++)
    {
        // Self is always an ally and not attackable
        if(i == curPlayer)
        {
            BOOST_REQUIRE(player1.IsAlly(i));
            BOOST_REQUIRE(!player1.IsAttackable(i));
        } else{
            BOOST_REQUIRE(!player1.IsAlly(i));
            BOOST_REQUIRE(player1.IsAttackable(i));
        }
        for(unsigned j=0; j<PACTS_COUNT; j++)
        {
            BOOST_REQUIRE_EQUAL(player1.GetPactState(PactType(j), i), GamePlayer::NO_PACT);
            BOOST_REQUIRE_EQUAL(player1.GetRemainingPactTime(PactType(j), i), 0u);
        }
    }
    PostBox& postbox2 = *world.GetPostMgr().GetPostBox(2);
    BOOST_REQUIRE_EQUAL(postbox2.GetNumMsgs(), 0u);

    // Invalid length
    this->SuggestPact(2, NON_AGGRESSION_PACT, 0u);
    BOOST_REQUIRE_EQUAL(player1.GetPactState(TREATY_OF_ALLIANCE, 2), GamePlayer::NO_PACT);
    BOOST_REQUIRE_EQUAL(postbox2.GetNumMsgs(), 0u);

    const unsigned duration = 10;
    this->SuggestPact(2, NON_AGGRESSION_PACT, duration);
    // Suggesting a pact must send a post message to the target player
    BOOST_REQUIRE_EQUAL(postbox2.GetNumMsgs(), 1u);
    const DiplomacyPostQuestion* msg = dynamic_cast<const DiplomacyPostQuestion*>(postbox2.GetMsg(0));
    BOOST_REQUIRE(msg);
    BOOST_REQUIRE_EQUAL(msg->GetPactType(), NON_AGGRESSION_PACT);
    BOOST_REQUIRE_EQUAL(msg->GetPlayerId(), curPlayer);
    GamePlayer& player2 = world.GetPlayer(2);
    // But pact should not have been changed yet
    BOOST_REQUIRE(player1.IsAttackable(2));
    BOOST_REQUIRE(player2.IsAttackable(1));
    // But should be in progress for player1
    BOOST_REQUIRE_EQUAL(player1.GetPactState(NON_AGGRESSION_PACT, 2), GamePlayer::IN_PROGRESS);
    BOOST_REQUIRE_EQUAL(player2.GetPactState(NON_AGGRESSION_PACT, 1), GamePlayer::NO_PACT);

    // Same player sends accept -> Disallowed
    this->AcceptPact(msg->GetPactId(), NON_AGGRESSION_PACT, 2);
    BOOST_REQUIRE_EQUAL(player1.GetPactState(NON_AGGRESSION_PACT, 2), GamePlayer::IN_PROGRESS);
    BOOST_REQUIRE_EQUAL(player2.GetPactState(NON_AGGRESSION_PACT, 1), GamePlayer::NO_PACT);

    curPlayer = 2;
    // Wrong ID
    this->AcceptPact(msg->GetPactId() + 1, NON_AGGRESSION_PACT, msg->GetPlayerId());
    BOOST_REQUIRE_EQUAL(player1.GetPactState(NON_AGGRESSION_PACT, 2), GamePlayer::IN_PROGRESS);
    BOOST_REQUIRE_EQUAL(player2.GetPactState(NON_AGGRESSION_PACT, 1), GamePlayer::NO_PACT);
    // Wrong Pact
    this->AcceptPact(msg->GetPactId(), TREATY_OF_ALLIANCE, msg->GetPlayerId());
    BOOST_REQUIRE_EQUAL(player1.GetPactState(NON_AGGRESSION_PACT, 2), GamePlayer::IN_PROGRESS);
    BOOST_REQUIRE_EQUAL(player2.GetPactState(NON_AGGRESSION_PACT, 1), GamePlayer::NO_PACT);
    BOOST_REQUIRE_EQUAL(player1.GetPactState(TREATY_OF_ALLIANCE, 2), GamePlayer::NO_PACT);
    BOOST_REQUIRE_EQUAL(player2.GetPactState(TREATY_OF_ALLIANCE, 1), GamePlayer::NO_PACT);
    // Correct
    this->AcceptPact(msg->GetPactId(), NON_AGGRESSION_PACT, msg->GetPlayerId());
    BOOST_REQUIRE_EQUAL(player1.GetPactState(NON_AGGRESSION_PACT, 2), GamePlayer::ACCEPTED);
    BOOST_REQUIRE_EQUAL(player2.GetPactState(NON_AGGRESSION_PACT, 1), GamePlayer::ACCEPTED);
    BOOST_REQUIRE_EQUAL(player1.GetPactState(TREATY_OF_ALLIANCE, 2), GamePlayer::NO_PACT);
    BOOST_REQUIRE_EQUAL(player2.GetPactState(TREATY_OF_ALLIANCE, 1), GamePlayer::NO_PACT);
    BOOST_REQUIRE(!player1.IsAttackable(2));
    BOOST_REQUIRE(!player2.IsAttackable(1));
    
    // Check duration
    BOOST_REQUIRE_EQUAL(player1.GetRemainingPactTime(NON_AGGRESSION_PACT, 2), duration);
    em.ExecuteNextGF();
    BOOST_REQUIRE_EQUAL(player1.GetRemainingPactTime(NON_AGGRESSION_PACT, 2), duration - 1);
    // Double accept -> Do not change
    this->AcceptPact(msg->GetPactId(), NON_AGGRESSION_PACT, msg->GetPlayerId());
    BOOST_REQUIRE_EQUAL(player1.GetRemainingPactTime(NON_AGGRESSION_PACT, 2), duration - 1);
    // Execute the GFs during which the pact is valid (1 already run)
    for(unsigned i = 1; i < duration; i++)
    {
        BOOST_REQUIRE_EQUAL(player1.GetPactState(NON_AGGRESSION_PACT, 2), GamePlayer::ACCEPTED);
        BOOST_REQUIRE_EQUAL(player1.GetRemainingPactTime(NON_AGGRESSION_PACT, 2), duration - i);
        em.ExecuteNextGF();
        player1.TestPacts();
    }
    // On last GF the pact expired (first GF is the GF in which the pact was concluded)
    BOOST_REQUIRE_EQUAL(player1.GetRemainingPactTime(NON_AGGRESSION_PACT, 2), 0u);
    BOOST_REQUIRE_EQUAL(player1.GetPactState(NON_AGGRESSION_PACT, 2), GamePlayer::NO_PACT);
    BOOST_REQUIRE_EQUAL(player2.GetRemainingPactTime(NON_AGGRESSION_PACT, 2), 0u);
    BOOST_REQUIRE_EQUAL(player2.GetPactState(NON_AGGRESSION_PACT, 2), GamePlayer::NO_PACT);
    player1.TestPacts();
}

BOOST_AUTO_TEST_SUITE_END()
