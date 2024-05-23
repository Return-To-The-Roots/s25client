// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "Desktop.h"
#include "network/ClientInterface.h"
#include "gameTypes/ServerType.h"
#include "liblobby/LobbyInterface.h"
#include <memory>

class ctrlChat;
class GameLobby;
class LobbyPlayerInfo;
class LuaInterfaceSettings;
class GameLobbyController;
class ILobbyClient;
enum class Team : uint8_t;

/// Desktop für das Hosten-eines-Spiels-Fenster
class dskGameLobby final : public Desktop, public ClientInterface, public LobbyInterface
{
public:
    dskGameLobby(ServerType serverType, std::shared_ptr<GameLobby> gameLobby, unsigned playerId,
                 std::unique_ptr<ILobbyClient> lobbyClient);
    ~dskGameLobby();

    /// Größe ändern-Reaktionen die nicht vom Skaling-Mechanismus erfasst werden.
    void Resize(const Extent& newSize) override;
    void SetActive(bool activate = true) override;

private:
    void SetPlayerReady(unsigned char player, bool ready);
    // GGS von den Controls auslesen
    void UpdateGGS();
    /// Aktualisiert eine Spielerreihe (löscht Controls und legt neue an)
    void UpdatePlayerRow(unsigned row);

    /// Füllt die Felder einer Reihe aus
    void ChangeTeam(unsigned player, Team);
    void ChangeReady(unsigned player, bool ready);
    void ChangeNation(unsigned player, Nation);
    void ChangePing(unsigned playerId);
    void ChangeColor(unsigned player, unsigned color);

    void Msg_PaintBefore() override;
    void Msg_Group_ButtonClick(unsigned group_id, unsigned ctrl_id) override;
    void Msg_Group_CheckboxChange(unsigned group_id, unsigned ctrl_id, bool checked) override;
    void Msg_Group_ComboSelectItem(unsigned group_id, unsigned ctrl_id, unsigned selection) override;
    void Msg_ButtonClick(unsigned ctrl_id) override;
    void Msg_EditEnter(unsigned ctrl_id) override;
    void Msg_MsgBoxResult(unsigned msgbox_id, MsgboxResult mbr) override;
    void Msg_ComboSelectItem(unsigned ctrl_id, unsigned selection) override;
    void Msg_CheckboxChange(unsigned ctrl_id, bool checked) override;
    void Msg_OptionGroupChange(unsigned ctrl_id, unsigned selection) override;

    void CI_Error(ClientError ce) override;

    void CI_NewPlayer(unsigned playerId) override;
    void CI_PlayerLeft(unsigned playerId) override;

    void CI_GameLoading(std::shared_ptr<Game> game) override;

    void CI_PlayerDataChanged(unsigned playerId) override;
    void CI_PingChanged(unsigned playerId, unsigned short ping) override;
    void CI_ReadyChanged(unsigned playerId, bool ready) override;
    void CI_PlayersSwapped(unsigned player1, unsigned player2) override;
    void CI_GGSChanged(const GlobalGameSettings& ggs) override;

    void CI_Chat(unsigned playerId, ChatDestination cd, const std::string& msg) override;
    void CI_Countdown(unsigned remainingTimeInSec) override;
    void CI_CancelCountdown(bool error) override;

    void FlashGameChat();

    void LC_Status_Error(const std::string& error) override;
    void LC_Chat(const std::string& player, const std::string& text) override;

    /// Addon options check with regards to peaceful mode and economy mode
    bool forceOptions = false;
    bool checkOptions();

    void GoBack();
    bool IsSinglePlayer() const { return serverType == ServerType::Local; }
    /// Check whether the given setting can be changed, i.e. is not disabled by lua
    bool IsChangeAllowed(const std::string& setting) const;

    const ServerType serverType;
    std::shared_ptr<GameLobby> gameLobby_;
    unsigned localPlayerId_;
    std::unique_ptr<ILobbyClient> lobbyClient_;
    bool hasCountdown_;
    std::unique_ptr<LuaInterfaceSettings> lua;
    std::unique_ptr<GameLobbyController> lobbyController;
    bool wasActivated, allowAddonChange;
    ctrlChat *gameChat, *lobbyChat;
    unsigned lobbyChatTabAnimId, localChatTabAnimId;
};
