// Copyright (c) 2016 - 2018 Settlers Freaks (sf-team at siedler25.org)
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

#include "GameLobby.h"
#include "ILobbyClient.hpp"
#include "JoinPlayerInfo.h"
#include "WindowManager.h"
#include "controls/ctrlButton.h"
#include "controls/ctrlOptionGroup.h"
#include "desktops/dskHostGame.h"
#include "uiHelper/uiHelpers.hpp"
#include <rttr/test/LogAccessor.hpp>
#include <turtle/mock.hpp>
#include <boost/test/unit_test.hpp>

//-V:MOCK_METHOD:813
//-V:MOCK_EXPECT:807

BOOST_AUTO_TEST_SUITE(UI)

/* clang-format off */
MOCK_BASE_CLASS(MockLobbyClient, ILobbyClient)
{
    MOCK_METHOD(IsLoggedIn, 0);
    MOCK_METHOD(AddListener, 1);
    MOCK_METHOD(RemoveListener, 1);
    MOCK_METHOD(SendServerJoinRequest, 0);
    MOCK_METHOD(SendRankingInfoRequest, 1);
    MOCK_METHOD(SendChat, 1);
};
/* clang-format on */

BOOST_FIXTURE_TEST_CASE(LobbyChat, uiHelper::Fixture)
{
    rttr::test::LogAccessor logAcc;

    GameLobby gameLobby(false, true, 2);
    JoinPlayerInfo& player = gameLobby.getPlayer(0);
    player.ps = PS_OCCUPIED;
    player.name = "TestName";
    player.isHost = true;

    auto client = std::make_unique<MockLobbyClient>();
    mock::sequence s, s2;
    MOCK_EXPECT(client->IsLoggedIn).at_least(1).in(s2).returns(true);
    MOCK_EXPECT(client->AddListener).exactly(1).in(s);
    MOCK_EXPECT(client->RemoveListener).exactly(1).in(s);
    MOCK_EXPECT(client->SendServerJoinRequest).exactly(1).in(s2);
    MOCK_EXPECT(client->SendRankingInfoRequest).at_least(1);
    MOCK_EXPECT(client->SendChat).exactly(1);

    // TODO: How to trigger through dskHostGame?
    client->SendChat("");

    auto* desktop = WINDOWMANAGER.Switch(
      std::make_unique<dskHostGame>(ServerType::LOBBY, std::shared_ptr<GameLobby>(&gameLobby, [](auto) {}), 0, std::move(client)));
    auto* ci = dynamic_cast<ClientInterface*>(desktop);
    auto* li = dynamic_cast<LobbyInterface*>(desktop);
    BOOST_REQUIRE(ci && li);
    std::vector<ctrlOptionGroup*> chatTab = desktop->GetCtrls<ctrlOptionGroup>();
    BOOST_REQUIRE_EQUAL(chatTab.size(), 1u);
    std::vector<ctrlButton*> chatBts = chatTab.front()->GetCtrls<ctrlButton>();
    BOOST_REQUIRE_EQUAL(chatBts.size(), 2u);

    WINDOWMANAGER.Draw();

    // Send a chat message via lobby chat and game chat with either visible
    for(unsigned i = 0; i < 3; i++)
    {
        ci->CI_Chat(0, CD_ALL, "Test2");
        RTTR_REQUIRE_LOG_CONTAINS("<TestName>", false);
        li->LC_Chat("OtherPlayer", "Test");
        RTTR_REQUIRE_LOG_CONTAINS("<OtherPlayer>", false);
        desktop->Msg_OptionGroupChange(chatTab.front()->GetID(), chatBts[i % 2]->GetID());
    }
    // Free desktop etc to trigger mock verification
    WINDOWMANAGER.CleanUp();
}

BOOST_AUTO_TEST_SUITE_END()
