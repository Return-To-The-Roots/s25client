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
    void resize(const Extent& newSize) override;
    void setActive(bool activate = true) override;

private:
    void setPlayerReady(unsigned char player, bool ready);
    // GGS von den Controls auslesen
    void updateGGS();
    /// Aktualisiert eine Spielerreihe (löscht Controls und legt neue an)
    void updatePlayerRow(unsigned row);

    /// Füllt die Felder einer Reihe aus
    void changeTeam(unsigned player, Team);
    void changeReady(unsigned player, bool ready);
    void changeNation(unsigned player, Nation);
    void changePortrait(unsigned player, unsigned portraitIndex);
    void changePing(unsigned playerId);
    void changeColor(unsigned player, unsigned color);

    void msgPaintBefore() override;
    void msgGroupButtonClick(unsigned group_id, unsigned ctrl_id) override;
    void msgGroupCheckboxChange(unsigned group_id, unsigned ctrl_id, bool checked) override;
    void msgGroupComboSelectItem(unsigned group_id, unsigned ctrl_id, unsigned selection) override;
    void msgButtonClick(unsigned ctrl_id) override;
    void msgEditEnter(unsigned ctrl_id) override;
    void msgMsgBoxResult(unsigned msgbox_id, MsgboxResult mbr) override;
    void msgComboSelectItem(unsigned ctrl_id, unsigned selection) override;
    void msgCheckboxChange(unsigned ctrl_id, bool checked) override;
    void msgOptionGroupChange(unsigned ctrl_id, unsigned selection) override;

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

    void flashGameChat();

    void LC_Status_Error(const std::string& error) override;
    void LC_Chat(const std::string& player, const std::string& text) override;

    /// Addon options check with regards to peaceful mode and economy mode
    bool forceOptions = false;
    bool checkOptions();

    void goBack();
    bool isSinglePlayer() const { return serverType == ServerType::Local; }
    /// Check whether the given setting can be changed, i.e. is not disabled by lua
    bool isChangeAllowed(const std::string& setting) const;

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
