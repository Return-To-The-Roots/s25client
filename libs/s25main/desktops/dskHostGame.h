// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
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

#ifndef WP_HOSTGAME_H_
#define WP_HOSTGAME_H_

#include "Desktop.h"
#include "GlobalGameSettings.h"
#include "ILobbyClient.hpp"
#include "network/ClientInterface.h"
#include "gameTypes/ServerType.h"
#include "liblobby/LobbyInterface.h"
#include "libutil/unique_ptr.h"

class ctrlChat;
class GameLobby;
class LobbyPlayerInfo;
class LuaInterfaceSettings;
struct GameLobbyController;

/// Desktop für das Hosten-eines-Spiels-Fenster
class dskHostGame : public Desktop, public ClientInterface, public LobbyInterface
{
public:
    dskHostGame(ServerType serverType, boost::shared_ptr<GameLobby> gameLobby, unsigned playerId,
                libutil::unique_ptr<ILobbyClient> lobbyClient);
    ~dskHostGame();

    /// Größe ändern-Reaktionen die nicht vom Skaling-Mechanismus erfasst werden.
    void Resize(const Extent& newSize) override;
    void SetActive(bool activate = true) override;

private:
    void SetPlayerReady(unsigned char player, bool ready);
    // GGS von den Controls auslesen
    void UpdateGGS();
    /// Aktualisiert eine Spielerreihe (löscht Controls und legt neue an)
    void UpdatePlayerRow(const unsigned row);

    /// Füllt die Felder einer Reihe aus
    void ChangeTeam(const unsigned i, const unsigned char nr);
    void ChangeReady(const unsigned i, const bool ready);
    void ChangeNation(const unsigned i, const Nation nation);
    void ChangePing(unsigned playerId);
    void ChangeColor(const unsigned i, const unsigned color);

    void Msg_PaintBefore() override;
    void Msg_Group_ButtonClick(const unsigned group_id, const unsigned ctrl_id) override;
    void Msg_Group_CheckboxChange(const unsigned group_id, const unsigned ctrl_id, const bool checked) override;
    void Msg_Group_ComboSelectItem(const unsigned group_id, const unsigned ctrl_id, const int selection) override;
    void Msg_ButtonClick(const unsigned ctrl_id) override;
    void Msg_EditEnter(const unsigned ctrl_id) override;
    void Msg_MsgBoxResult(const unsigned msgbox_id, const MsgboxResult mbr) override;
    void Msg_ComboSelectItem(const unsigned ctrl_id, const int selection) override;
    void Msg_CheckboxChange(const unsigned ctrl_id, const bool checked) override;
    void Msg_OptionGroupChange(const unsigned ctrl_id, const int selection) override;

    void LC_RankingInfo(const LobbyPlayerInfo& player) override;

    void CI_Error(const ClientError ce) override;

    void CI_NewPlayer(const unsigned playerId) override;
    void CI_PlayerLeft(const unsigned playerId) override;

    void CI_GameLoading(boost::shared_ptr<Game> game) override;

    void CI_PlayerDataChanged(unsigned playerId) override;
    void CI_PingChanged(const unsigned playerId, const unsigned short ping) override;
    void CI_ReadyChanged(const unsigned playerId, const bool ready) override;
    void CI_PlayersSwapped(const unsigned player1, const unsigned player2) override;
    void CI_GGSChanged(const GlobalGameSettings& ggs) override;

    void CI_Chat(const unsigned playerId, const ChatDestination cd, const std::string& msg) override;
    void CI_Countdown(unsigned remainingTimeInSec) override;
    void CI_CancelCountdown(bool error) override;

    void FlashGameChat();

    void LC_Status_Error(const std::string& error) override;
    void LC_Chat(const std::string& player, const std::string& text) override;

    void GoBack();
    bool IsSinglePlayer() { return serverType == ServerType::LOCAL; }

private:
    const ServerType serverType;
    boost::shared_ptr<GameLobby> gameLobby;
    unsigned localPlayerId_;
    libutil::unique_ptr<ILobbyClient> lobbyClient_;
    bool hasCountdown_;
    libutil::unique_ptr<LuaInterfaceSettings> lua;
    libutil::unique_ptr<GameLobbyController> lobbyHostController;
    bool wasActivated, allowAddonChange;
    ctrlChat *gameChat, *lobbyChat;
    unsigned lobbyChatTabAnimId, localChatTabAnimId;
};

#endif
