// Copyright (c) 2005 - 2015 Settlers Freaks (sf-team at siedler25.org)
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
#include "ClientInterface.h"
#include "LobbyInterface.h"
#include "GameProtocol.h"
#include "GlobalGameSettings.h"
#include "helpers/Deleter.h"
#include <boost/interprocess/smart_ptr/unique_ptr.hpp>

class GameWorldViewer;
class LobbyPlayerInfo;
class LuaInterfaceSettings;

/// Desktop für das Hosten-eines-Spiels-Fenster
class dskHostGame :
    public Desktop,
    public ClientInterface,
    public LobbyInterface
{
    public:

        /// Map übergeben, damit die Kartenvorschau erstellt werden kann
        dskHostGame(const ServerType serverType);

        /// Größe ändern-Reaktionen die nicht vom Skaling-Mechanismus erfasst werden.
        void Resize_(unsigned short width, unsigned short height) override;
        void SetActive(bool activate = true) override;
    private:

        void TogglePlayerReady(unsigned char player, bool ready);
        // GGS von den Controls auslesen
        void UpdateGGS();
        /// Aktualisiert eine Spielerreihe (löscht Controls und legt neue an)
        void UpdatePlayerRow(const unsigned row);

        /// Füllt die Felder einer Reihe aus
        void ChangeTeam(const unsigned i, const unsigned char nr);
        void ChangeReady(const unsigned i, const bool ready);
        void ChangeNation(const unsigned i, const Nation nation);
        void ChangePing(const unsigned i);
        void ChangeColor(const unsigned i, const unsigned color);

        void Msg_PaintBefore() override;
        void Msg_Group_ButtonClick(const unsigned int group_id, const unsigned int ctrl_id) override;
        void Msg_Group_CheckboxChange(const unsigned int group_id, const unsigned int ctrl_id, const bool checked) override;
        void Msg_Group_ComboSelectItem(const unsigned int group_id, const unsigned int ctrl_id, const int selection) override;
        void Msg_ButtonClick(const unsigned int ctrl_id) override;
        void Msg_EditEnter(const unsigned int ctrl_id) override;
        void Msg_MsgBoxResult(const unsigned msgbox_id, const MsgboxResult mbr) override;
        void Msg_ComboSelectItem(const unsigned int ctrl_id, const int selection) override;
        void Msg_CheckboxChange(const unsigned int ctrl_id, const bool checked) override;

        void LC_RankingInfo(const LobbyPlayerInfo& player) override;

        void CI_Error(const ClientError ce) override;

        void CI_NewPlayer(const unsigned player_id) override;
        void CI_PlayerLeft(const unsigned player_id) override;

        void CI_GameStarted(GameWorldViewer* gwv) override;

        void CI_PSChanged(const unsigned player_id, const PlayerState ps) override;
        void CI_NationChanged(const unsigned player_id, const Nation nation) override;
        void CI_TeamChanged(const unsigned player_id, const unsigned char team) override;
        void CI_PingChanged(const unsigned player_id, const unsigned short ping) override;
        void CI_ColorChanged(const unsigned player_id, const unsigned color) override;
        void CI_ReadyChanged(const unsigned player_id, const bool ready) override;
        void CI_PlayersSwapped(const unsigned player1, const unsigned player2) override;
        void CI_GGSChanged(const GlobalGameSettings& ggs) override;

        void CI_Chat(const unsigned player_id, const ChatDestination cd, const std::string& msg) override;
        void CI_Countdown(int countdown) override;
        void CI_CancelCountdown() override;

        void LC_Status_Error(const std::string& error) override;

        void GoBack();
        bool IsSinglePlayer(){ return serverType == ServerType::LOCAL; }
    private:
        GlobalGameSettings ggs_;
        bool hasCountdown_;
        const ServerType serverType;
        boost::interprocess::unique_ptr<LuaInterfaceSettings, Deleter<LuaInterfaceSettings> > lua;
        bool wasActivated, allowAddonChange;
};


#endif
